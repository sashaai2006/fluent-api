#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace exprflow::sync {

template <typename Task>
class BlockQueue {
 private:
  std::queue<Task> queue_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool open_{true};

 public:
  BlockQueue() = default;
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
};

template <typename Task>
BlockQueue<Task>::~BlockQueue() {
  Close();
}

template <typename Task>
template <typename U>
void BlockQueue<Task>::Push(U&& t) {
  {
    std::lock_guard guard(mutex_);
    if (!open_) {
      return;
    }
    queue_.push(std::forward<U>(t));
  }
  cv_.notify_one();
}

template <typename Task>
std::optional<Task> BlockQueue<Task>::Get() {
  std::unique_lock lock(mutex_);
  cv_.wait(lock, [&]() -> bool { return !(open_ && queue_.empty()); });
  if (queue_.empty()) {
    return std::nullopt;
  }
  auto t = std::move(queue_.front());
  queue_.pop();
  return t;
}

template <typename T>
bool BlockQueue<T>::Empty() {
  std::lock_guard lock(mutex_);
  return queue_.empty();
}

template <typename T>
size_t BlockQueue<T>::Size() {
  std::lock_guard lock(mutex_);
  return queue_.size();
}

template <typename Task>
void BlockQueue<Task>::Close() {
  std::lock_guard guard(mutex_);
  open_ = false;
  cv_.notify_all();
}

}  // namespace exprflow::sync
