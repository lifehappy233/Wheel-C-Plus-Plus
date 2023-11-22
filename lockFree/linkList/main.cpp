#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>
// #include <boost/lockfree/queue.hpp>
// #include <boost/lockfree/stack.hpp>

int main() {
  static const int kCoefficient = 4 + 1 / 4;
  std::cout << kCoefficient << "\n";
  return 0;
}
