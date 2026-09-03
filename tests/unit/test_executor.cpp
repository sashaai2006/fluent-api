#include <exprflow/executor.hpp>
#include <exprflow/graph/graph.hpp>

#include <gtest/gtest.h>

#include <any>
#include <atomic>
#include <chrono>
#include <format>
#include <memory>
#include <stdexcept>
#include <thread>

using namespace exprflow;

namespace {

constexpr std::size_t kTwoWorkers = 2;
constexpr std::size_t kFourWorkers = 4;
constexpr int kIndependentNodes = 64;
constexpr int kChainLength = 16;

auto MakeSealedGraph() {
  return std::make_shared<TaskGraph>();
}

}  // namespace

TEST(executor, should_throw_on_null_graph) {
  Executor executor(kTwoWorkers);
  EXPECT_THROW((void)executor.Submit(nullptr), std::logic_error);
}

TEST(executor, should_throw_on_unsealed_graph) {
  Executor executor(kTwoWorkers);
  auto graph = MakeSealedGraph();
  graph->AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  EXPECT_THROW((void)executor.Submit(graph), std::logic_error);
}

TEST(executor, should_complete_empty_graph) {
  Executor executor(kTwoWorkers);
  auto graph = MakeSealedGraph();
  graph->Seal();
  executor.Submit(graph).get();
}

TEST(executor, should_run_chain_in_order) {
  Executor executor(kTwoWorkers);
  auto graph = MakeSealedGraph();
  std::atomic<int> next{0};
  std::atomic<bool> order_ok{true};

  TaskGraph::NodeId prev = 0;
  for (int i = 0; i < kChainLength; ++i) {
    const auto id = graph->AddNode([&, i](std::vector<std::any>&) -> std::any {
      if (next.load() != i) {
        order_ok.store(false);
      }
      next.store(i + 1);
      return {};
    });
    if (i > 0) {
      graph->AddEdge(prev, id);
    }
    prev = id;
  }
  graph->Seal();
  executor.Submit(graph).get();

  EXPECT_TRUE(order_ok.load());
  EXPECT_EQ(next.load(), kChainLength);
}

TEST(executor, should_run_diamond_with_join_after_both_parents) {
  Executor executor(kFourWorkers);
  auto graph = MakeSealedGraph();
  std::atomic<int> a_done{0};
  std::atomic<int> b_done{0};
  std::atomic<int> c_done{0};
  std::atomic<int> join_parents{0};

  const auto a = graph->AddNode([&](std::vector<std::any>&) -> std::any {
    a_done.store(1);
    return {};
  });
  const auto b = graph->AddNode([&](std::vector<std::any>&) -> std::any {
    if (a_done.load() != 1) {
      b_done.store(-1);
      return {};
    }
    b_done.store(1);
    return {};
  });
  const auto c = graph->AddNode([&](std::vector<std::any>&) -> std::any {
    if (a_done.load() != 1) {
      c_done.store(-1);
      return {};
    }
    c_done.store(1);
    return {};
  });
  const auto d = graph->AddNode([&](std::vector<std::any>&) -> std::any {
    join_parents.store(b_done.load() + c_done.load());
    return {};
  });
  graph->AddEdge(a, b);
  graph->AddEdge(a, c);
  graph->AddEdge(b, d);
  graph->AddEdge(c, d);
  graph->Seal();
  executor.Submit(graph).get();

  EXPECT_EQ(a_done.load(), 1);
  EXPECT_EQ(b_done.load(), 1);
  EXPECT_EQ(c_done.load(), 1);
  EXPECT_EQ(join_parents.load(), 2);
}

TEST(executor, should_run_independent_nodes) {
  Executor executor(kFourWorkers);
  auto graph = MakeSealedGraph();
  std::atomic<int> counter{0};
  for (int i = 0; i < kIndependentNodes; ++i) {
    graph->AddNode([&](std::vector<std::any>&) -> std::any {
      counter.fetch_add(1);
      return {};
    });
  }
  graph->Seal();
  executor.Submit(graph).get();
  EXPECT_EQ(counter.load(), kIndependentNodes);
}

