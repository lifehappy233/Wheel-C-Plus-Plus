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

#define HASH_LEVEL_INDEX(HASH, LEVEL) (((HASH) >> ((BucketLevel - LEVEL) * 6)) & 0x3f)

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
class LockFreeHashTable {
  struct Dummy;
  using Hash = std::hash<K>;
  using Bucket = std::atomic<Dummy*>;
 public:
  LockFreeHashTable() : hash_func_(Hash()), size_(0), bucket_size_(0) {
    auto *segments = segments_;
    for (size_t level = 1; level <= MaxSegLevel; level++) {
      auto *new_segments = NewSegments(level);
      segments[0].data_.store(new_segments, std::memory_order_release);
      segments = new_segments;
    }
    auto *buckets = NewBuckets();
    segments[0].data_.store(buckets, std::memory_order_release);

    auto *head = new Dummy(0);
    buckets[0].store(head, std::memory_order_release);
    head_ = head;
  }

  ~LockFreeHashTable() = default;

  LockFreeHashTable(const LockFreeHashTable &other) = delete;
  LockFreeHashTable(LockFreeHashTable &&other) = delete;
  LockFreeHashTable &operator=(const LockFreeHashTable &other) = delete;
  LockFreeHashTable &operator=(LockFreeHashTable &&other) = delete;

  bool Insert(const K &key, const V &value) { return Emplace(key, value); }

  bool Insert(const K &key, V &&value) { return Emplace(key, std::move(value)); }

  bool Insert(K &&key, const V &value) { return Emplace(std::move(key), value); }

  bool Insert(K &&key, V &&value) { return Emplace(std::move(key), std::move(value)); }

  bool Find(const K &key, V &value);

  bool Delete(const K &key);

  size_t Size() { return size_.load(std::memory_order_acquire); }

 private:

  template <typename ArgK, typename ArgV>
  bool Emplace(ArgK &&key, ArgV &&value);

  size_t GetHash(const K &key) {
    return (hash_func_(key) & (0xffffff));
  }

  size_t GetParentIndex(size_t index) {
    size_t high = 31 - __builtin_clz(index);
    return (index & (~(1 << high)));
  }

  struct Segment {
    Segment() : level_(0), data_(nullptr) {}

    explicit Segment(size_t level) : level_(level), data_(nullptr) {}

    ~Segment() {
      void *ptr = data_.load(std::memory_order_acquire);
      if (ptr == nullptr) {
        return;
      }

      if (level_ <= MaxSegLevel) {
        auto *segment = static_cast<Segment *>(ptr);
        delete[] segment;
      } else {
        auto *buckets = static_cast<Bucket *>(ptr);
        delete[] buckets;
      }
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

    const size_t hash_;
    const size_t order_key_;
    std::atomic<Node*> next_;
  };

  template <typename ArgK, typename ArgV>
  struct Regular : public Node {
    Regular(ArgK &&key, ArgV &&value)
        : Node(GetHash(key), false), key_(std::forward<ArgK>(key)),
          value_(new V(std::forward<ArgV>(value)), std::memory_order_release) {}

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
    std::atomic<V *> value_;
  };

  struct Dummy : public Node {
    Dummy(size_t index) : Node(index, true) {}
    ~ Dummy() override = default;

    Dummy() = delete;
    Dummy(const Dummy &other) = delete;
    Dummy(Dummy &&other) = delete;
    Dummy& operator = (const Dummy &other) = delete;
    Dummy& operator = (Dummy &&other) = delete;
  };

  Dummy* GetBucketByIndex(size_t index);

  Dummy* GetBucketByHash(size_t hash);

  Dummy* InitializeBucket(size_t index);

  Bucket* NewBuckets();

  Segment* NewSegments(int level);

  bool InsertDummy(Dummy *parent, Dummy *dummy, Dummy **head);

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
    std::cout << std::this_thread::get_id() << " ~ HashTableReclaimer()\n";
  }

  HashTableReclaimer() = delete;
  HashTableReclaimer(const HashTableReclaimer &other) = delete;
  HashTableReclaimer(HashTableReclaimer &&other) = delete;
  HashTableReclaimer& operator = (const HashTableReclaimer &other) = delete;
  HashTableReclaimer& operator = (HashTableReclaimer &&other) = delete;
 private:
  explicit HashTableReclaimer(HazardList &global_hp_list) : Reclaimer(global_hp_list) {
    // std::cout << std::this_thread::get_id() << " HashTableReclaimer()\n";
  }
};

