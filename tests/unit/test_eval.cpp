#include <exprflow/eval.hpp>
#include <exprflow/executor.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

TEST(eval, should_eval_then_join) {
  EXPECT_EQ(Eval(Value(2, 3).Then([](int a, int b) { return a + b; })), 5);
}

TEST(eval, should_eval_every_then_join) {
  EXPECT_DOUBLE_EQ(
      Eval(Value(2, 3)
               .Every([](int a, int b) { return std::pow((a + b) / 2.0, 2); },
                      [](int a, int b) { return std::pow((a - b) / 2.0, 2); },
                      [](int a, int b) { return std::max(a, b); },
                      [](int a, int b) { return std::min(a, b); })
               .Then([](double half_sum, double half_diff, int mx, int mn) {
                 return (half_sum + half_diff) * mx / mn;
               })),
      9.75);
}

TEST(eval, should_eval_if_then_else) {
  EXPECT_EQ(Eval(Value(4).IfThenElse([](int x) { return x > 0; },
                                     [](int x) { return x * 2; },
                                     [](int) { return 0; })),
            8);
}

TEST(eval, should_eval_map) {
  EXPECT_EQ(
      Eval(Value(std::vector<int>{1, 2, 3}).Map([](int x) { return x * 2.0; })),
      (std::vector<double>{2.0, 4.0, 6.0}));
}

TEST(eval, should_eval_fold) {
  EXPECT_EQ(Eval(Value(std::vector<int>{1, 2, 3})
                     .Fold([](int acc, int x) { return acc + x; }, 0)),
            6);
}

TEST(eval, should_eval_on_shared_executor) {
  Executor executor(2);
  EXPECT_EQ(
      Eval(Value(2, 3).Then([](int a, int b) { return a + b; }), executor), 5);
  EXPECT_EQ(
      Eval(Value(4, 6).Then([](int a, int b) { return a + b; }), executor), 10);
}
