#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <thread>
#include <utility>

namespace exprflow::sync {

// Lock-free блокирующая очередь MPMC
template <typename Task>
class BlockQueue {
 private:
  struct Node {
    Node() = default;
    template <typename U>
    explicit Node(U&& value) : value(std::in_place, std::forward<U>(value)) {}

    std::optional<Task> value;
    std::atomic<Node*> next{nullptr};
    Node* retired_next{nullptr};
  };

  static constexpr std::int64_t kClosedBit = std::int64_t{1} << 62;
  static constexpr std::int64_t kCountMask = ~kClosedBit;

  alignas(64) std::atomic<Node*> head_;
  alignas(64) std::atomic<Node*> tail_;
  alignas(64) std::atomic<std::int64_t> count_{0};
  std::atomic<std::int64_t> waiters_{0};
  std::atomic<Node*> garbage_{nullptr};

 public:
  BlockQueue() {
    Node* dummy = new Node();
    head_.store(dummy, std::memory_order_relaxed);
    tail_.store(dummy, std::memory_order_relaxed);
  }
  BlockQueue(const BlockQueue&) = delete;
  BlockQueue& operator=(const BlockQueue&) = delete;
  BlockQueue(BlockQueue&&) = delete;
  BlockQueue& operator=(BlockQueue&&) = delete;
  ~BlockQueue();

  template <typename U>
  void Push(U&& t);
  std::optional<Task> Get();
  bool Empty();
  size_t Size();
  void Close();

 private:
  Task PopClaimed();
  void Retire(Node* node);
};

template <typename Task>
BlockQueue<Task>::~BlockQueue() {
  Close();
  Node* node = garbage_.load(std::memory_order_relaxed);
  while (node != nullptr) {
    Node* next = node->retired_next;
    delete node;
    node = next;
  }
  node = head_.load(std::memory_order_relaxed);
  while (node != nullptr) {
    Node* next = node->next.load(std::memory_order_relaxed);
    delete node;
    node = next;
  }
}

template <typename Task>
template <typename U>
void BlockQueue<Task>::Push(U&& t) {
  if ((count_.load(std::memory_order_acquire) & kClosedBit) != 0) {
    return;
  }
  Node* node = new Node(std::forward<U>(t));
  Node* prev_tail = tail_.exchange(node, std::memory_order_acq_rel);
  prev_tail->next.store(node, std::memory_order_release);
  count_.fetch_add(1, std::memory_order_acq_rel);
  if (waiters_.load(std::memory_order_acquire) > 0) {
    count_.notify_one();
  }
}

template <typename Task>
std::optional<Task> BlockQueue<Task>::Get() {
  for (;;) {
    std::int64_t observed = count_.load(std::memory_order_acquire);
    const std::int64_t available = observed & kCountMask;
    if (available > 0) {
      if (count_.compare_exchange_weak(observed, observed - 1,
                                       std::memory_order_acq_rel,
                                       std::memory_order_acquire)) {
        return PopClaimed();
      }
      continue;
    }
    if ((observed & kClosedBit) != 0) {
      return std::nullopt;
    }
    // Отмечаемся как ждущие ДО wait: если Push проскочит между load
    // счётчика и fetch_add здесь, wait просто не уснёт — значение
    // уже другое, будильник не теряется.
    waiters_.fetch_add(1, std::memory_order_release);
    count_.wait(observed, std::memory_order_acquire);
    waiters_.fetch_sub(1, std::memory_order_release);
  }
}

template <typename Task>
Task BlockQueue<Task>::PopClaimed() {
  for (;;) {
    Node* head = head_.load(std::memory_order_acquire);
    Node* next = head->next.load(std::memory_order_acquire);
    if (next == nullptr) {
      std::this_thread::yield();
      continue;
    }
    if (head_.compare_exchange_weak(head, next, std::memory_order_acq_rel,
                                    std::memory_order_acquire)) {
      Task value = std::move(*next->value);
      Retire(head);
      return value;
    }
  }
}

template <typename Task>
void BlockQueue<Task>::Retire(Node* node) {
  node->retired_next = garbage_.load(std::memory_order_relaxed);
  while (!garbage_.compare_exchange_weak(node->retired_next, node,
                                         std::memory_order_release,
                                         std::memory_order_relaxed)) {
  }
}

template <typename Task>
bool BlockQueue<Task>::Empty() {
  return Size() == 0;
}

template <typename Task>
size_t BlockQueue<Task>::Size() {
  return static_cast<size_t>(count_.load(std::memory_order_acquire) &
                             kCountMask);
}

template <typename Task>
void BlockQueue<Task>::Close() {
  count_.fetch_or(kClosedBit, std::memory_order_acq_rel);
  count_.notify_all();
}

}  // namespace exprflow::sync
