#include <exprflow/executor.hpp>
#include <exprflow/flow/compiler.hpp>
#include <exprflow/flow/flow.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <any>
#include <cmath>
#include <vector>

namespace {

constexpr std::size_t kTwoWorkers = 2;

auto RunCompiled(Compiled& compiled) {
  Executor executor(kTwoWorkers);
  return executor.Submit(compiled.graph).get();
}

template <typename T>
T ResultAt(const std::vector<std::any>& results,
           const Compiled& compiled,
           std::size_t index = 0) {
  return std::any_cast<T>(results[compiled.outputs[index]]);
}

}  // namespace

TEST(compiler, should_emit_one_source_per_value) {
  auto flow = Value(2, 3);
  auto compiled = Compile(flow);
  ASSERT_EQ(compiled.graph->Size(), 2u);
  ASSERT_EQ(compiled.outputs.size(), 2u);
  EXPECT_EQ(compiled.graph->Indegrees()[compiled.outputs[0]], 0u);
  EXPECT_EQ(compiled.graph->Indegrees()[compiled.outputs[1]], 0u);
}

TEST(compiler, should_run_then_join) {
  auto flow = Value(2, 3).Then([](int a, int b) { return a + b; });
  auto compiled = Compile(flow);
  ASSERT_EQ(compiled.graph->Size(), 3u);
  ASSERT_EQ(compiled.outputs.size(), 1u);
  EXPECT_EQ(compiled.graph->Indegrees()[compiled.outputs[0]], 2u);

  const auto results = RunCompiled(compiled);
  EXPECT_EQ(ResultAt<int>(results, compiled), 5);
}

TEST(compiler, should_run_every_then_join) {
  auto flow =
      Value(2, 3)
          .Every([](int a, int b) { return std::pow((a + b) / 2.0, 2); },
                 [](int a, int b) { return std::pow((a - b) / 2.0, 2); },
                 [](int a, int b) { return std::max(a, b); },
                 [](int a, int b) { return std::min(a, b); })
          .Then([](double half_sum, double half_diff, int mx, int mn) {
            return (half_sum + half_diff) * mx / mn;
          });
  auto compiled = Compile(flow);
  EXPECT_EQ(compiled.graph->Size(), 7u);

  const auto results = RunCompiled(compiled);
  EXPECT_DOUBLE_EQ(ResultAt<double>(results, compiled), 9.75);
}

TEST(compiler, should_run_if_then_else) {
  auto flow = Value(4).IfThenElse([](int x) { return x > 0; },
                                  [](int x) { return x * 2; },
                                  [](int) { return 0; });
  auto compiled = Compile(flow);
  const auto results = RunCompiled(compiled);
  EXPECT_EQ(ResultAt<int>(results, compiled), 8);
}

TEST(compiler, should_run_map) {
  auto flow = Value(std::vector<int>{1, 2, 3}).Map([](int x) { return x * 2.0; });
  auto compiled = Compile(flow);
  const auto results = RunCompiled(compiled);
  EXPECT_EQ((ResultAt<std::vector<double>>(results, compiled)),
            (std::vector<double>{2.0, 4.0, 6.0}));
}

TEST(compiler, should_run_fold) {
  auto flow = Value(std::vector<int>{1, 2, 3}).Fold(
      [](int acc, int x) { return acc + x; }, 0);
  auto compiled = Compile(flow);
  const auto results = RunCompiled(compiled);
  EXPECT_EQ(ResultAt<int>(results, compiled), 6);
}