TEST(executor, should_rethrow_task_exception_from_future) {
  Executor executor(kTwoWorkers);
  auto graph = MakeSealedGraph();
  graph->AddNode([](std::vector<std::any>&) -> std::any {
    throw std::runtime_error{std::format("boom")};
  });
  graph->Seal();

  auto future = executor.Submit(graph);
  try {
    future.get();
    FAIL() << "expected std::runtime_error";
  } catch (const std::runtime_error& e) {
    EXPECT_STREQ(e.what(), "boom");
  }
}

TEST(executor, should_skip_dependents_after_failure) {
  Executor executor(kTwoWorkers);
  auto graph = MakeSealedGraph();
  std::atomic<bool> dependent_ran{false};

  const auto a = graph->AddNode([](std::vector<std::any>&) -> std::any {
    throw std::runtime_error{std::format("boom")};
  });
  const auto b = graph->AddNode([&](std::vector<std::any>&) -> std::any {
    dependent_ran.store(true);
    return {};
  });
  graph->AddEdge(a, b);
  graph->Seal();

  auto future = executor.Submit(graph);
  EXPECT_THROW(future.get(), std::runtime_error);
  EXPECT_FALSE(dependent_ran.load());
}

TEST(executor, should_run_same_graph_twice) {
  Executor executor(kTwoWorkers);
  auto graph = MakeSealedGraph();
  std::atomic<int> counter{0};
  const auto a = graph->AddNode([&](std::vector<std::any>&) -> std::any {
    counter.fetch_add(1);
    return {};
  });
  const auto b = graph->AddNode([&](std::vector<std::any>&) -> std::any {
    counter.fetch_add(1);
    return {};
  });
  graph->AddEdge(a, b);
  graph->Seal();

  executor.Submit(graph).get();
  executor.Submit(graph).get();
  EXPECT_EQ(counter.load(), 2 * 2);
}

TEST(executor, should_run_graphs_concurrently) {
  Executor executor(kFourWorkers);
  std::atomic<int> counter{0};

  auto make_chain = [&] {
    auto graph = MakeSealedGraph();
    TaskGraph::NodeId prev = 0;
    for (int i = 0; i < kChainLength; ++i) {
      const auto id = graph->AddNode([&](std::vector<std::any>&) -> std::any {
        counter.fetch_add(1);
        return {};
      });
      if (i > 0) {
        graph->AddEdge(prev, id);
      }
      prev = id;
    }
    graph->Seal();
    return graph;
  };

  auto first = executor.Submit(make_chain());
  auto second = executor.Submit(make_chain());
  first.get();
  second.get();
  EXPECT_EQ(counter.load(), 2 * kChainLength);
}

TEST(executor, should_run_same_graph_concurrently_with_itself) {
  Executor executor(kFourWorkers);
  auto graph = MakeSealedGraph();
  std::atomic<int> counter{0};
  for (int i = 0; i < kIndependentNodes; ++i) {
    graph->AddNode([&](std::vector<std::any>&) -> std::any {
      counter.fetch_add(1);
      return {};
    });
  }
  graph->Seal();

  auto first = executor.Submit(graph);
  auto second = executor.Submit(graph);
  first.get();
  second.get();
  EXPECT_EQ(counter.load(), 2 * kIndependentNodes);
}

TEST(executor, should_finish_inflight_run_before_destruction) {
  std::future<std::vector<std::any>> future;
  std::atomic<bool> done{false};
  {
    Executor executor(kTwoWorkers);
    auto graph = MakeSealedGraph();
    graph->AddNode([&](std::vector<std::any>&) -> std::any {
      std::this_thread::sleep_for(std::chrono::milliseconds{50});
      done.store(true);
      return {};
    });
    graph->Seal();
    future = executor.Submit(graph);
  }
  EXPECT_NO_THROW(future.get());
  EXPECT_TRUE(done.load());
}
