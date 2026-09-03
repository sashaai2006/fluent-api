#include <exprflow/executor.hpp>

#include <atomic>
#include <cstdint>
#include <exception>
#include <format>
#include <stdexcept>
#include <utility>

struct alignas(64) PaddedCounter {
  std::atomic<std::uint32_t> value{0};
};

struct Executor::RunState {
  explicit RunState(std::shared_ptr<const TaskGraph> g)
      : graph(std::move(g)),
        results(graph->Size()),
        unresolved(std::make_unique<PaddedCounter[]>(graph->Size())),
        remaining(graph->Size()) {
    const auto indeg = graph->Indegrees();
    for (std::size_t i = 0; i < indeg.size(); ++i) {
      unresolved[i].value.store(indeg[i], std::memory_order_relaxed);
    }
  }

  void Fail(std::exception_ptr error) {
    {
      std::lock_guard lock(error_mutex);
      if (!first_error) {
        first_error = std::move(error);
      }
    }
    failed.store(true, std::memory_order_release);
  }

  void Settle() {
    if (first_error) {
      done.set_exception(first_error);
    } else {
      done.set_value(std::move(results));
    }
  }

  std::shared_ptr<const TaskGraph> graph;
  std::vector<std::any> results;
  std::unique_ptr<PaddedCounter[]> unresolved;
  std::atomic<std::uint32_t> remaining;
  std::atomic<bool> failed{false};
  std::mutex error_mutex;
  std::exception_ptr first_error;
  std::promise<std::vector<std::any>> done;
};

Executor::Executor(std::size_t threads) {
  if (threads == 0) {
    threads = 1;
  }
  workers_.reserve(threads);
  try {
    for (std::size_t i = 0; i < threads; ++i) {
      workers_.emplace_back([this]() { Loop(); });
    }
  } catch (...) {
    queue_.Close();
    throw;
  }
}

Executor::~Executor() {
  {
    std::unique_lock lock(runs_mutex_);
    stopping_ = true;
    runs_cv_.wait(lock, [this] { return active_runs_ == 0; });
  }
  queue_.Close();
}

auto Executor::Submit(std::shared_ptr<const TaskGraph> graph)
    -> std::future<std::vector<std::any>> {
  if (!graph) {
    throw std::logic_error(std::format("Executor::Submit: graph is null"));
  }
  if (!graph->Sealed()) {
    throw std::logic_error(
        std::format("Executor::Submit: graph is not sealed"));
  }

  auto run = std::make_shared<RunState>(std::move(graph));
  auto future = run->done.get_future();

  if (run->graph->Size() == 0) {
    run->Settle();
    return future;
  }

  {
    std::lock_guard lock(runs_mutex_);
    if (stopping_) {
      throw std::logic_error(
          std::format("Executor::Submit: executor is stopping"));
    }
    ++active_runs_;
  }

  const auto indeg = run->graph->Indegrees();
  for (TaskGraph::NodeId id = 0; id < run->graph->Size(); ++id) {
    if (indeg[id] == 0u) {
      queue_.Push(WorkItem{run, id});
    }
  }
  return future;
}

void Executor::Loop() {
  while (auto item = queue_.Get()) {
    ExecuteNode(*item);
  }
}

void Executor::ExecuteNode(const WorkItem& item) {
  const auto& [run, id] = item;

  if (!run->failed.load(std::memory_order_acquire)) {
    try {
      run->results[id] = run->graph->Task(id)(run->results);
    } catch (...) {
      run->Fail(std::current_exception());
    }
  }

  for (const auto succ : run->graph->Successors(id)) {
    if (run->unresolved[succ].value.fetch_sub(1, std::memory_order_acq_rel) ==
        1) {
      queue_.Push(WorkItem{run, succ});
    }
  }

  if (run->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
    run->Settle();
    OnRunFinished();
  }
}

void Executor::OnRunFinished() {
  std::lock_guard lock(runs_mutex_);
  if (--active_runs_ == 0) {
    runs_cv_.notify_one();
  }
}
