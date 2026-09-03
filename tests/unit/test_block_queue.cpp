#include <exprflow/sync/block_queue.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <numeric>
#include <optional>
#include <thread>
#include <vector>

namespace {

constexpr int kItemsPerProducer = 5000;
constexpr std::size_t kProducers = 4;
constexpr std::size_t kConsumers = 4;
constexpr int kTotalItems = kItemsPerProducer * static_cast<int>(kProducers);

}  // namespace

TEST(block_queue, should_keep_fifo_order_single_thread) {
  exprflow::sync::BlockQueue<int> queue;
  for (int i = 0; i < 100; ++i) {
    queue.Push(i);
  }
  for (int i = 0; i < 100; ++i) {
    auto value = queue.Get();
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, i);
  }
}

TEST(block_queue, should_block_until_push) {
  exprflow::sync::BlockQueue<int> queue;
  std::atomic<bool> pushed{false};
  std::jthread consumer([&] {
    auto value = queue.Get();
    EXPECT_TRUE(value.has_value());
    EXPECT_TRUE(pushed.load());
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  pushed.store(true);
  queue.Push(7);
}

TEST(block_queue, should_drain_items_before_nullopt_after_close) {
  exprflow::sync::BlockQueue<int> queue;
  queue.Push(1);
  queue.Push(2);
  queue.Close();
  ASSERT_EQ(queue.Get(), 1);
  ASSERT_EQ(queue.Get(), 2);
  EXPECT_EQ(queue.Get(), std::nullopt);
}

TEST(block_queue, should_drop_push_after_close) {
  exprflow::sync::BlockQueue<int> queue;
  queue.Close();
  queue.Push(42);
  EXPECT_EQ(queue.Get(), std::nullopt);
  EXPECT_TRUE(queue.Empty());
  EXPECT_EQ(queue.Size(), 0u);
}

TEST(block_queue, should_wake_all_waiters_on_close) {
  exprflow::sync::BlockQueue<int> queue;
  std::atomic<int> finished{0};
  std::vector<std::jthread> waiters;
  for (std::size_t i = 0; i < kConsumers; ++i) {
    waiters.emplace_back([&] {
      if (!queue.Get().has_value()) {
        finished.fetch_add(1);
      }
    });
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  queue.Close();
  for (auto& waiter : waiters) {
    waiter.join();
  }
  EXPECT_EQ(finished.load(), static_cast<int>(kConsumers));
}

TEST(block_queue, should_deliver_every_item_exactly_once_mpmc) {
  exprflow::sync::BlockQueue<int> queue;
  std::vector<std::atomic<int>> seen(kTotalItems);
  std::atomic<int> consumed{0};

  std::vector<std::jthread> producers;
  for (std::size_t p = 0; p < kProducers; ++p) {
    producers.emplace_back([&, p] {
      const int base = static_cast<int>(p) * kItemsPerProducer;
      for (int i = 0; i < kItemsPerProducer; ++i) {
        queue.Push(base + i);
      }
    });
  }

  std::vector<std::jthread> consumers;
  for (std::size_t c = 0; c < kConsumers; ++c) {
    consumers.emplace_back([&] {
      while (auto value = queue.Get()) {
        seen[*value].fetch_add(1);
        consumed.fetch_add(1);
      }
    });
  }

  for (auto& producer : producers) {
    producer.join();
  }
  queue.Close();
  for (auto& consumer : consumers) {
    consumer.join();
  }

  EXPECT_EQ(consumed.load(), kTotalItems);
  for (int i = 0; i < kTotalItems; ++i) {
    EXPECT_EQ(seen[i].load(), 1) << "item " << i;
  }
}
