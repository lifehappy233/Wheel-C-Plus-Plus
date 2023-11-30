#ifndef LOCK_FREE_HASH_TABLE_H_
#define LOCK_FREE_HASH_TABLE_H_

#include <atomic>

#include "reclaim.h"

namespace lockFree {

/*
 * bucket的索引方式类似页表的查询, 最多四层(level 0 ~ 3) 最后一层是 bucket, 所以hash值只有后24位有用
 * level 0 | level 1 | level 2 | level 3 |
 * segment | segment | segment | bucket  |
 */

constexpr size_t MaxSegLevel = 2;

constexpr size_t BucketLevel = 3;

constexpr size_t KSegMaxSize = 64;

constexpr double LoadFactor = 0.618;

constexpr size_t BucketMaxSize = 1 << 24;

#define HASH_LEVEL_INDEX(HASH, LEVEL) (((HASH) >> (((BucketLevel) - (LEVEL)) * 6)) & 0x3f)

static const uint8_t reverseTable[256] = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

size_t ReverseBit24(size_t num) {
  return (reverseTable[num & 0xFF] << 16) | (reverseTable[(num >> 8) & 0xFF] << 8) | reverseTable[(num >> 16) & 0xFF];
}

template <typename K, typename V>
class HashTableReclaimer;

template <typename K, typename V>
class LockFreeHashTable {
  struct Dummy;
  using Hash = std::hash<K>;
  using Bucket = std::atomic<Dummy*>;
 public:
  LockFreeHashTable() : hash_func_(Hash()), size_(0), bucket_size_(2) {
    auto *segments = segments_;
    // 0 | 1 | 2
    for (size_t level = 1; level <= MaxSegLevel; level++) {
      auto *new_segments = NewSegments(level);
      segments[0].data_.store(new_segments, std::memory_order_release);
      segments = new_segments;
    }
    // 当前持有第 2 层的segments
    auto *buckets = NewBuckets();
    segments[0].data_.store(buckets, std::memory_order_release);

    auto *head = new Dummy(0);
    buckets[0].store(head, std::memory_order_release);
    head_ = head;
  }

  ~ LockFreeHashTable() {
    // TODO
    // std::cout << "~ LockFreeHashTable()\n";
  }

  LockFreeHashTable(const LockFreeHashTable &other) = delete;
  LockFreeHashTable(LockFreeHashTable &&other) = delete;
  LockFreeHashTable &operator=(const LockFreeHashTable &other) = delete;
  LockFreeHashTable &operator=(LockFreeHashTable &&other) = delete;

  bool Insert(const K &key, const V &value) {
    auto *insert_node = new Regular(key, value, hash_func_(key));
    return InsertRegular(insert_node);
  }

  bool Insert(const K &key, V &&value) {
    auto *insert_node = new Regular(key, std::move(value), hash_func_(key));
    return InsertRegular(insert_node);
  }

  bool Insert(K &&key, const V &value) {
    auto *insert_node = new Regular(std::move(key), value, hash_func_(key));
    return InsertRegular(insert_node);
  }

  bool Insert(K &&key, V &&value) {
    auto *insert_node = new Regular(std::move(key), std::move(value), hash_func_(key));
    return InsertRegular(insert_node);
  }

  bool Find(const K &key, V &value);

  bool Delete(const K &key);

  size_t Size() { return size_.load(std::memory_order_acquire); }

 private:
  friend HashTableReclaimer<K, V>;

  struct Segment {
    Segment() : level_(0), data_(nullptr) {}

    explicit Segment(size_t level) : level_(level), data_(nullptr) {}

    ~ Segment() {
//      // std::cout << "OK\n";
      void *ptr = data_.load(std::memory_order_acquire);
      if (ptr == nullptr) {
        return;
      }
//      // std::cout << "OK\n";

//      // std::cout << "~ Segment() in\n";
      if (level_ < MaxSegLevel) {
//        // std::cout << "Segment in\n";
        auto *segment = static_cast<Segment *>(ptr);
        delete[] segment;
//        // std::cout << "Segment ou\n";
      } else {
//        // std::cout << "Bucket in\n";
        auto *buckets = static_cast<Bucket *>(ptr);
//        // std::cout << "Bucket ou\n";
        delete[] buckets;
      }
//      // std::cout << "~ Segment() ou\n";
    }

    Segment(const Segment &other) = delete;
    Segment(Segment &&other) = delete;
    Segment &operator=(const Segment &other) = delete;
    Segment &operator=(Segment &&other) = delete;

