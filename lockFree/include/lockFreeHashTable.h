#ifndef LOCK_FREE_HASH_TABLE_H_
#define LOCK_FREE_HASH_TABLE_H_

#include <atomic>

#include "reclaim.h"

template<typename K, typename V>
class LockFreeHashTable {
 public:
  LockFreeHashTable() = default;
  ~ LockFreeHashTable() = default;

  LockFreeHashTable(const LockFreeHashTable &other) = delete;
  LockFreeHashTable(LockFreeHashTable &&other) = delete;
  LockFreeHashTable& operator = (const LockFreeHashTable &other) = delete;
  LockFreeHashTable& operator = (LockFreeHashTable &&other) = delete;

  bool Insert(const K &key, const V &value) { return Emplace(key, value); }

  bool Insert(const K &key, V &&value) { return Emplace(key, std::move(value)); }

  bool Insert(K &&key, const V &value) { return Emplace(std::move(key), value); }

  bool Insert(K &&key, V &&value) { return Emplace(std::move(key), std::move(value)); }

  bool Find(const K &key, V &value);

  bool Delete(const K &key);

  size_t Size() { return size_.load(std::memory_order_acquire); }

 private:

  template<typename ArgK, typename ArgV>
  bool Emplace(ArgK &&key, ArgV &&value);

  std::atomic<size_t> size_;
};

template<typename K, typename V>
template<typename ArgK, typename ArgV>
bool LockFreeHashTable<K, V>::Emplace(ArgK &&key, ArgV &&value) {

}

template<typename K, typename V>
bool LockFreeHashTable<K, V>::Find(const K &key, V &value) {

}

template<typename K, typename V>
bool LockFreeHashTable<K, V>::Delete(const K &key) {

}

#endif
