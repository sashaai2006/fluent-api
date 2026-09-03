#include <exprflow/graph/graph.hpp>

#include <gtest/gtest.h>

#include <any>
#include <cstddef>
#include <stdexcept>
#include <vector>

constexpr std::size_t kTenNodes = 10;

TEST(task_graph, should_report_not_sealed_when_default_constructed) {
  TaskGraph graph;
  EXPECT_FALSE(graph.Sealed());
  EXPECT_EQ(graph.Size(), 0u);
}

TEST(task_graph, should_report_size_after_adding_nodes) {
  TaskGraph graph;
  for (std::size_t i = 0; i < kTenNodes; ++i) {
    graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  }
  EXPECT_EQ(graph.Size(), kTenNodes);
}

TEST(task_graph, should_throw_when_adding_node_after_seal) {
  TaskGraph graph;
  graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  graph.Seal();
  EXPECT_THROW(
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; }),
      std::logic_error);
}

TEST(task_graph, should_throw_when_save_after_seal) {
  TaskGraph graph;
  graph.Seal();
  EXPECT_THROW(graph.Save(1), std::logic_error);
}

TEST(task_graph, should_throw_when_adding_edge_after_seal) {
  TaskGraph graph;
  const auto a =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  const auto b =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  graph.Seal();
  EXPECT_THROW(graph.AddEdge(a, b), std::logic_error);
}

TEST(task_graph, should_throw_when_edge_from_unknown_node) {
  TaskGraph graph;
  const auto a =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  EXPECT_THROW(graph.AddEdge(a + 1, a), std::out_of_range);
}

TEST(task_graph, should_throw_when_edge_to_unknown_node) {
  TaskGraph graph;
  const auto a =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  EXPECT_THROW(graph.AddEdge(a, a + 1), std::out_of_range);
}

TEST(task_graph, should_throw_when_self_loop) {
  TaskGraph graph;
  const auto a =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  EXPECT_THROW(graph.AddEdge(a, a), std::logic_error);
}

TEST(task_graph, should_throw_when_seal_twice) {
  TaskGraph graph;
  graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  graph.Seal();
  EXPECT_THROW(graph.Seal(), std::logic_error);
}

TEST(task_graph, should_throw_when_cycle_on_seal) {
  TaskGraph graph;
  const auto a =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  const auto b =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  graph.AddEdge(a, b);
  graph.AddEdge(b, a);
  EXPECT_THROW(graph.Seal(), std::logic_error);
  EXPECT_FALSE(graph.Sealed());
}

TEST(task_graph, should_remain_unsealed_when_disconnected_cycle) {
  TaskGraph graph;
  graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  const auto b =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  const auto c =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  graph.AddEdge(b, c);
  graph.AddEdge(c, b);
  EXPECT_THROW(graph.Seal(), std::logic_error);
  EXPECT_FALSE(graph.Sealed());
}

TEST(task_graph, should_seal_empty_graph) {
  TaskGraph graph;
  graph.Seal();
  EXPECT_TRUE(graph.Sealed());
  EXPECT_EQ(graph.Size(), 0u);
  EXPECT_TRUE(graph.Indegrees().empty());
}

TEST(task_graph, should_throw_when_task_before_seal) {
  TaskGraph graph;
  const auto a =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  EXPECT_THROW((void)graph.Task(a), std::logic_error);
}

TEST(task_graph, should_throw_when_successors_before_seal) {
  TaskGraph graph;
  const auto a =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  EXPECT_THROW((void)graph.Successors(a), std::logic_error);
}

TEST(task_graph, should_throw_when_indegrees_before_seal) {
  TaskGraph graph;
  graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  EXPECT_THROW((void)graph.Indegrees(), std::logic_error);
}

TEST(task_graph, should_throw_when_task_unknown_node) {
  TaskGraph graph;
  graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  graph.Seal();
  EXPECT_THROW((void)graph.Task(1), std::out_of_range);
}

TEST(task_graph, should_throw_when_successors_unknown_node) {
  TaskGraph graph;
  graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  graph.Seal();
  EXPECT_THROW((void)graph.Successors(1), std::out_of_range);
}

TEST(task_graph, should_invoke_task_obtained_from_accessor) {
  TaskGraph graph;
  bool ran = false;
  const auto a = graph.AddNode([&](std::vector<std::any>&) -> std::any {
    ran = true;
    return {};
  });
  graph.Seal();
  std::vector<std::any> results(graph.Size());
  graph.Task(a)(results);
  EXPECT_TRUE(ran);
}

TEST(task_graph, should_expose_successors_after_seal) {
  TaskGraph graph;
  const auto a =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  const auto b =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  const auto c =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  const auto d =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  graph.AddEdge(a, b);
  graph.AddEdge(a, c);
  graph.AddEdge(b, d);
  graph.AddEdge(c, d);
  graph.Seal();

  const auto a_succ = graph.Successors(a);
  ASSERT_EQ(a_succ.size(), 2u);
  EXPECT_EQ(a_succ[0], b);
  EXPECT_EQ(a_succ[1], c);

  const auto b_succ = graph.Successors(b);
  ASSERT_EQ(b_succ.size(), 1u);
  EXPECT_EQ(b_succ[0], d);

  EXPECT_TRUE(graph.Successors(d).empty());
}

TEST(task_graph, should_expose_indegrees_after_seal) {
  TaskGraph graph;
  const auto a =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  const auto b =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  const auto c =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  const auto d =
      graph.AddNode([](std::vector<std::any>&) -> std::any { return {}; });
  graph.AddEdge(a, b);
  graph.AddEdge(a, c);
  graph.AddEdge(b, d);
  graph.AddEdge(c, d);
  graph.Seal();

  const auto indeg = graph.Indegrees();
  ASSERT_EQ(indeg.size(), 4u);
  EXPECT_EQ(indeg[a], 0u);
  EXPECT_EQ(indeg[b], 1u);
  EXPECT_EQ(indeg[c], 1u);
  EXPECT_EQ(indeg[d], 2u);
}
