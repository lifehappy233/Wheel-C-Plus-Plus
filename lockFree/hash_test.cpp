#include <map>
#include <mutex>
#include <ctime>
#include <random>
#include <iostream>
#include <unordered_map>

#include "lockFreeHashTable.h"

std::string generateRandomString() {
  const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const size_t maxLength = 20;

  std::random_device rd;
  std::mt19937 generator(rd());
  std::uniform_int_distribution<size_t> distribution(1, maxLength);

  size_t length = distribution(generator);

  std::uniform_int_distribution<size_t> charDistribution(0, characters.size() - 1);

  std::string randomString;
  for (size_t i = 0; i < length; ++i) {
    randomString += characters[charDistribution(generator)];
  }

  return randomString;
}

void InsertFindDeleteTest() {
  {
    auto hashTable = lockFree::LockFreeHashTable<std::string, std::string>();
    std::string key = "life", value = "happy";
    assert(hashTable.Insert(key, value));
    assert(key == "life");
    assert(value == "happy");
    std::string find_key = "life", find_value;
    assert(hashTable.Find(find_key, find_value));
    assert(find_value == "happy");
    assert(hashTable.Size() == 1);
  }
  {
    auto hashTable = lockFree::LockFreeHashTable<std::string, std::string>();
    std::string key = "life", value = "happy";
    assert(hashTable.Insert(key, std::move(value)));
    assert(key == "life");
    assert(value == "");
    std::string find_key = "life", find_value;
    assert(hashTable.Find(find_key, find_value));
    assert(find_value == "happy");
    assert(hashTable.Size() == 1);
  }
  {
    auto hashTable = lockFree::LockFreeHashTable<std::string, std::string>();
    std::string key = "life", value = "happy";
    assert(hashTable.Insert(std::move(key), value));
    assert(key == "");
    assert(value == "happy");
    std::string find_key = "life", find_value;
    assert(hashTable.Find(find_key, find_value));
    assert(find_value == "happy");
    assert(hashTable.Size() == 1);
  }
  {
    auto hashTable = lockFree::LockFreeHashTable<std::string, std::string>();
    std::string key = "life", value = "happy";
    assert(hashTable.Insert(std::move(key), std::move(value)));
    assert(key == "");
    assert(value == "");
    std::string find_key = "life", find_value;
    assert(hashTable.Find(find_key, find_value));
    assert(find_value == "happy");
    assert(hashTable.Size() == 1);
  }
  {
    auto hashTable = lockFree::LockFreeHashTable<std::string, std::string>();
    std::string key = "life", value = "happy";
    assert(hashTable.Insert(key, value));

    std::string find_key = "life", find_value;
    assert(hashTable.Find(find_key, find_value));
    assert(find_value == "happy");

    assert(hashTable.Size() == 1);
    std::string value1 = "life";
    assert(!hashTable.Insert(key, value1));

    assert(hashTable.Find(find_key, find_value));
    assert(find_value == "life");
  }

  {
    const int limit = 1000000;
    std::map<std::string, int> mp;
    auto hashTable = lockFree::LockFreeHashTable<std::string, std::string>();
    std::vector<std::pair<std::string, std::string>> insert;
    for (int i = 0; i < limit; i++) {
      std::string key;
      do {
        key = generateRandomString();
      } while (mp.count(key));
      mp[key] = 1;
      std::string value = generateRandomString();
      insert.emplace_back(key, value);
      // std::cout << "\n" << i << "\n";
      hashTable.Insert(key, value);
      std::string find_value;
      assert(hashTable.Find(key, find_value));
      assert(find_value == value);
      assert(hashTable.Size() == i + 1);
      // std::cout << "--------------------------\n";
    }
    size_t size = limit;
    for (int i = 0; i < limit; i++) {
      if (i & 1) {
        size--;
        // std::cout << "Delete: " << i << "\n";
        assert(hashTable.Delete(insert[i].first));
        assert(hashTable.Size() == size);
      }
    }

    for (int i = 0; i < limit; i++) {
      std::string value;
      auto result = hashTable.Find(insert[i].first, value);
      if (i & 1) {
        assert(!result);
      } else {
        assert(result);
        assert(value == insert[i].second);
      }
    }

  }
  std::cout << "========== Insert Find Delete Test ==========\n";
}

void Multi(size_t ins, size_t del, size_t fd) {
  std::cout << "Multi: " << " (" << ins << ", " << del << ", " << fd << ")\n";
  size_t base = ins * del * fd;
  size_t limit = 1000 / base * base;

  std::vector<std::thread> insert_threads;
  std::vector<std::thread> delete_threads;
  std::vector<std::thread> find_threads;

  std::map<std::string, int> mp;
  std::vector<std::pair<std::string, std::string>> pairs;
  for (int i = 0; i < limit; i++) {
    std::string key;
    do {
      key = generateRandomString();
    } while (mp.contains(key));
    mp[key] = 1;
    std::string value = generateRandomString();
    pairs.emplace_back(key, value);
  }
  auto hashTable = lockFree::LockFreeHashTable<std::string, std::string>();

  std::cout << "Begin ----------\n";
  for (int i = 0; i < ins; i++) {
    insert_threads.emplace_back([&hashTable, &pairs](int l, int r) {
      for (int i = l; i < r; i++) {
        if (i % 100 == 0) {
          std::cout << "Insert: " << i << "\n";
        }
        assert(hashTable.Insert(pairs[i].first, pairs[i].second));
      }
    }, limit / ins * i, limit / ins * (i + 1));
  }
  for (int i = 0; i < del; i++) {
    delete_threads.emplace_back([&hashTable, &pairs](int l, int r) {
      for (int i = l; i < r; i++) {
        if (i & 1) {
          continue ;
        }
        if (i % 100 == 0) {
          std::cout << "Delete: " << i << "\n";
        }
        while (!hashTable.Delete(pairs[i].first));
      }
    }, limit / del * i, limit / del * (i + 1));
  }
  for (int i = 0; i < fd; i++) {
    find_threads.emplace_back([&hashTable, &pairs](int l, int r) {
      std::string value;
      for (int i = l; i < r; i++) {
        if (i % 100 == 0) {
          std::cout << "Find: " << i << "\n";
        }
        auto ans = hashTable.Find(pairs[i].first, value);
        if (ans) {
          assert(value == pairs[i].second);
        }
      }
    }, limit / fd * i, limit / fd * (i + 1));
  }

  for (auto &it : insert_threads) {
    it.join();
  }
  for (auto &it : delete_threads) {
    it.join();
  }
  for (auto &it : find_threads) {
    it.join();
  }
}

int main() {
  std::srand(static_cast<unsigned int>(std::time(nullptr)));
//  InsertFindDeleteTest();
  // insert delete, find
  for (size_t ins = 1; ins <= 5; ins++) {
    for (size_t del = 1; del <= 5; del++) {
      for (size_t fd = 1; fd < 5; fd++) {
        Multi(ins, del, fd);
      }
    }
  }
  return 0;
}
