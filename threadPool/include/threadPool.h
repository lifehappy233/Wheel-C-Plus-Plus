#include <thread>
#include <type_traits>
#include <future>
#include <queue>

namespace threadPool {

class threadPool {
 public:

  explicit threadPool(size_t pool_size);

  ~ threadPool();

  template<typename F, typename ...Args>
  auto push(F&& f, Args&& ...args)
      -> std::future<std::invoke_result_t<F, Args...>>;

 private:
  bool stop_{false};
  size_t pool_size_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::vector<std::thread> threads_;
  std::queue<std::function<void()>> tasks_;
};

template <typename F, typename... Args>
auto threadPool::push(F &&f, Args &&...args) -> std::future<std::invoke_result_t<F, Args...>> {
  using retType = std::invoke_result_t<F, Args...>;

  auto task = std::make_shared<std::packaged_task<retType()>>(std::bind(f, std::forward<Args>(args)...));
  auto res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(mutex_);
    tasks_.push([task = std::move(task)]() { (*task)(); });
    cv_.notify_one();
  }
  return res;
}

}
