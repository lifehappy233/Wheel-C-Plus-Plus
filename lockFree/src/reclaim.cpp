#include <unordered_set>

#include "reclaim.h"

namespace lockFree {

size_t Reclaimer::MarkHazard(void *ptr) {
  if (ptr == nullptr) {
    return -1;
  }
  // 先从 local_hp_list_ 找，这里的保证不会有线程竞争
  size_t res_index = 0;
  for (auto *it : local_hp_list_) {
    if (it->ptr_.load(std::memory_order_relaxed) == nullptr) {
      it->ptr_.store(ptr, std::memory_order_release);
      return res_index;
    }
    res_index++;
  }
  RequireHazardPoint();
  res_index = local_hp_list_.size() - 1;
  local_hp_list_[res_index]->ptr_.store(ptr, std::memory_order_release);
  return res_index;
}

void Reclaimer::UnmarkHazard(size_t index) {
  assert(0 <= index && index < local_hp_list_.size());
  local_hp_list_[index]->ptr_.store(nullptr, std::memory_order_release);
}

void Reclaimer::ReclaimLater(void *ptr, std::function<void(void *)> &&delete_function){
  auto reclaimPoint = reclaim_pool_.Pop();
  reclaimPoint->ptr_ = ptr;
  reclaimPoint->delete_function_ = std::move(delete_function);

  reclaim_map_.insert(std::make_pair(ptr, reclaimPoint));
}

void Reclaimer::ReclaimNoHazard() {
  if (reclaim_map_.size() < rate_ * global_hp_list_.Size()) {
    return ;
  }
  std::unordered_set<void*> can_not_delete_set;
  auto p = global_hp_list_.head_.load(std::memory_order_acquire);
  while (p) {
    void* const ptr = p->ptr_.load(std::memory_order_acquire);
    if (ptr != nullptr) {
      can_not_delete_set.insert(ptr);
    }
    p = p->next_.load(std::memory_order_acquire);
  }

  for (auto it = reclaim_map_.begin(); it != reclaim_map_.end(); ) {
    if (can_not_delete_set.find(it->second->ptr_) != can_not_delete_set.end()) {
      it++;
      continue ;
    }
    it->second->delete_function_(it->second->ptr_);
    reclaim_pool_.Push(it->second);
    it = reclaim_map_.erase(it);
  }
}

void Reclaimer::RequireHazardPoint() {
  // 先从global中获取
  auto p = global_hp_list_.head_.load(std::memory_order_acquire);
  while (p) {
    if (!p->flag_.test_and_set()) {
      local_hp_list_.emplace_back(p);
      return ;
    }
    p = p->next_.load(std::memory_order_acquire);
  }

  // 否则只能 new 一个了
  p = new InternalHazardPoint();
  p->flag_.test_and_set();
  InternalHazardPoint *old_head;
  do {
    old_head = global_hp_list_.head_.load(std::memory_order_acquire);
    p->next_ = old_head;
  } while (!global_hp_list_.head_.compare_exchange_strong(old_head, p, std::memory_order_acq_rel));
  local_hp_list_.emplace_back(p);
}

bool Reclaimer::Hazard(void *ptr) {
  auto p = global_hp_list_.head_.load(std::memory_order_acquire);
  while (p) {
    if (p->ptr_.load(std::memory_order_acquire) == ptr) {
      return true;
    }
    p = p->next_.load(std::memory_order_acquire);
  }
  return false;
}

}
