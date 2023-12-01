#include <map>
#include <mutex>
#include <ctime>
#include <random>
#include <iostream>
#include <algorithm>
#include <unordered_map>

#include "lockFreeHashTable.h"
#include "block.h"

std::random_device rd;

std::string generateRandomString() {
  const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const size_t maxLength = 20;

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

void MultiInsert(size_t ins) {
  std::cout << "MultiInsert: " << ins << " ----------\n";
  int limit = 1000000, repeat = 1;
  std::vector<std::pair<std::string, std::string>> units;
  std::map<std::string, int> mp;
  for (int i = 0; i < limit; i++) {
    std::string key;
    do {
      key = generateRandomString();
    } while (mp.count(key));
    mp[key] = 1;
    std::string value = generateRandomString();
    units.emplace_back(key, value);
  }

  std::vector<std::pair<std::string, std::string>> inserts;
  for (int rep = 0; rep < repeat; rep++) {
    for (int i = 0; i < limit; i++) {
      inserts.emplace_back(units[i]);
    }
  }
  std::shuffle(inserts.begin(), inserts.end(), std::default_random_engine(rd()));

  std::vector<std::thread> insert_threads;
  auto hashTable = lockFree::LockFreeHashTable<std::string, std::string>();

  std::atomic<int> success[ins];
  for (int i = 0; i < ins; i++) {
    success[i].store(0);
  }

  for (int i = 0; i < ins; i++) {
    std::shuffle(inserts.begin(), inserts.end(), std::default_random_engine(rd()));
    insert_threads.emplace_back([&hashTable, inserts, &success, limit, repeat](int id) {
      int total = 0;
      for (int i = 0; i < limit * repeat; i++) {
        total += hashTable.Insert(inserts[i].first, inserts[i].second);
        if (i % 1000 == 999) {
          std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
      }
      success[id].fetch_add(total);
    }, i);
  }
  for (auto &it : insert_threads) {
    it.join();
  }
  int total = 0;
  for (int i = 0; i < ins; i++) {
    std::cout << success[i].load() << " \n"[i + 1 == ins];
    total += success[i].load();
  }
  assert(total == limit);

  for (int i = 0; i < limit; i++) {
    std::string value;
    auto res = hashTable.Find(units[i].first, value);
    assert(res);
    assert(value == units[i].second);
  }
}

void Multi(size_t ins, size_t del, size_t fd) {
  std::cout << "Multi: " << " (" << ins << ", " << del << ", " << fd << ")\n";
  size_t base = std::max(ins, 1ul) * std::max(del, 1ul) * std::max(fd, 1ul);
  size_t limit = 1000000 / base * base;
  std::cout << limit << "\n";
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
  // rejudge key unique
  mp.clear();
  for (const auto& it : pairs) {
    mp[it.first]++;
  }
  for (const auto& it : mp) {
    assert(it.second == 1);
  }

  auto hashTable = lockFree::LockFreeHashTable<std::string, std::string>();

  std::cout << "Begin ----------\n";
//  for (const auto &it : pairs) {
//    std::cout << it.first << "\n";
//  }
//  std::cout << "All Key --------\n";
  for (int i = 0; i < ins; i++) {
    insert_threads.emplace_back([&hashTable, &pairs](int l, int r) {
      for (int i = l; i < r; i++) {
//        if (i % 100 == 0) {
//          std::cout << "Insert: " << i << "\n";
//        }
        assert(hashTable.Insert(pairs[i].first, pairs[i].second));
      }
    }, limit / ins * i, limit / ins * (i + 1));
  }
  for (int i = 0; i < del; i++) {
    delete_threads.emplace_back([&hashTable, &pairs, ins, del, fd](int l, int r) {
      for (int i = l; i < r; i++) {
        if (i & 1) {
          continue ;
        }
        while (!hashTable.Delete(pairs[i].first)) {
          std::string output = std::to_string(ins) + " " + std::to_string(del)
                               + " " + std::to_string(fd) + " " + pairs[i].first + "\n";
//          std::cout << output;
//          std::cout << "Delete\n";
        }
      }
    }, limit / del * i, limit / del * (i + 1));
  }
  for (int i = 0; i < fd; i++) {
    find_threads.emplace_back([&hashTable, &pairs](int l, int r) {
      std::string value;
      for (int i = l; i < r; i++) {
//        if (i % 100 == 0) {
//          std::cout << "Find: " << i << "\n";
//        }
        while (!hashTable.Find(pairs[i].first, value));
        assert(value == pairs[i].second);
      }
    }, limit / fd * i, limit / fd * (i + 1));
  }

//  auto print_thread = std::thread([&hashTable]() {
//    std::this_thread::sleep_for(std::chrono::seconds(2));
//    hashTable.DebugPrint();
//  });

  for (auto &it : insert_threads) {
    it.join();
  }
  assert(hashTable.Size() == limit);
  std::cout << "Insert OK\n";
  for (auto &it : delete_threads) {
    it.join();
  }
  for (auto &it : find_threads) {
    it.join();
  }

  std::cout << "ALLDONE\n";
  //  print_thread.join();
}

void MultiInsertBenchmarkLockFree(size_t ins, size_t limit,
                                  const std::vector<std::pair<std::string, std::string>> &inserts) {
  limit = limit / ins * ins;

  std::vector<std::thread> insert_threads;
  auto hashTable = lockFree::LockFreeHashTable<std::string, std::string>();

  for (int i = 0; i < ins; i++) {
    insert_threads.emplace_back([&hashTable, &inserts](int l, int r) {
      int total = 0;
      for (int i = l; i < r; i++) {
        total += hashTable.Insert(inserts[i].first, inserts[i].second);
      }
    }, limit / ins * i, limit / ins * (i + 1));
  }
  for (auto &it : insert_threads) {
    it.join();
  }
  assert(hashTable.Size() == limit);
}

void MultiInsertBenchmarkBlock(size_t ins, size_t limit,
                               const std::vector<std::pair<std::string, std::string>> &inserts) {
  limit = limit / ins * ins;

  std::vector<std::thread> insert_threads;
  auto hashTable = block::BlockHashMap<std::string, std::string>();

  for (int i = 0; i < ins; i++) {
    insert_threads.emplace_back([&hashTable, &inserts](int l, int r) {
      int total = 0;
      for (int i = l; i < r; i++) {
        total += hashTable.Insert(inserts[i].first, inserts[i].second);
      }
    }, limit / ins * i, limit / ins * (i + 1));
  }
  for (auto &it : insert_threads) {
    it.join();
  }
  assert(hashTable.Size() == limit);
}

int main() {
  std::srand(static_cast<unsigned int>(std::time(nullptr)));
//  InsertFindDeleteTest();
//  for (size_t ins = 1; ins <= 8; ins++) {
//    MultiInsert(ins);
//  }
//
//  size_t limit = 10000000;
//  std::vector<std::pair<std::string, std::string>> inserts;
//  std::map<std::string, int> mp;
//  for (size_t i = 0; i < limit; i++) {
//    std::string key;
//    do {
//      key = generateRandomString();
//    } while (mp.count(key));
//    mp[key] = 1;
//    std::string value = generateRandomString();
//    inserts.emplace_back(key, value);
//  }
//
//  for (size_t ins = 1; ins <= 20; ins++) {
//    printf("Thread(%2lu), ", ins);
//    auto begin_lock_free = std::chrono::steady_clock::now();
//    MultiInsertBenchmarkLockFree(ins, limit, inserts);
//    auto end_lock_free = std::chrono::steady_clock::now();
//
//    auto begin_block = std::chrono::steady_clock::now();
//    MultiInsertBenchmarkBlock(ins, limit, inserts);
//    auto end_block = std::chrono::steady_clock::now();
//
//    printf("LockFree(%5lld ms), Block(%5lld ms)\n",
//           static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(end_lock_free - begin_lock_free).count()),
//           static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(end_block - begin_block).count()));
//  }
  // insert delete, find
  for (size_t ins = 1; ins <= 10; ins++) {
    for (size_t del = 1; del <= 10; del++) {
      for (size_t fd = 1; fd <= 10; fd++) {
        Multi(ins, del, fd);
      }
    }
  }
//  Multi(1, 3, 2);
  return 0;
}
