#pragma once

#include "inline_task.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace dag {

class TaskGraph {
 public:
  using NodeId = uint32_t;

 private:
  /*
    flat_edges_ — «лента» рёбер (сплющенный vector<vector<NodeId>>).
    flat_edges_[e] = k  <=>  ребро номер e на ленте ведёт в узел k.
    Индекс e — номер места на ленте, НЕ номер узла-отправителя.
    flat_edges_[0] = 1 значит «ребро №0 ведёт в 1», а не «потомки узла 0».

    НО без вспомогательной структуры не понятно, чей это кусок:
    где кончились исходящие узла 0 и начались исходящие узла 1.


    Группа i — все исходящие рёбра ОДНОГО отправителя i (0, 1 или несколько).
    На ленте группы лежат встык: сначала все из 0, сразу все из 1, …

    flat_offsets_ — линейка начал групп, длина node_count + 1.
    flat_offsets_[i]     = место на ленте, где начинается группа узла i
    flat_offsets_[i + 1] = где начинается группа i+1 = конец группы i (не
    включая)

    Соседи i:
      flat_edges_[ flat_offsets_[i]  ..  flat_offsets_[i + 1] )

    Последний столбик flat_offsets_[n] — «лента кончилась» (= число рёбер).
    Пустая группа: flat_offsets_[i] == flat_offsets_[i + 1], кусок нулевой
    длины.
 */

  std::vector<NodeId> flat_edges_;
  std::vector<uint32_t> flat_offsets_;

  std::vector<InlineTask<void()>> tasks_;

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

  auto AddNode(InlineTask<void()> task) -> NodeId;
  void AddEdge(NodeId from, NodeId to);
  void Seal();
  bool Sealed() const;
  uint32_t Size() const;

  auto Task(NodeId id) const -> const InlineTask<void()>&;
  auto Successors(NodeId id) const -> std::span<const NodeId>;
  auto Indegrees() const -> std::span<const std::uint32_t>;
};

}  // namespace dag
