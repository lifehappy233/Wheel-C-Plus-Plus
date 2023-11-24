#include <iostream>
#include <string>
#include <cassert>

#include "reclaim.h"
#include "lockFreeQueue.h"

void basic_test() {
  lockFree::LockFreeQueue<std::string> string_q;
  for (int i = 0; i < 1000; i++) {
    std::string str = "lifehappy" + std::to_string(i);
    if (i & 1) {
      string_q.Push(str);
      assert(str == "lifehappy" + std::to_string(i));
    } else {
      string_q.Push(std::move(str));
      assert(str == "");
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

void five_to_five() {
  lockFree::LockFreeQueue<std::string> q;

  std::vector<std::thread> threads1;
  for (int i = 0; i < 5; i++) {
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
  for (int i = 0; i < 5; i++) {
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

  assert(q.Size() == 5);
  while (q.Size() != 0) {
    std::string str;
    while (!q.Pop(str));
    std::cout << str << "\n";
  }
  std::cout << lockFree::LockFreeQueue<std::string>::Global_Size() << "\n";
}

int main() {
  basic_test();
  only_one_to_one();
  five_to_one();
  one_to_five();
  five_to_five();
//  lockFree::LockFreeQueue<std::string> q;
//
////  q.Push("lifehappy");
//
//  std::thread a([&q]() {
//    for (int i = 0; i < 1000; i++) {
//      std::string str = "lifehappy";
//      if (i & 1) {
//        q.Push(str);
//      } else {
//        q.Push(std::move(str));
//      }
//    }
//  });
//
//  std::thread b([&q]() {
//    std::string str;
//    for (int i = 0; i < 1000; i++) {
//      while (!q.Pop(str));
//    }
//  });
//
//  a.join();
//  b.join();
//
//  std::cout << q.Size() << "\n";
  return 0;
}
