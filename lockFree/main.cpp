#include <iostream>

#include "reclaim.h"
#include "lockFreeQueue.h"

void delete_function(void *ptr) {
  std::cout << *static_cast<int*>(ptr) << "\n";
  delete static_cast<int*>(ptr);
}

struct Node {
  Node(void *ptr, std::function<void(void*)>&& delete_function)
      : ptr_(ptr), delete_function_(std::move(delete_function)) {}

  ~ Node() = default;

  void *ptr_;
  std::function<void(void*)> delete_function_;
};

std::unordered_map<void*, Node*> map_;

int main() {
  for (int i = 0; i < 10; i++) {
    int *p = new int(i);
    map_.insert(std::make_pair(p, new Node(p, delete_function)));
  }
  for (auto it = map_.begin(); it != map_.end(); ) {
    std::cout << (it->first == it->second->ptr_) << " ";
    it->second->delete_function_(it->second->ptr_);
    delete it->second;
    std::cout << (it->first == it->second->ptr_) << "\n";
    it = map_.erase(it);
  }

//  for (auto it = map_.begin(); it != map_.end(); it++) {
//    std::cout << *(int *)it->first << "\n";
////    std::cout << (it->first == nullptr) << " " << (it->second == nullptr) << "\n";
//  }

  std::cout << map_.size() << "\n";
//  std::function<void(void*)> func(nullptr);
//  std::cout << (func == nullptr) << "\n";
  return 0;
}
