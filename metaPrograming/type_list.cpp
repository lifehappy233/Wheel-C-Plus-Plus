#include <iostream>

#include "typeList.h"

using namespace typeList;

void impl_test();

void algorithm_test();

int main() {
//  impl_test();
  // insert in sorted: 1 1 1 1 1
  std::cout << "insert in sorted: ";
  std::cout << std::is_same_v<type_list<int32_t>, insert_in_sorted<type_list<>, int32_t , compare>> << " "
            << std::is_same_v<type_list<char, short, int32_t, int64_t>, insert_in_sorted<type_list<char, short, int32_t>, int64_t, compare>> << " "
            << std::is_same_v<type_list<char, short, int32_t, int64_t>, insert_in_sorted<type_list<char, short, int64_t>, int32_t, compare>> << " "
            << std::is_same_v<type_list<char, short, int32_t, float, int64_t>, insert_in_sorted<type_list<char, short, int32_t, int64_t>, float, compare>> << " "
            << std::is_same_v<type_list<char, int32_t, long long, float, int64_t>, insert_in_sorted<type_list<char, long long, float, int64_t>, int32_t, compare>> << "\n";

  // insert sort: 1 1 1
  std::cout << "insert sort: ";
  std::cout << std::is_same_v<type_list<>, insert_sort<type_list<>, compare>> << " "
            << std::is_same_v<type_list<char>, insert_sort<type_list<char>, compare>> << " "
            << std::is_same_v<type_list<float, int>, insert_sort<type_list<int, float>, compare>> << " ";

  // 非稳定排序，相同大小的类型，初始在前面的会跑到后面去
  using type_list_sort = type_list<short int, char, short, int, long long, unsigned int, unsigned, unsigned long long, unsigned char>;
  using type_list_sort_ans = type_list<unsigned char, char, short, short int, unsigned, unsigned int, int, unsigned long long, long long>;
  std::cout << std::is_same_v<type_list_sort_ans, insert_sort<type_list_sort, compare>> << "\n";

  return 0;
}

void impl_test() {

  // is empty: 1 0 0
  std::cout << "is empty: ";
  std::cout << is_empty<type_list<>> << " "
            << is_empty<type_list<int>> << " "
            << is_empty<type_list<int, float, double>> << "\n\n";

  // front: 1 0
  std::cout << "front: ";
  std::cout << std::is_same_v<int, front<type_list<int, float>>> << " "
            << std::is_same_v<float, front<type_list<int, float>>> << "\n\n";

  // pop front: 1 0 1
  std::cout << "pop front: ";
  std::cout << std::is_same_v<type_list<>, pop_front<type_list<int>>> << " "
            << std::is_same_v<type_list<int>, pop_front<type_list<int>>> << " "
            << std::is_same_v<type_list<int>, pop_front<type_list<float, int>>> << "\n\n";

  // push front: 1 0 1
  std::cout << "push front: ";
  std::cout << std::is_same_v<type_list<int>, push_front<type_list<>, int>> << " "
            << std::is_same_v<type_list<int, float>, push_front<type_list<int>, float>> << " "
            << std::is_same_v<type_list<float, int>, push_front<type_list<int>, float>> << "\n\n";

  // push back: 1 1 0
  std::cout << "push back: ";
  std::cout << std::is_same_v<type_list<int>, push_back<type_list<>, int>> << " "
            << std::is_same_v<type_list<int, float>, push_back<type_list<int>, float>> << " "
            << std::is_same_v<type_list<float, int>, push_back<type_list<int>, float>> << "\n\n";

  // back: 1 0 1
  std::cout << "back: ";
  std::cout << std::is_same_v<int, back<type_list<int>>> << " "
            << std::is_same_v<int, back<type_list<int, float>>> << " "
            << std::is_same_v<float, back<type_list<int, float>>> << "\n\n";

  // pop back: 1 1 0
  std::cout << "pop back: ";
  std::cout << std::is_same_v<type_list<>, pop_back<type_list<int>>> << " "
            << std::is_same_v<type_list<float>, pop_back<type_list<float, int>>> << " "
            << std::is_same_v<type_list<int>, pop_back<type_list<float, int>>> << "\n\n";

  // reverse: 1 0 1 1
  std::cout << "reverse: ";
  std::cout << std::is_same_v<type_list<>, reverse<type_list<>>> << " "
            << std::is_same_v<type_list<int, float>, reverse<type_list<int, float>>> << " "
            << std::is_same_v<type_list<float, int>, reverse<type_list<int, float>>> << " "
            << std::is_same_v<type_list<int, float, double>, reverse<type_list<double, float, int>>> << "\n\n";

  // largest: 1, 0, 1
  // char, short, int32_t, int64_t, double
  std::cout << sizeof(char) << " " << sizeof(short) << " " << sizeof(int32_t)
            << " " << sizeof(int64_t) << " " << sizeof(double) << "\n";
  using type1 = type_list<char, short, int32_t, int64_t, double>;
  using type2 = type_list<char, short, int32_t, double, int64_t>;
  std::cout << "largest: ";
  std::cout << std::is_same_v<double, largest<type1, compare>> << " "
            << std::is_same_v<double, largest<type2, compare>> << " "
            << std::is_same_v<int64_t, largest<type2, compare>> << "\n\n";

  // merge: 1 1 1 0
  std::cout << "merge: ";
  std::cout << std::is_same_v<type_list<int>, merge<type_list<>, type_list<int>>> << " "
            << std::is_same_v<type_list<int>, merge<type_list<int>, type_list<>>> << " "
            << std::is_same_v<type_list<int, float>, merge<type_list<int>, type_list<float>>> << " "
            << std::is_same_v<type_list<int, float>, merge<type_list<float>, type_list<int>>> << "\n\n";

  // insert: 1 1 1
  std::cout << "insert: ";
  std::cout << std::is_same_v<type_list<int>, insert<type_list<>, 0, int>> << " "
            << std::is_same_v<type_list<int, float, double>, insert<type_list<int, double>, 1, float>> << " "
            << std::is_same_v<type_list<int, float, double>, insert<type_list<int, float>, 2, double>> << "\n";
}
