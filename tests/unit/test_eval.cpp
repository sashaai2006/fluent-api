#include <exprflow/eval.hpp>
#include <exprflow/executor.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <numeric>
#include <vector>

using namespace exprflow;

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

TEST(eval, should_eval_bare_value_through_leaf_reference) {
  // Выход графа — сам лист: слот хранит ссылку, а не копию.
  EXPECT_EQ(Eval(Value(42)), 42);
}

TEST(eval, should_map_large_range_in_chunks_preserving_order) {
  constexpr int kSize = 100'000;
  std::vector<int> input(kSize);
  std::iota(input.begin(), input.end(), 0);

  Executor executor(4);
  const auto mapped = Eval(
      Value(std::move(input)).Map([](int x) { return x * 2 + 1; }), executor);

  ASSERT_EQ(mapped.size(), static_cast<std::size_t>(kSize));
  for (int i = 0; i < kSize; ++i) {
    ASSERT_EQ(mapped[i], i * 2 + 1) << "index " << i;
  }
}

TEST(eval, should_fold_in_chunks_with_non_identity_seed) {
  // seed = 100: последовательный fold даёт 100 + сумма; параллельный
  // обязан дать то же значение (seed не размножается по чанкам).
  constexpr int kSize = 50'000;
  std::vector<int> input(kSize, 1);

  Executor executor(4);
  const int folded = Eval(
      Value(std::move(input)).Fold([](int acc, int x) { return acc + x; }, 100),
      executor);
  EXPECT_EQ(folded, 100 + kSize);
}

TEST(eval, should_fold_empty_range_to_seed) {
  const int folded = Eval(Value(std::vector<int>{})
                              .Fold([](int acc, int x) { return acc + x; }, 7));
  EXPECT_EQ(folded, 7);
}

TEST(eval, should_execute_map_chunks_concurrently) {
  constexpr int kSize = 200'000;
  std::vector<int> input(kSize, 1);

  std::atomic<int> in_flight{0};
  std::atomic<int> max_in_flight{0};

  Executor executor(4);
  (void)Eval(Value(std::move(input))
                 .Map([&](int x) {
                   const int now = in_flight.fetch_add(1) + 1;
                   int prev = max_in_flight.load();
                   while (now > prev &&
                          !max_in_flight.compare_exchange_weak(prev, now)) {
                   }
                   // Короткая занятость, чтобы чанки точно пересеклись.
                   volatile double sink = x;
                   for (int i = 0; i < 100; ++i) {
                     sink = sink * 1.0000001 + 1.0;
                   }
                   in_flight.fetch_sub(1);
                   return x;
                 })
                 .Fold([](int acc, int x) { return acc + x; }, 0),
             executor);

  EXPECT_GT(max_in_flight.load(), 1);
}