    size_t level_;
    std::atomic<void *> data_;
  };

  struct Node {
    Node(size_t hash, bool is_dummy)
        : hash_(hash), order_key_(is_dummy ? DummyKey(hash) : RegularKey(hash)) {}

    virtual ~ Node() = default;

    Node() = delete;
    Node(const Node &other) = delete;
    Node(Node &&other) = delete;
    Node& operator = (const Node &other) = delete;
    Node& operator = (Node &&other) = delete;

    size_t DummyKey(size_t hash) { return (ReverseBit24(hash) << 1); }

    size_t RegularKey(size_t hash) { return ((ReverseBit24(hash) << 1) | 1); }

    bool IsDummy() { return (order_key_ & 1) == 0; }

    const size_t hash_;
    const size_t order_key_;
    std::atomic<Node*> next_;
  };

  struct Regular : public Node {
    Regular(const K &key, const V &value, size_t hash)
        : Node(hash, false), key_(key), value_(new V(value)) {}
    Regular(const K &key, V &&value, size_t hash)
        : Node(hash, false), key_(key), value_(new V(std::move(value))) {}
    Regular(K &&key, const V &value, size_t hash)
        : Node(hash, false), key_(std::move(key)), value_(new V(value)) {}
    Regular(K &&key, V &&value, size_t hash)
        : Node(hash, false), key_(std::move(key)), value_(new V(std::move(value))) {}

    Regular(const K &key, size_t hash)
        : Node(hash, false), key_(key) {};

    ~ Regular() override {
      auto *value = value_.load(std::memory_order_acquire);
      delete value;
    }

    Regular() = delete;
    Regular(const Regular &other) = delete;
    Regular(Regular &&other) = delete;
    Regular& operator = (const Regular &other) = delete;
    Regular& operator = (Regular &&other) = delete;

    const K key_;
    std::atomic<V *> value_{nullptr};
  };

  struct Dummy : public Node {
    explicit Dummy(size_t index) : Node(index, true) {}
    ~ Dummy() override = default;

    Dummy() = delete;
    Dummy(const Dummy &other) = delete;
    Dummy(Dummy &&other) = delete;
    Dummy& operator = (const Dummy &other) = delete;
    Dummy& operator = (Dummy &&other) = delete;
  };

  size_t GetHash(const K &key) { return (hash_func_(key) & (0xffffff)); }

  size_t GetParentIndex(size_t index) { return (index & (~(1 << (31 - __builtin_clz(index))))); }

  static void DeleteNode(void* ptr) { delete static_cast<Node*>(ptr); }

  Node* Marked(Node *ptr) {
    return reinterpret_cast<Node*>(reinterpret_cast<uint64_t>(ptr) & (0x1));
  }

  Node* Unmarked(Node *ptr) {
    return reinterpret_cast<Node*>(reinterpret_cast<uint64_t>(ptr) & (~0x1));
  }

  bool IsMarked(Node *ptr) {
    return (reinterpret_cast<uint64_t>(ptr) & 0x1) == 0x1;
  }

  bool Less(Node *a, Node *b) {
    if (a->order_key_ != b->order_key_) {
      return a->order_key_ < b->order_key_;
    }
    if (a->IsDummy() || b->IsDummy()) {
      return false;
    }
    return static_cast<Regular*>(a)->key_ < static_cast<Regular*>(b)->key_;
  }

  bool GreaterEqual(Node* a, Node* b) {
    return !Less(a, b);
  }

  bool Equal(Node *a, Node *b) {
    return (!Less(a, b)) && (!Less(b, a));
  }

  Dummy* GetBucketByIndex(size_t index);

  Dummy* GetBucketByHash(size_t hash);

  Dummy* InitializeBucket(size_t index);

  Bucket* NewBuckets();

  Segment* NewSegments(int level);

  bool InsertDummy(Dummy *parent_head, Dummy *head, Dummy **maybe_head);

  bool InsertRegular(Regular *insert_node);

  bool SearchNode(Dummy* head, Node* search_node, Node** prev_ptr,
                  Node** cur_ptr, HazardPoint &prev_hp, HazardPoint &cur_hp);

  Hash hash_func_;

  Dummy *head_;

  std::atomic<size_t> size_;

  std::atomic<size_t> bucket_size_;

  Segment segments_[KSegMaxSize];

