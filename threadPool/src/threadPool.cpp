#include <thread>
#include <type_traits>
#include <future>
#include <queue>

#include "threadPool.h"

namespace threadPool {

threadPool::threadPool(size_t pool_size) : pool_size_(pool_size) {
  for (size_t i = 0; i < pool_size_; i++) {
    threads_.emplace_back([this]() {
      for (;;) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(mutex_);
          while (!stop_ && tasks_.empty()) {
            cv_.wait(lock);
          }
          if (stop_) {
            return;
          }
          task = std::move(tasks_.front());
          tasks_.pop();
        }
        task();
      }
    });
  }
}

threadPool::~threadPool() {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    stop_ = true;
  }
  cv_.notify_all();
  for (auto &it : threads_) {
    it.join();
  }
}

}
