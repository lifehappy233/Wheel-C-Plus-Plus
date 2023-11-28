#include <iostream>
#include <cmath>

#include "lockFreeHashTable.h"

int main() {
  auto hash = std::hash<std::string>();
  std::cout << hash("okkk\n") << "\n";
  return 0;
}
