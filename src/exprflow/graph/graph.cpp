#include <exprflow/graph/graph.hpp>

#include <algorithm>
#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>
#include <utility>

uint32_t TaskGraph::Size() const {
  return static_cast<uint32_t>(tasks_.size());
}

auto TaskGraph::AddNode(NodeTask task) -> NodeId {
  if (Sealed()) [[unlikely]] {
    throw std::logic_error(std::format(
        "TaskGraph::AddNode: cannot add node, graph is already sealed"));
  }

  if (Size() >= std::numeric_limits<NodeId>::max()) [[unlikely]] {
    throw std::length_error(std::format(
        "TaskGraph::AddNode: graph limit reached, maximum allowed nodes: {}",
        std::numeric_limits<NodeId>::max()));
  }

  const auto id = static_cast<NodeId>(Size());
  tasks_.push_back(std::move(task));
  out_adj_.emplace_back();
  indegree_.push_back(0);
  return id;
}

void TaskGraph::AddEdge(NodeId from, NodeId to) {
  if (Sealed()) [[unlikely]] {
    throw std::logic_error(std::format(
        "TaskGraph::AddEdge: cannot add edge, graph is already sealed"));
  }

  if (from >= Size()) [[unlikely]] {
    throw std::out_of_range(std::format(
        "TaskGraph::AddEdge: 'from' NodeId ({}) is invalid, total nodes: {}",
        from, Size()));
  }

  if (to >= Size()) [[unlikely]] {
    throw std::out_of_range(std::format(
        "TaskGraph::AddEdge: 'to' NodeId ({}) is invalid, total nodes: {}", to,
        Size()));
  }

  if (from == to) [[unlikely]] {
    throw std::logic_error(
        std::format("TaskGraph::AddEdge: self-loops are forbidden, attempted "
                    "to connect node {} to itself",
                    from));
  }

  out_adj_[from].push_back(to);
  ++indegree_[to];
}

void TaskGraph::Seal() {
  if (Sealed()) [[unlikely]] {
    throw std::logic_error(
        std::format("TaskGraph::Seal: graph is already sealed"));
  }

  const auto n = Size();

  std::vector<std::uint32_t> indeg = indegree_;
  std::vector<NodeId> order;
  order.reserve(n);
  for (NodeId i = 0; i < n; ++i) {
    if (indeg[i] == 0) {
      order.push_back(i);
    }
  }
  for (std::size_t qi = 0; qi < order.size(); ++qi) {
    for (NodeId v : out_adj_[order[qi]]) {
      if (--indeg[v] == 0) {
        order.push_back(v);
      }
    }
  }
  if (order.size() != static_cast<std::size_t>(n)) {
    throw std::logic_error(std::format("TaskGraph::Seal: cycle"));
  }

  flat_offsets_.assign(static_cast<std::size_t>(n) + 1, 0);
  for (NodeId i = 0; i < n; ++i) {
    flat_offsets_[i + 1] =
        flat_offsets_[i] + static_cast<std::uint32_t>(out_adj_[i].size());
  }

  flat_edges_.resize(flat_offsets_[n]);
  for (NodeId i = 0; i < n; ++i) {
    std::ranges::copy(out_adj_[i], flat_edges_.begin() + flat_offsets_[i]);
  }

  out_adj_.clear();
  out_adj_.shrink_to_fit();

  sealed_ = true;
}

bool TaskGraph::Sealed() const {
  return sealed_;
}

auto TaskGraph::Task(NodeId id) const -> const NodeTask& {
  if (!Sealed()) [[unlikely]] {
    throw std::logic_error(std::format("TaskGraph::Task: graph is not sealed"));
  }

  if (id >= Size()) [[unlikely]] {
    throw std::out_of_range(
        std::format("TaskGraph::Task: NodeId ({}) is invalid, total nodes: {}",
                    id, Size()));
  }

  return tasks_[id];
}

auto TaskGraph::Successors(NodeId id) const -> std::span<const NodeId> {
  if (!Sealed()) [[unlikely]] {
    throw std::logic_error(
        std::format("TaskGraph::Successors: graph is not sealed"));
  }

  if (id >= Size()) [[unlikely]] {
    throw std::out_of_range(std::format(
        "TaskGraph::Successors: NodeId ({}) is invalid, total nodes: {}", id,
        Size()));
  }

  const auto begin = flat_offsets_[id];
  const auto end = flat_offsets_[id + 1];
  return {flat_edges_.data() + begin, end - begin};
}

auto TaskGraph::Indegrees() const -> std::span<const std::uint32_t> {
  if (!Sealed()) [[unlikely]] {
    throw std::logic_error(
        std::format("TaskGraph::Indegrees: graph is not sealed"));
  }

  return indegree_;
}
