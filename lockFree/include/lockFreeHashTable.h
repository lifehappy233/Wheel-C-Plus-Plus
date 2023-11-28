#ifndef LOCK_FREE_HASH_TABLE_H_
#define LOCK_FREE_HASH_TABLE_H_

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

 private:
};

#endif
