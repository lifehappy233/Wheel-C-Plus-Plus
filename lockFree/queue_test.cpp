#include <iostream>
#include <string>
#include <cassert>

#include "reclaim.h"
#include "lockFreeQueue.h"
#include "block.h"

void basic_test() {
  lockFree::LockFreeQueue<std::string> string_q;
  for (int i = 0; i < 1000; i++) {
    std::string str = "lifehappy" + std::to_string(i);
    if (i & 1) {
      string_q.Push(str);
      assert(str == "lifehappy" + std::to_string(i));
    } else {
      string_q.Push(std::move(str));
    }
    assert(string_q.Size() == i + 1);
  }
  for (int i = 0; i < 1000; i++) {
    std::string str;
    while (!string_q.Pop(str));
    assert(str == "lifehappy" + std::to_string(i));
  }
  std::cout << lockFree::LockFreeQueue<std::string>::Global_Size() << "\n";
}

void only_one_to_one() {
  lockFree::LockFreeQueue<std::string> q;

  std::thread a([&q]() {
    for (int i = 0; i < 1000; i++) {
      std::string str = "lifehappy" + std::to_string(i);
      if (i & 1) {
        q.Push(str);
      } else {
        q.Push(std::move(str));
      }
    }
  });

  std::thread b([&q]() {
    std::string str;
    for (int i = 0; i < 1000; i++) {
      while (!q.Pop(str));

      assert(str == "lifehappy" + std::to_string(i));
    }
  });

  a.join();
  b.join();

  std::cout << lockFree::LockFreeQueue<std::string>::Global_Size() << "\n";
}

void five_to_one() {
  lockFree::LockFreeQueue<std::string> q;

  std::vector<std::thread> threads1;
  for (int i = 0; i < 5; i++) {
    threads1.emplace_back([&q]() {
      for (int i = 0; i < 1000; i++) {
        std::string str = "lifehappy" + std::to_string(i);
        if (i & 1) {
          q.Push(str);
        } else {
          q.Push(std::move(str));
        }
      }
    });
  }
  std::unordered_map<std::string, int> mp;
  auto b = std::thread([&q, &mp]() {
    std::string str;
    for (int i = 0; i < 5000; i++) {
      while (!q.Pop(str));
      mp[str]++;
    }
  });

  for (auto &it : threads1) {
    it.join();
  }
  b.join();

  for (auto &it : mp) {
    assert(it.second == 5);
  }
  assert(q.Size() == 0);
  std::cout << lockFree::LockFreeQueue<std::string>::Global_Size() << "\n";
}

void one_to_five() {
  lockFree::LockFreeQueue<std::string> q;

  std::thread a([&q]() {
    for (int i = 0; i < 5000; i++) {
      std::string str = "lifehappy" + std::to_string(i);
      if (i & 1) {
        q.Push(str);
      } else {
        q.Push(std::move(str));
      }
    }
  });

  std::vector<std::thread> threads;
  for (int i = 0; i < 5; i++) {
    threads.emplace_back([&q]() {
      std::string str;
      for (int i = 0; i < 1000; i++) {
        while (!q.Pop(str));
      }
    });
  }

  a.join();
  for (auto &it : threads) {
    it.join();
  }

  assert(q.Size() == 0);
  std::cout << lockFree::LockFreeQueue<std::string>::Global_Size() << "\n";
}

void ten_to_ten() {
  lockFree::LockFreeQueue<std::string> q;

  std::vector<std::thread> threads1;
  for (int i = 0; i < 10; i++) {
    threads1.emplace_back([&q]() {
      for (int i = 0; i < 1001; i++) {
        std::string str = "lifehappy" + std::to_string(i);
        if (i & 1) {
          q.Push(str);
        } else {
          q.Push(std::move(str));
        }
      }
    });
  }

  std::vector<std::thread> threads2;
  for (int i = 0; i < 10; i++) {
    threads2.emplace_back([&q]() {
      std::string str;
      for (int i = 0; i < 1000; i++) {
        while (!q.Pop(str));
      }
    });
  }

  for (auto &it : threads1) {
    it.join();
  }
  for (auto &it : threads2) {
    it.join();
  }

  assert(q.Size() == 10);
  std::cout << lockFree::LockFreeQueue<std::string>::Global_Size() << "\n";
}

void lock_free_queue(size_t producer, size_t consumer, int limit) {
  std::vector<std::thread> producers;
  std::vector<std::thread> consumers;

  lockFree::LockFreeQueue<int64_t> q;

  for (auto i = 0; i < producer; i++) {
    producers.emplace_back([&q, limit]() {
      for (int i = 0; i < limit; i++) {
        q.Push(114514);
      }
    });
  }

  int need = limit * static_cast<int>(producer) / static_cast<int>(consumer);

  for (auto i = 0; i < consumer; i++) {
    consumers.emplace_back([&q, limit = need]() {
      int64_t value;
      for (int i = 0; i < limit; i++) {
        while (!q.Pop(value));
      }
    });
  }

  for (auto &it : producers) {
    it.join();
  }
  for (auto &it : consumers) {
    it.join();
  }
}

void block_queue(size_t producer, size_t consumer, int limit) {
  std::vector<std::thread> producers;
  std::vector<std::thread> consumers;

  block::BlockQueue<int64_t> q;

  for (auto i = 0; i < producer; i++) {
    producers.emplace_back([&q, limit]() {
      for (int i = 0; i < limit; i++) {
        q.Push(114514);
      }
    });
  }

  int need = limit * static_cast<int>(producer) / static_cast<int>(consumer);
  for (auto i = 0; i < consumer; i++) {
    consumers.emplace_back([&q, limit = need]() {
      int64_t value;
      for (int i = 0; i < limit; i++) {
        while (!q.Pop(value));
      }
    });
  }

  for (auto &it : producers) {
    it.join();
  }
  for (auto &it : consumers) {
    it.join();
  }
}

void benchmark_test() {
  for (int i = 1; i <= 10; i++) {
    for (int j = 1; j <= 10; j++) {
      auto begin_lock_free = std::chrono::steady_clock::now();
      lock_free_queue(i, j, 1000000);
      auto end_lock_free = std::chrono::steady_clock::now();


      auto begin_block = std::chrono::steady_clock::now();
      block_queue(i, j, 1000000);
      auto end_block = std::chrono::steady_clock::now();

      printf("Producer(%2d), Consumer(%2d), LockFree(%5lld ms), Block(%5lld ms)\n", i, j,
             static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(end_lock_free - begin_lock_free).count()),
             static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(end_block - begin_block).count()));
    }
  }
}

int main() {
//  basic_test();
//  only_one_to_one();
//  five_to_one();
//  one_to_five();
//  ten_to_ten();

//  benchmark_test();

  auto begin_lock_free = std::chrono::steady_clock::now();
  lock_free_queue(10, 10, 1000000);
  auto end_lock_free = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end_lock_free - begin_lock_free).count() << "\n";
  return 0;
}
