#include <iostream>
#include <vector>

#include "threadPool.h"

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
