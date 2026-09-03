#pragma once

#include "block_queue.hpp"
#include "dag/graph.hpp"

#include <condition_variable>
#include <cstddef>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace dag {

class Executor {
 public:
  explicit Executor(std::size_t threads = std::thread::hardware_concurrency());
  ~Executor();

  Executor(const Executor&) = delete;
  Executor& operator=(const Executor&) = delete;
  Executor(Executor&&) = delete;
  Executor& operator=(Executor&&) = delete;

  auto Submit(std::shared_ptr<const TaskGraph> graph) -> std::future<void>;

 private:
  struct RunState;
  using WorkItem = std::pair<std::shared_ptr<RunState>, TaskGraph::NodeId>;

  void Loop();
  void ExecuteNode(const WorkItem& item);
  void OnRunFinished();
  BlockQueue<WorkItem> queue_;
  std::vector<std::jthread> workers_;

  std::mutex runs_mutex_;
  std::condition_variable runs_cv_;
  std::size_t active_runs_{0};
  bool stopping_{false};
};

}  // namespace dag