  static HazardList global_hp_list_;
};

template <typename K, typename V>
HazardList LockFreeHashTable<K, V>::global_hp_list_;

template <typename K, typename V>
class HashTableReclaimer : public Reclaimer {
 public:
  static HashTableReclaimer& GetInstance() {
    thread_local HashTableReclaimer queueReclaimer =
        HashTableReclaimer(LockFreeHashTable<K, V>::global_hp_list_);
    return queueReclaimer;
  }

  ~ HashTableReclaimer() override {
    // std::cout << std::this_thread::get_id() << " ~ HashTableReclaimer()\n";
  }

  HashTableReclaimer() = delete;
  HashTableReclaimer(const HashTableReclaimer &other) = delete;
  HashTableReclaimer(HashTableReclaimer &&other) = delete;
  HashTableReclaimer& operator = (const HashTableReclaimer &other) = delete;
  HashTableReclaimer& operator = (HashTableReclaimer &&other) = delete;
 private:
  explicit HashTableReclaimer(HazardList &global_hp_list) : Reclaimer(global_hp_list) { }
};

template <typename K, typename V>
bool LockFreeHashTable<K, V>::InsertRegular(Regular *insert_node) {
  Node *prev;
  Node *cur;
  HazardPoint prev_hp;
  HazardPoint cur_hp;
  // std::cout << "InsertRegular 0\n";
  auto *head = GetBucketByHash(insert_node->hash_);
  do {
    // std::cout << "InsertRegular 1\n";
    if (SearchNode(head, insert_node, &prev, &cur, prev_hp, cur_hp)) {
//      // std::cout << "InsertRegular 2\n";
      V* new_value = insert_node->value_.load(std::memory_order_consume);
      V* old_value = static_cast<Regular*>(cur)->value_.exchange(new_value, std::memory_order_release);
      insert_node->value_.store(nullptr, std::memory_order_release);

      delete insert_node;
      delete old_value;
      return false;
    }
    // std::cout << "InsertRegular 3\n";
    insert_node->next_.store(cur, std::memory_order_release);
//    // std::cout << "InsertRegular 4\n";
  } while (!prev->next_.compare_exchange_strong(cur, insert_node, std::memory_order_acq_rel));
//  // std::cout << "InsertRegular 5\n";
  size_.fetch_add(1, std::memory_order_acq_rel);
//  // std::cout << "InsertRegular 6\n";
  auto bucket_size = bucket_size_.load(std::memory_order_acquire);
  auto cur_size = static_cast<double>(size_.load(std::memory_order_acquire));
  // std::cout << cur_size * LoadFactor << " " << size_t(cur_size * LoadFactor) << " " << bucket_size << "\n";
  if (bucket_size != BucketMaxSize && size_t(cur_size * LoadFactor) > bucket_size) {
    // std::cout << "bucket_size: " << bucket_size + bucket_size << "\n";
    bucket_size_.compare_exchange_strong(bucket_size, bucket_size + bucket_size, std::memory_order_acq_rel);
  }
//  // std::cout << "InsertRegular 7\n";
  return true;
}

template <typename K, typename V>
bool LockFreeHashTable<K, V>::Find(const K &key, V &value) {
  Regular find_node(key, hash_func_(key));
  auto *head = GetBucketByHash(find_node.hash_);

  Node *prev;
  Node *cur;
  HazardPoint prev_hp;
  HazardPoint cur_hp;
  if (!SearchNode(head, &find_node, &prev, &cur, prev_hp, cur_hp)) {
    return false;
  }
  value = *static_cast<Regular*>(cur)->value_.load(std::memory_order_acquire);
  return true;
}

template <typename K, typename V>
bool LockFreeHashTable<K, V>::Delete(const K &key) {
  // std::cout << "Delete begin ------------\n";
  Regular delete_node(key, hash_func_(key));
  // std::cout << "Delete pre GetBucketByHash\n";
  auto *head = GetBucketByHash(delete_node.hash_);
  // std::cout << "Delete end GetBucketByHash\n";

  Node *prev;
  Node *cur;
  Node *next;
  HazardPoint prev_hp;
  HazardPoint cur_hp;
  do {
    do {
      // std::cout << "Delete pre SearchNode\n";
      if (!SearchNode(head, &delete_node, &prev, &cur, prev_hp, cur_hp)) {
        // std::cout << "Not Find\n";
        return false;
      }
      // std::cout << "Delete suf SearchNode\n";
      next = cur->next_.load(std::memory_order_acquire);
    } while (IsMarked(next));
  } while (!cur->next_.compare_exchange_strong(next, Marked(next), std::memory_order_acq_rel));
  // 标记为逻辑删除了，说明该节点被当前线程所删除
  // std::cout << "Find and delete\n";
  size_.fetch_sub(1, std::memory_order_acq_rel);
  if (prev->next_.compare_exchange_strong(cur, next, std::memory_order_acq_rel)) {
    auto& hashTableReclaimer = HashTableReclaimer<K, V>::GetInstance();
    hashTableReclaimer.ReclaimLater(cur, DeleteNode);
    hashTableReclaimer.ReclaimNoHazard();
  } else {
    prev_hp.Unmark();
    cur_hp.Unmark();
    // 再尝试删除一次
    SearchNode(head, &delete_node, &prev, &cur, prev_hp, cur_hp);
  }
  return true;
}

template <typename K, typename V>
LockFreeHashTable<K, V>::Bucket* LockFreeHashTable<K, V>::NewBuckets()  {
  auto *buckets = new Bucket[KSegMaxSize];
  return buckets;
}

template <typename K, typename V>
LockFreeHashTable<K, V>::Segment* LockFreeHashTable<K, V>::NewSegments(int level) {
  auto *segments = new Segment[KSegMaxSize];
  for (size_t i = 0; i < KSegMaxSize; i++) {
    segments[i].level_ = level;
  }
  return segments;
}

template <typename K, typename V>
LockFreeHashTable<K, V>::Dummy* LockFreeHashTable<K, V>::GetBucketByIndex(size_t index) {
  auto *segments = segments_;
  for (size_t level = 1; level <= MaxSegLevel; level++) {
    size_t id = HASH_LEVEL_INDEX(index, level - 1);
//    // std::cout << id << " level(" << level - 1 << ") ";
    segments = static_cast<Segment*>(segments[id].data_.load(std::memory_order_acquire));
    if (segments == nullptr) {
//      // std::cout << "\n";
      return nullptr;
    }
  }
  size_t id = HASH_LEVEL_INDEX(index, MaxSegLevel);
//  // std::cout << id << " level(" << MaxSegLevel << ") ";
  auto *buckets = static_cast<Bucket*>(segments[id].data_.load(std::memory_order_acquire));
  if (buckets == nullptr) {
//    // std::cout << "\n";
    return nullptr;
  }
  id = HASH_LEVEL_INDEX(index, BucketLevel);
//  // std::cout << id << " level(" << BucketLevel << ") ";
  auto bucket = buckets[id].load(std::memory_order_acquire);
//  // std::cout << "\n";
//  // std::cout << (bucket == nullptr) << "\n";
  return bucket;
}

template <typename K, typename V>
LockFreeHashTable<K, V>::Dummy* LockFreeHashTable<K, V>::GetBucketByHash(size_t hash) {
  auto bucket_size = bucket_size_.load(std::memory_order_acquire);
  auto index = hash & (bucket_size - 1);
  // std::cout << index << " :GetBucketByHash\n";
  auto *head = GetBucketByIndex(index);
  // std::cout << index << " :~GetBucketByHash\n";
  if (head == nullptr) {
    head = InitializeBucket(index);
  }
  // std::cout << index << " :GetBucketByHash end\n";
  return head;
}

template <typename K, typename V>
LockFreeHashTable<K, V>::Dummy* LockFreeHashTable<K, V>::InitializeBucket(size_t index) {
  auto parent_index = GetParentIndex(index);
  // std::cout << "parent index: " << parent_index << "\n";
  auto *parent_head = GetBucketByIndex(parent_index);
  if (parent_head == nullptr) {
    // std::cout << "Parent null\n";
    parent_head = InitializeBucket(parent_index);
  }

  // std::cout << index << " InitializeBucket\n";
  auto *segments = segments_;
  for (size_t level = 1; level <= MaxSegLevel; level++) {
    size_t id = HASH_LEVEL_INDEX(index, level - 1);
    // std::cout << id << " level(" << level - 1 << ", ";
    auto &segment = segments[id];
    auto *sub_segments = static_cast<Segment *>(segment.data_.load(std::memory_order_acquire));

    if (sub_segments == nullptr) {
      // std::cout << "nul) ";
      sub_segments = NewSegments(level);
      void *expect = nullptr;
      if (!segment.data_.compare_exchange_strong(expect, sub_segments, std::memory_order_acq_rel)) {
        delete [] sub_segments;
        sub_segments = static_cast<Segment *>(expect);
      }
    } else {
      // std::cout << "hav) ";
    }
    segments = sub_segments;
  }
  size_t id = HASH_LEVEL_INDEX(index, MaxSegLevel);
  auto &segment = segments[id];
  // std::cout << id << " level(" << MaxSegLevel << ", ";
  auto *buckets = static_cast<Bucket*>(segment.data_.load(std::memory_order_acquire));
  if (buckets == nullptr) {
    // std::cout << "nul) ";
    buckets = NewBuckets();
    void *expect = nullptr;
    if (!segment.data_.compare_exchange_strong(expect, buckets, std::memory_order_acq_rel)) {
      delete []buckets;
      buckets = static_cast<Bucket*>(expect);
    }
  } else {
    // std::cout << "hav) ";
  }
  id = HASH_LEVEL_INDEX(index, BucketLevel);
  auto &bucket = buckets[id];

  // std::cout << id << " level(" << BucketLevel << ", ";

  auto head = bucket.load(std::memory_order_acquire);
  if (head == nullptr) {
    // std::cout << "nul)\n";
    head = new Dummy(index);
    Dummy *maybe_head;
    if (InsertDummy(parent_head, head, &maybe_head)) {
      bucket.store(head, std::memory_order_release);
    } else {
      delete head;
      head = maybe_head;
    }
  } else {
    // std::cout << "hav)\n";
  }
  // std::cout << "InitializeBucket end\n";
  return head;
}

template <typename K, typename V>
bool LockFreeHashTable<K, V>::SearchNode(Dummy *head, Node *search_node, Node **prev_ptr,
                                         Node **cur_ptr, HazardPoint &prev_hp, HazardPoint &cur_hp) {
  auto &hashTableReclaimer = HashTableReclaimer<K, V>::GetInstance();

try_again:
  Node *prev = head;
  Node *cur = prev->next_.load(std::memory_order_acquire);
  Node *next;
  while (true) {
    cur_hp.Unmark();
    cur_hp = HazardPoint(&hashTableReclaimer, cur);
    if (prev->next_.load(std::memory_order_acquire) != cur) {
      // std::cout << "again 1\n";
      goto try_again;
    }
    if (cur == nullptr) {
      *prev_ptr = prev;
      *cur_ptr = cur;
      return false;
    }
    next = cur->next_.load(std::memory_order_acquire);
    // prev -> cur -> next，如果next被标记，则表示 cur 已经被逻辑删除
    // 下面的逻辑需要保证 prev -> cur -> next 一定是连续的
    if (IsMarked(next)) {
      // 尝试将当前物理删除
      if (!prev->next_.compare_exchange_strong(cur, Unmarked(next), std::memory_order_acq_rel)) {
        goto try_again;
      }
      // prev.next = cur 则可以保证 prev -> cur -> next 连续, 因为 next 是提前获取的
      hashTableReclaimer.ReclaimLater(cur, DeleteNode);
      hashTableReclaimer.ReclaimNoHazard();
      cur = Unmarked(next);
    } else {
      if (prev->next_.load(std::memory_order_acquire) != cur) {
        goto try_again;
      }
      // 连续原因同上
      if (GreaterEqual(cur, search_node)) {
        *prev_ptr = prev;
        *cur_ptr = cur;
        return Equal(cur, search_node);
      }
      HazardPoint tmp = std::move(cur_hp);
      cur_hp = std::move(prev_hp);
      prev_hp = std::move(tmp);

      prev = cur;
      cur = next;
    }
  }
}

template <typename K, typename V>
bool LockFreeHashTable<K, V>::InsertDummy(Dummy *parent_head, Dummy *head, Dummy **maybe_head) {
  Node *prev;
  Node *cur;
  HazardPoint prev_hp;
  HazardPoint cur_hp;
  do {
    // std::cout << "InsertDummy loop, order key: " << parent_head->order_key_ << ", " << head->order_key_ << "\n";
    if (SearchNode(parent_head, head, &prev, &cur, prev_hp, cur_hp)) {
      *maybe_head = static_cast<Dummy*>(cur);
      return false;
    }
    head->next_.store(cur, std::memory_order_release);
  } while (!prev->next_.compare_exchange_strong(cur, head, std::memory_order_acq_rel));
  return true;
}

}

#endif
