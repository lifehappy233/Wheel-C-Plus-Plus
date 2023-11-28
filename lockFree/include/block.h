#ifndef BLOCK_H_
#define BLOCK_H_

#include <mutex>
#include <queue>
#include <stack>
#include <unordered_map>

namespace block {

template<typename T>
class BlockQueue {
 public:
  BlockQueue() = default;
  ~ BlockQueue() = default;

  BlockQueue(const BlockQueue &other) = delete;
  BlockQueue(BlockQueue &&other) = delete;
  BlockQueue& operator = (const BlockQueue &other) = delete;
  BlockQueue& operator = (BlockQueue &&other) = delete;

  void Push(const T &data) { Emplace(data); }

  void Push(T &&data) { Emplace(std::move(data)); }

  bool Pop(T &data) {
    std::unique_lock lock(mu_);
    if (q_.empty()) {
      return false;
    }
    data = q_.front();
    q_.pop();
    return true;
  }

 private:
  template<typename Arg>
  void Emplace(Arg &&arg) {
    std::unique_lock lock(mu_);
    q_.push(std::forward<Arg>(arg));
  }

  std::mutex mu_;
  std::queue<T> q_;
};

template<typename T>
class BlockStack {
 public:
  BlockStack() = default;
  ~ BlockStack() = default;

  BlockStack(const BlockStack &other) = delete;
  BlockStack(BlockStack &&other) = delete;
  BlockStack& operator = (const BlockStack &other) = delete;
  BlockStack& operator = (BlockStack &&other) = delete;

  void Push(const T &data) { Emplace(data); }

  void Push(T &&data) { Emplace(std::move(data)); }

  bool Pop(T &data) {
    std::unique_lock lock(mu_);
    if (q_.empty()) {
      return false;
    }
    data = q_.front();
    q_.pop();
    return true;
  }

 private:
  template<typename Arg>
  void Emplace(Arg &&arg) {
    std::unique_lock lock(mu_);
    q_.push(std::forward<Arg>(arg));
  }

  std::mutex mu_;
  std::queue<T> q_;
};

template<typename K, typename V>
class BlockHashMap {
 public:
  BlockHashMap() = default;
  ~ BlockHashMap() = default;

  BlockHashMap(const BlockHashMap &other) = delete;
  BlockHashMap(BlockHashMap &&other) = delete;
  BlockHashMap& operator = (const BlockHashMap &other) = delete;
  BlockHashMap& operator = (BlockHashMap &&other) = delete;

  bool Insert(const K &key, const V &value) { return Emplace(key, value); }

  bool Insert(const K &key, V &&value) { return Emplace(key, std::move(value)); }

  bool Insert(K &&key, const V &value) { return Emplace(std::move(key), value); }

  bool Insert(K &&key, V &&value) { return Emplace(std::move(key), std::move(value)); }

  bool Find(const K &key, V &value);

  bool Delete(const K &key);

  size_t Size() {
    auto lock = std::unique_lock(mutex_);
    return size_;
  }
 private:

  template<typename ArgK, typename ArgV>
  bool Emplace(ArgK &&key, ArgV &&value);

  bool InnerFind(const K &key) {
    return map_.count(key);
  }

  size_t size_;
  std::mutex mutex_;
  std::unordered_map<K, V> map_;
};

template<typename K, typename V>
template<typename ArgK, typename ArgV>
bool BlockHashMap<K, V>::Emplace(ArgK &&key, ArgV &&value) {
  auto lock = std::unique_lock<std::mutex>(mutex_);
  if (InnerFind(key)) {
    return false;
  }
  map_[std::forward<ArgK>(key)] = std::forward<ArgV>(value);
  size_++;
  return true;
}

template<typename K, typename V>
bool BlockHashMap<K, V>::Find(const K &key, V &value) {
  auto lock = std::unique_lock<std::mutex>(mutex_);
  if (InnerFind(key)) {
    value = map_[key];
    return true;
  }
  return false;
}

template<typename K, typename V>
bool BlockHashMap<K, V>::Delete(const K &key) {
  auto lock = std::unique_lock<std::mutex>(mutex_);
  if (InnerFind(key)) {
    map_.erase(key);
    size_--;
    return true;
  }
  return false;
}

}

#endif
