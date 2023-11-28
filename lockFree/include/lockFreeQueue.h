#ifndef LOCKFREEQUEUE_H_
#define LOCKFREEQUEUE_H_

#include "reclaim.h"

namespace lockFree {

template<typename T>
class QueueReclaimer;

template<typename T>
class LockFreeQueue {
 public:
  LockFreeQueue() : front_(new Data()), tail_(front_.load(std::memory_order_relaxed)), size_(0) {}

  ~ LockFreeQueue() {
    auto p = front_.load(std::memory_order_acquire);
    while (p) {
      auto temp = p;
      p = p->next_.load(std::memory_order_acquire);
      delete temp;
    }
  }

  void Push(T &&data) { Emplace(std::move(data)); }

  void Push(const T&data) { Emplace(data); }

  bool Pop(T &data);

  size_t Size() {
    return size_.load(std::memory_order_acquire);
  }

  static size_t Global_Size() {
    return global_hp_list_.Size();
  }

  LockFreeQueue(const LockFreeQueue &other) = delete;
  LockFreeQueue(LockFreeQueue &&other) = delete;
  LockFreeQueue& operator = (const LockFreeQueue &other) = delete;
  LockFreeQueue& operator = (LockFreeQueue &&other) = delete;

 private:
  friend QueueReclaimer<T>;

  struct Data;

  template<typename Arg>
  void Emplace(Arg &&arg);

  Data* AcquireSafeNode(std::atomic<Data*>& atomic_node, HazardPoint& hp);

  bool InsertNewTail(Data *old_tail, Data *new_tail);

  static void DeleteData(void *ptr) { delete static_cast<Data*>(ptr); }

  struct Data {
    Data() : data_(nullptr), next_(nullptr) {}

    ~ Data() {
      auto p = data_.load(std::memory_order_acquire);
      delete p;
    }

    Data(const Data &other) = delete;
    Data(Data &&other) = delete;
    Data& operator = (const Data &other) = delete;
    Data& operator = (Data &&other) = delete;

    std::atomic<T*> data_;
    std::atomic<Data*> next_;
  };

  std::atomic<Data*> front_;
  std::atomic<Data*> tail_;
  std::atomic<size_t> size_;
  static HazardList global_hp_list_;
};

template<typename T>
HazardList LockFreeQueue<T>::global_hp_list_;

template<typename T>
class QueueReclaimer : public Reclaimer {
 public:
  static QueueReclaimer& GetInstance() {
    thread_local QueueReclaimer queueReclaimer =
        QueueReclaimer(LockFreeQueue<T>::global_hp_list_);
    return queueReclaimer;
  }

  ~ QueueReclaimer() override {
    // std::cout << std::this_thread::get_id() << " ~ QueueReclaimer()\n";
  }

  QueueReclaimer() = delete;
  QueueReclaimer(const QueueReclaimer &other) = delete;
  QueueReclaimer(QueueReclaimer &&other) = delete;
  QueueReclaimer& operator = (const QueueReclaimer &other) = delete;
  QueueReclaimer& operator = (QueueReclaimer &&other) = delete;
 private:
  explicit QueueReclaimer(HazardList &global_hp_list) : Reclaimer(global_hp_list) {
    // std::cout << std::this_thread::get_id() << " QueueReclaimer()\n";
  }
};

template<typename T> template<typename Arg>
void LockFreeQueue<T>::Emplace(Arg &&arg) {
  T *data = new T(std::forward<Arg>(arg));
  auto *new_tail = new Data();
  // std::cout << *data << "\n";
  for (;;) {
    HazardPoint hp;
    // std::cout << "OKK\n";
    // std::cout << hp.index() << "\n";
    auto tail = AcquireSafeNode(tail_, hp);
    // std::cout << hp.index() << "\n";
    T *null_ptr = nullptr;
    if (tail->data_.compare_exchange_strong(null_ptr, data, std::memory_order_acq_rel)) {
//      std::cout << *tail->data_.load(std::memory_order_acquire) << "\n";
      if (!InsertNewTail(tail, new_tail)) {
        delete new_tail;
      }
      return ;
    } else {
      if(InsertNewTail(tail, new_tail)) {
        new_tail = new Data();
      }
    }
  }
}

template<typename T>
bool LockFreeQueue<T>::Pop(T &data) {
  HazardPoint hp;
  Data *front;
  Data *new_front;
  do {
    front = AcquireSafeNode(front_, hp);
    if (front == tail_.load(std::memory_order_acquire)) {
      return false;
    }
    new_front = front->next_.load(std::memory_order_acquire);
  } while (!front_.compare_exchange_strong(front, new_front, std::memory_order_acq_rel));
//  std::cout << *front->data_.load() << "\n";
//  std::cout << *front->data_.load(std::memory_order_acquire) << "\n";
  size_.fetch_sub(1, std::memory_order_acq_rel);
  data = std::forward<T>(*front->data_.load(std::memory_order_acquire));
//  std::cout << data << "\n";
  auto &reclaimer = QueueReclaimer<T>::GetInstance();
  reclaimer.ReclaimLater(front, DeleteData);
  reclaimer.ReclaimNoHazard();
  return true;
}

template<typename T>
LockFreeQueue<T>::Data* LockFreeQueue<T>::AcquireSafeNode(std::atomic<Data *> &atomic_node, HazardPoint &hp) {
  auto &reclaimer = QueueReclaimer<T>::GetInstance();
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

template<typename T>
bool LockFreeQueue<T>::InsertNewTail(Data *old_tail, Data *new_tail) {
  Data *null_ptr = nullptr;
  if (old_tail->next_.compare_exchange_strong(null_ptr, new_tail, std::memory_order_acq_rel)) {
    tail_.store(new_tail, std::memory_order_release);
    size_.fetch_add(1, std::memory_order_acq_rel);
    return true;
  }
  return false;
}

}

#endif