template <typename K, typename V>
template <typename ArgK, typename ArgV>
bool LockFreeHashTable<K, V>::Emplace(ArgK &&key, ArgV &&value) {
  return true;
}

template <typename K, typename V>
bool LockFreeHashTable<K, V>::Find(const K &key, V &value) {
  return true;
}

template <typename K, typename V>
bool LockFreeHashTable<K, V>::Delete(const K &key) {
  return true;
}

template <typename K, typename V>
LockFreeHashTable<K, V>::Bucket* LockFreeHashTable<K, V>::NewBuckets()  {
  auto *buckets = new Bucket[KSegMaxSize];
  for (size_t i = 0; i < KSegMaxSize; i++) {
    buckets[i].store(nullptr, std::memory_order_release);
  }
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
  Bucket *buckets;
  Dummy *bucket;
  for (size_t level = 0; level <= MaxSegLevel; level++) {
    size_t id = HASH_LEVEL_INDEX(index, level);
    if (level != MaxSegLevel) { // 0 -> 1 -> 2 共三层加载两次
      segments = static_cast<Segment *>(segments_[id].data_.load(std::memory_order_acquire));
      if (segments == nullptr) {
        return nullptr;
      }
    } else {
      buckets = static_cast<Bucket *>(segments[id].data_.load(std::memory_order_acquire));
      if (buckets == nullptr) {
        return nullptr;
      }
      id = HASH_LEVEL_INDEX(index, BucketLevel);
      bucket =  buckets[id].load(std::memory_order_acquire);
    }
  }
  return bucket;
}

template <typename K, typename V>
LockFreeHashTable<K, V>::Dummy* LockFreeHashTable<K, V>::GetBucketByHash(size_t hash) {
  auto bucket_size = bucket_size_.load(std::memory_order_acquire);
  auto index = hash & (bucket_size - 1);
  auto *head = GetBucketByIndex(index);
  if (head == nullptr) {

  }
  return head;
}

template <typename K, typename V>
LockFreeHashTable<K, V>::Dummy* LockFreeHashTable<K, V>::InitializeBucket(size_t index) {
  size_t parent_index = GetParentIndex(index);
  Dummy *parent_head = GetBucketByIndex(parent_index);
  if (parent_head == nullptr) {
    parent_head = InitializeBucket(parent_index);
  }

  auto *segments = segments_;
  Bucket *buckets;
  for (size_t level = 0; level <= MaxSegLevel; level++) {
    size_t id = HASH_LEVEL_INDEX(index, level);
    Segment &segment = segments[id];
    if (level != MaxSegLevel) {
      auto *sub_segments = static_cast<Segment*>(segment.data_.load(std::memory_order_acquire));
      if (sub_segments == nullptr) {
        sub_segments = NewSegments(level + 1);
        void *expect = nullptr;
        if (!segment.data_.compare_exchange_strong(expect, sub_segments, std::memory_order_acq_rel)) {
          delete []sub_segments;
          sub_segments = static_cast<Segment*>(expect);
        }
      }
      segments = sub_segments;
    } else {
      buckets = static_cast<Bucket*>(segment.data_.load(std::memory_order_acquire));
      if (buckets == nullptr) {
        buckets = NewBuckets();
        void *expect = nullptr;
        if (!segment.data_.compare_exchange_strong(expect, buckets, std::memory_order_acq_rel)) {
          delete []buckets;
          buckets = static_cast<Bucket*>(expect);
        }
      }
    }
  }
  size_t id = HASH_LEVEL_INDEX(index, BucketLevel);
  Bucket &bucket = buckets[id];

  auto head = bucket.load(std::memory_order_acquire);
  if (head == nullptr) {
    head = new Dummy(index);
    Dummy *maybe_head;
    if (InsertDummy(parent_head, head, &maybe_head)) {
      bucket.store(head, std::memory_order_release);
    } else {
      delete head;
      head = maybe_head;
    }
  }
  return head;
}

template <typename K, typename V>
bool LockFreeHashTable<K, V>::SearchNode(Dummy *head, Node *search_node, Node **prev_ptr,
                                         Node **cur_ptr, HazardPoint &prev_hp, HazardPoint &cur_hp) {

}

template <typename K, typename V>
bool LockFreeHashTable<K, V>::InsertDummy(Dummy *parent_head, Dummy *head, Dummy **maybe_head) {
  // TODO:
  if (parent_head & 1) {
    return false;
  }
  return true;
}

}

#endif
