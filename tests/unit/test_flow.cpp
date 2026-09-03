#include <exprflow/flow/flow.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <tuple>
#include <type_traits>
#include <vector>

using namespace exprflow;

TEST(flow, should_wrap_value_outputs) {
  auto flow = Value(2, 3);
  static_assert(std::is_same_v<decltype(flow)::Outputs, std::tuple<int, int>>);
}

TEST(flow, should_then_join_value_outputs) {
  auto flow = Value(2, 3).Then([](int a, int b) { return a + b; });
  static_assert(std::is_same_v<decltype(flow)::Outputs, std::tuple<int>>);
}

TEST(flow, should_every_keep_branch_outputs) {
  auto flow =
      Value(2, 3).Every([](int a, int b) { return std::pow((a + b) / 2.0, 2); },
                        [](int a, int b) { return std::pow((a - b) / 2.0, 2); },
                        [](int a, int b) { return std::max(a, b); },
                        [](int a, int b) { return std::min(a, b); });
  static_assert(std::is_same_v<decltype(flow)::Outputs,
                               std::tuple<double, double, int, int>>);
}

TEST(flow, should_then_after_every_join_branches) {
  auto flow =
      Value(2, 3)
          .Every([](int a, int b) { return std::pow((a + b) / 2.0, 2); },
                 [](int a, int b) { return std::pow((a - b) / 2.0, 2); },
                 [](int a, int b) { return std::max(a, b); },
                 [](int a, int b) { return std::min(a, b); })
          .Then([](double half_sum, double half_diff, int mx, int mn) {
            return (half_sum + half_diff) * mx / mn;
          });
  static_assert(std::is_same_v<decltype(flow)::Outputs, std::tuple<double>>);
}

TEST(flow, should_if_then_else_unify_branch_types) {
  auto flow =
      Value(4).IfThenElse([](int x) { return x > 0; },
                          [](int x) { return x * 2; }, [](int) { return 0; });
  static_assert(std::is_same_v<decltype(flow)::Outputs, std::tuple<int>>);
}

TEST(flow, should_map_range_elements) {
  auto flow =
      Value(std::vector<int>{1, 2, 3}).Map([](int x) { return x * 2.0; });
  static_assert(
      std::is_same_v<decltype(flow)::Outputs, std::tuple<std::vector<double>>>);
}

TEST(flow, should_fold_range_with_seed) {
  auto flow = Value(std::vector<int>{1, 2, 3})
                  .Fold([](int acc, int x) { return acc + x; }, 0);
  static_assert(std::is_same_v<decltype(flow)::Outputs, std::tuple<int>>);
}
