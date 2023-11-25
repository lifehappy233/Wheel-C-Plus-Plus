#ifndef BLOCK_H_
#define BLOCK_H_

#include <mutex>
#include <queue>

namespace block {

template<typename T>
class BlockQueue {
 public:
  BlockQueue() = default;
  ~ BlockQueue() = default;

  BlockQueue(const BlockQueue &other) = delete;
  BlockQueue(BlockQueue &&other) = delete;
  BlockQueue& operator = (const BlockQueue &other) = delete;
  BlockQueue& operator = (BlockQueue &&other) = delete;

  void Push(const T &data) { Emplace(data); }

  void Push(T &&data) { Emplace(std::move(data)); }

  bool Pop(T &data) {
    std::unique_lock lock(mu_);
    if (q_.empty()) {
      return false;
    }
    data = q_.front();
    q_.pop();
    return true;
  }

 private:
  template<typename Arg>
  void Emplace(Arg &&arg) {
    std::unique_lock lock(mu_);
    q_.push(std::forward<Arg>(arg));
  }

  std::mutex mu_;
  std::queue<T> q_;
};

}

#endif
