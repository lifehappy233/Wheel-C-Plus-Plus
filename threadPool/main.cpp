#include <iostream>
#include <vector>
#include <thread>
#include <type_traits>
#include <future>
#include <queue>

namespace threadPool {

class threadPool {
 public:

  threadPool(size_t pool_size) : pool_size_(pool_size) {
    for (size_t i = 0; i < pool_size_; i++) {
      threads_.emplace_back([this](){
        for (;;) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(mutex_);
            while (!stop_ && tasks_.empty()) {
              cv_.wait(lock);
            }
            if (stop_) {
              return ;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
          }
          task();
        }
      });
    }
  }

  ~ threadPool() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      stop_ = true;
    }
    cv_.notify_all();
    for (auto& it : threads_) {
      it.join();
    }
  }

  template<typename F, typename ...Args>
  auto push(F&& f, Args&& ...args)
      -> std::future<std::invoke_result_t<F, Args...>> {
    using retType = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<retType()>>(
        std::bind(f, std::forward<Args>(args)...));
    auto res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(mutex_);
      tasks_.push([task = std::move(task)]() {
        (*task)();
      });
      cv_.notify_one();
    }
    return res;
  }

 private:
  bool stop_{false};
  size_t pool_size_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::vector<std::thread> threads_;
  std::queue<std::function<void()>> tasks_;
};

}

int calc_primes(int n) {
  int cnt = 0;
  std::vector<int> vis(n, 0), prime(n);
  for (int i = 2; i <= n; i++) {
    if (!vis[i]) {
      prime[++cnt] = i;
    }
    for (int j = 1; j <= cnt && i * prime[j] <= n; j++) {
      vis[i * prime[j]] = 1;
      if (i % prime[j] == 0) {
        break;
      }
    }
  }
  return cnt;
}

int main() {
  auto pool = threadPool::threadPool(std::thread::hardware_concurrency());
  std::vector<std::future<int>> results;
  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < 10000; i++) {
    results.emplace_back(pool.push(calc_primes, 100000));
  }
  long long sum = 0;
  for (auto &it : results) {
    sum += it.get();
  }
  auto end = std::chrono::steady_clock::now();
  auto time1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << time1.count() << " " << sum << "\n";
  return 0;
}
