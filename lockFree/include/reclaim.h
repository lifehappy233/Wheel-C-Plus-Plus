#ifndef RECLAIM_H_
#define RECLAIM_H_

#include <vector>
#include <atomic>
#include <thread>
#include <cassert>
#include <iostream>
#include <functional>
#include <unordered_map>

namespace lockFree {

/*
 * HazardPoint 和 HazardList 需要满足多线程安全
 */

struct InternalHazardPoint {
  InternalHazardPoint() : ptr_(nullptr), flag_(false), next_(nullptr) {}
  ~ InternalHazardPoint() = default;

  InternalHazardPoint(const InternalHazardPoint &other) = delete;
  InternalHazardPoint(InternalHazardPoint &&other) = delete;
  InternalHazardPoint& operator = (const InternalHazardPoint &other) = delete;
  InternalHazardPoint& operator = (InternalHazardPoint &&other) = delete;

  std::atomic_flag flag_;
  std::atomic<void*> ptr_;
  std::atomic<InternalHazardPoint*> next_;
};

struct HazardList {
  HazardList() : size_(1), head_(new InternalHazardPoint()) {
    // std::cout << std::this_thread::get_id() << " HazardList()\n";
  }

  ~ HazardList() {
//    int size = 0;
    auto *p = head_.load(std::memory_order_acquire);
    while (p) {
//      size++;
      auto temp = p;
      p = p->next_.load(std::memory_order_acquire);
      delete temp;
    }
//    std::cout << size << " okk\n";
  }

  int32_t Size() {
    return size_.load(std::memory_order_relaxed);
  }

  HazardList(const HazardList &other) = delete;
  HazardList(HazardList &&other) = delete;
  HazardList& operator = (const HazardList &other) = delete;
  HazardList& operator = (HazardList &&other) = delete;

  std::atomic<int32_t> size_;
  std::atomic<InternalHazardPoint*> head_;
};

/*
 * ReclaimPoint 和 ReclaimPool 只需要满足单线程安全即可
 */

struct ReclaimPoint {
  ReclaimPoint() : ptr_(nullptr), next_(nullptr), delete_function_(nullptr) {}
  ~ ReclaimPoint() = default;

  ReclaimPoint(const ReclaimPoint &other) = delete;
  ReclaimPoint(ReclaimPoint &&other) = delete;
  ReclaimPoint& operator = (const ReclaimPoint &other) = delete;
  ReclaimPoint& operator = (ReclaimPoint &&other) = delete;

  void *ptr_;
  ReclaimPoint *next_;
  std::function<void(void*)> delete_function_;
};

struct ReclaimPool {
  ReclaimPool() : head_(new ReclaimPoint()) {}

  ~ ReclaimPool() {
    auto p = head_;
    while (p) {
      auto temp = p;
      p = p->next_;
      delete temp;
    }
  }

  void Push(ReclaimPoint *p) {
    p->next_ = head_;
    head_ = p;
  }

  ReclaimPoint* Pop() {
    if (head_->next_ == nullptr) {
      return new ReclaimPoint();
    }
    auto ans = head_;
    head_ = head_->next_;
    ans->next_ = nullptr;
    return ans;
  }

  ReclaimPoint *head_;
};

class Reclaimer {
 public:
  explicit Reclaimer(HazardList &global_hp_list) : global_hp_list_(global_hp_list) {
    // std::cout << std::this_thread::get_id() << " Reclaimer()\n";
  }

  virtual ~ Reclaimer() {
    // std::cout << std::this_thread::get_id() << " ~ Reclaimer()\n";
    // 将当前持有的 local hazard point 归还到 global hazard list
    for (auto it : local_hp_list_) {
      assert(it->ptr_.load(std::memory_order_acquire) == nullptr);
      it->flag_.clear(std::memory_order_release);
    }
    // 删除 待删除节点
    for (auto it = reclaim_map_.begin(); it != reclaim_map_.end();) {
      while (Hazard(it->second->ptr_)) {
        std::this_thread::yield();
      }

      it->second->delete_function_(it->second->ptr_);
      delete it->second;
      it = reclaim_map_.erase(it);
    }
  }

  int32_t MarkHazard(void *ptr);

  void UnmarkHazard(int32_t index);

  void ReclaimLater(void *ptr, std::function<void(void*)> &&delete_function);

  void ReclaimNoHazard();

  Reclaimer() = delete;
  Reclaimer(const Reclaimer &other) = delete;
  Reclaimer(Reclaimer &&other) = delete;
  Reclaimer& operator = (const Reclaimer &other) = delete;
  Reclaimer& operator = (Reclaimer &&other) = delete;

 private:

  bool Hazard(void *ptr);

  void RequireHazardPoint();

  std::vector<InternalHazardPoint*> local_hp_list_;
  HazardList &global_hp_list_;

  ReclaimPool reclaim_pool_;
  std::unordered_map<void*, ReclaimPoint*> reclaim_map_;

  static const int32_t rate_ = 4;
};

class HazardPoint {
 public:
  HazardPoint() : index_(-1), reclaimer_(nullptr) {}

  HazardPoint(Reclaimer *reclaimer, void *ptr)
      : reclaimer_(reclaimer), index_(reclaimer->MarkHazard(ptr)) {}

  ~ HazardPoint() {
    Unmark();
  }

  int32_t index() {
    return index_;
  }

  void Unmark() {
    if (reclaimer_ != nullptr && index_ != -1) {
      reclaimer_->UnmarkHazard(index_);
    }
  }

  HazardPoint(const HazardPoint &other) = delete;
  HazardPoint& operator = (const HazardPoint &other) = delete;

  HazardPoint(HazardPoint &&other)  noexcept {
    *this = std::move(other);
  }

  HazardPoint& operator = (HazardPoint &&other)  noexcept {
    this->reclaimer_ = other.reclaimer_;
    this->index_ = other.index_;
    other.index_ = -1;
    return *this;
  }

 private:
  int32_t index_{};
  Reclaimer *reclaimer_{};
};

}

#endif
