#pragma once

#include <exprflow/graph/inplace_function.hpp>

#include <any>
#include <cstdint>
#include <format>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace exprflow {

class TaskGraph {
 public:
  using NodeId = uint32_t;
  using NodeTask = detail::InplaceFunction<std::any(std::vector<std::any>&)>;

 private:
  // After Seal(): CSR of outgoing edges — successors of i are
  // flat_edges_[flat_offsets_[i] .. flat_offsets_[i + 1]).
  std::vector<NodeId> flat_edges_;
  std::vector<uint32_t> flat_offsets_;

  std::vector<NodeTask> tasks_;
  std::vector<std::shared_ptr<void>> owned_;

  std::vector<std::vector<NodeId>> out_adj_;
  std::vector<std::uint32_t> indegree_;

  bool sealed_{false};

 public:
  TaskGraph() = default;
  ~TaskGraph() = default;

  TaskGraph(const TaskGraph&) = delete;
  TaskGraph& operator=(const TaskGraph&) = delete;
  TaskGraph(TaskGraph&&) = delete;
  TaskGraph& operator=(TaskGraph&&) = delete;
  auto AddNode(NodeTask task) -> NodeId;

  template <typename T>
  T* Save(T value) {
    if (Sealed()) [[unlikely]] {
      throw std::logic_error(std::format(
          "TaskGraph::Save: cannot save, graph is already sealed"));
    }
    auto owned = std::make_shared<T>(std::move(value));
    T* raw = owned.get();
    owned_.push_back(std::move(owned));
    return raw;
  }

  void AddEdge(NodeId from, NodeId to);
  void Seal();
  bool Sealed() const;
  uint32_t Size() const;

  auto Task(NodeId id) const -> const NodeTask&;
  auto Successors(NodeId id) const -> std::span<const NodeId>;
  auto Indegrees() const -> std::span<const std::uint32_t>;
};

}  // namespace exprflow
