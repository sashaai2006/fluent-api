#include <dag/graph.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <stdexcept>
#include <vector>

using namespace dag;

constexpr std::size_t kTenNodes = 10;

TEST(task_graph, should_report_not_sealed_when_default_constructed) {
  TaskGraph graph;
  EXPECT_FALSE(graph.Sealed());
  EXPECT_EQ(graph.Size(), 0u);
}

TEST(task_graph, should_report_size_after_adding_nodes) {
  TaskGraph graph;
  for (std::size_t i = 0; i < kTenNodes; ++i) {
    graph.AddNode([] {});
  }
  EXPECT_EQ(graph.Size(), kTenNodes);
}

TEST(task_graph, should_throw_when_adding_node_after_seal) {
  TaskGraph graph;
  graph.AddNode([] {});
  graph.Seal();
  EXPECT_THROW(graph.AddNode([] {}), std::logic_error);
}

TEST(task_graph, should_throw_when_adding_edge_after_seal) {
  TaskGraph graph;
  const auto a = graph.AddNode([] {});
  const auto b = graph.AddNode([] {});
  graph.Seal();
  EXPECT_THROW(graph.AddEdge(a, b), std::logic_error);
}

TEST(task_graph, should_throw_when_edge_from_unknown_node) {
  TaskGraph graph;
  const auto a = graph.AddNode([] {});
  EXPECT_THROW(graph.AddEdge(a + 1, a), std::out_of_range);
}

TEST(task_graph, should_throw_when_edge_to_unknown_node) {
  TaskGraph graph;
  const auto a = graph.AddNode([] {});
  EXPECT_THROW(graph.AddEdge(a, a + 1), std::out_of_range);
}

TEST(task_graph, should_throw_when_self_loop) {
  TaskGraph graph;
  const auto a = graph.AddNode([] {});
  EXPECT_THROW(graph.AddEdge(a, a), std::logic_error);
}

TEST(task_graph, should_throw_when_seal_twice) {
  TaskGraph graph;
  graph.AddNode([] {});
  graph.Seal();
  EXPECT_THROW(graph.Seal(), std::logic_error);
}

TEST(task_graph, should_throw_when_cycle_on_seal) {
  TaskGraph graph;
  const auto a = graph.AddNode([] {});
  const auto b = graph.AddNode([] {});
  graph.AddEdge(a, b);
  graph.AddEdge(b, a);
  EXPECT_THROW(graph.Seal(), std::logic_error);
  EXPECT_FALSE(graph.Sealed());
}

TEST(task_graph, should_remain_unsealed_when_disconnected_cycle) {
  TaskGraph graph;
  graph.AddNode([] {});
  const auto b = graph.AddNode([] {});
  const auto c = graph.AddNode([] {});
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
  const auto a = graph.AddNode([] {});
  EXPECT_THROW((void)graph.Task(a), std::logic_error);
}

TEST(task_graph, should_throw_when_successors_before_seal) {
  TaskGraph graph;
  const auto a = graph.AddNode([] {});
  EXPECT_THROW((void)graph.Successors(a), std::logic_error);
}

TEST(task_graph, should_throw_when_indegrees_before_seal) {
  TaskGraph graph;
  graph.AddNode([] {});
  EXPECT_THROW((void)graph.Indegrees(), std::logic_error);
}

TEST(task_graph, should_throw_when_task_unknown_node) {
  TaskGraph graph;
  graph.AddNode([] {});
  graph.Seal();
  EXPECT_THROW((void)graph.Task(1), std::out_of_range);
}

TEST(task_graph, should_throw_when_successors_unknown_node) {
  TaskGraph graph;
  graph.AddNode([] {});
  graph.Seal();
  EXPECT_THROW((void)graph.Successors(1), std::out_of_range);
}

TEST(task_graph, should_invoke_task_obtained_from_accessor) {
  TaskGraph graph;
  bool ran = false;
  const auto a = graph.AddNode([&] { ran = true; });
  graph.Seal();
  graph.Task(a)();
  EXPECT_TRUE(ran);
}

TEST(task_graph, should_expose_successors_after_seal) {
  TaskGraph graph;
  const auto a = graph.AddNode([] {});
  const auto b = graph.AddNode([] {});
  const auto c = graph.AddNode([] {});
  const auto d = graph.AddNode([] {});
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
  const auto a = graph.AddNode([] {});
  const auto b = graph.AddNode([] {});
  const auto c = graph.AddNode([] {});
  const auto d = graph.AddNode([] {});
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
