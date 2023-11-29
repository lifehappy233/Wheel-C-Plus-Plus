#ifndef LOCKFREESTACK_H_
#define LOCKFREESTACK_H_

#include <cassert>

#include "reclaim.h"

namespace lockFree {

template<typename T>
class StackReclaimer;

template<typename T>
class LockFreeStack {
 public:
  LockFreeStack()
      : front_(new Data()), tail_(front_.load(std::memory_order_acquire)), size_(0) {}

  ~ LockFreeStack() {
    auto *p = front_.load(std::memory_order_acquire);
    while (p) {
      auto temp = p;
      p = p->next_.load(std::memory_order_acquire);
      delete temp;
    }
  }

  void Push(const T &data) { Emplace(data); }

  void Push(T &&data) { Emplace(std::move(data)); }

  bool Pop(T &data);

  size_t Size() { return size_.load(std::memory_order_acquire); }

  static size_t Global_Size() {
    return global_hp_list_.Size();
  }

  LockFreeStack(const LockFreeStack &other) = delete;
  LockFreeStack(LockFreeStack &&other) = delete;
  LockFreeStack& operator = (const LockFreeStack &other) = delete;
  LockFreeStack& operator = (LockFreeStack &&other) = delete;

 private:
  friend StackReclaimer<T>;

  struct Data {
    Data() : data_(nullptr), next_(nullptr) {}
    ~ Data() {
      auto *data = data_.load(std::memory_order_acquire);
      delete data;
    }

    Data(const Data &other) = delete;
    Data(Data &&other) = delete;
    Data& operator = (const Data &other) = delete;
    Data& operator = (Data &&other) = delete;

    std::atomic<T*> data_;
    std::atomic<Data*> next_;
  };

  template<typename Arg>
  void Emplace(Arg &&arg);

  Data* AcquireSafeNode(std::atomic<Data*>& atomic_node, HazardPoint& hp);

  static void DeleteData(void *data) { delete static_cast<Data*>(data); }

  std::atomic<Data*> front_;
  std::atomic<Data*> tail_;
  std::atomic<size_t> size_;

  static HazardList global_hp_list_;
};

template<typename T> HazardList LockFreeStack<T>::global_hp_list_;

template<typename T>
class StackReclaimer : public Reclaimer {
 public:
  static StackReclaimer& GetInstance() {
    thread_local StackReclaimer stackReclaimer =
        StackReclaimer(LockFreeStack<T>::global_hp_list_);
    return stackReclaimer;
  }

  ~ StackReclaimer() override = default;

  StackReclaimer() = delete;
  StackReclaimer(const StackReclaimer &other) = delete;
  StackReclaimer(StackReclaimer &&other) = delete;
  StackReclaimer& operator = (const StackReclaimer &other) = delete;
  StackReclaimer& operator = (StackReclaimer &&other) = delete;
 private:
  explicit StackReclaimer(HazardList &global_hp_list) : Reclaimer(global_hp_list) {}
};

template<typename T> template<typename Arg>
void LockFreeStack<T>::Emplace(Arg &&arg) {
  auto *new_front = new Data();
  new_front->data_.store(new T(std::forward<Arg>(arg)), std::memory_order_release);
  for (;;) {
    HazardPoint hp;
    auto *front = AcquireSafeNode(front_, hp);
    new_front->next_.store(front, std::memory_order_release);
    if (front_.compare_exchange_strong(front, new_front, std::memory_order_acq_rel)) {
      size_.fetch_add(1, std::memory_order_acq_rel);
      return ;
    }
  }
}

template<typename T>
bool LockFreeStack<T>::Pop(T &data) {
  Data *front;
  Data *new_front;
  HazardPoint hp;
  do {
    front = AcquireSafeNode(front_, hp);
    if (front->next_.load(std::memory_order_acquire) == nullptr) {
      return false;
    }
    new_front = front->next_.load(std::memory_order_acquire);
  } while (!front_.compare_exchange_strong(front, new_front, std::memory_order_acq_rel));
  size_.fetch_sub(1, std::memory_order_acq_rel);

  data = std::forward<T>(*front->data_.load(std::memory_order_acquire));
  auto &reclaimer = StackReclaimer<T>::GetInstance();
  reclaimer.ReclaimLater(front, DeleteData);
  return true;
}

template<typename T>
LockFreeStack<T>::Data* LockFreeStack<T>::AcquireSafeNode(std::atomic<Data *> &atomic_node, HazardPoint &hp) {
  auto &reclaimer = StackReclaimer<T>::GetInstance();
  auto *node = atomic_node.load(std::memory_order_acquire);
  Data *temp;
  do {
    hp.Unmark();
    temp = node;
    hp = HazardPoint(&reclaimer, node);
    node = atomic_node.load(std::memory_order_acquire);
  } while (temp != node);
  return node;
}
}

#endif
