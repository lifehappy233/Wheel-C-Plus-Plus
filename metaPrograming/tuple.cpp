#include <iostream>

#include "myTuple.h"

int main() {
  using namespace myTuple;
  const std::string str1 = "lifehappy";
  tuple<int, float, double, std::string> a1(114514, 114.514f, 114.514, std::move(str1));
  // std::cout << a1.value_ << " " << a1.rest_.value_ << " " << a1.rest_.rest_.value_ << " " << a1.rest_.rest_.rest_.value_ << "\n";
  // std::cout << str1 << " str1\n";

  std::string str2 = "lifehappy";
  tuple<int, float, double, std::string> a2(114514, 114.514f, 114.514, std::move(str2));
  // std::cout << a2.value_ << " " << a2.rest_.value_ << " " << a2.rest_.rest_.value_ << " " << a2.rest_.rest_.rest_.value_ << "\n";
  // std::cout << str2 << " str2\n";

  std::cout << tuple_size_v<decltype(a1)> << " " << tuple_size<decltype(a2)>::value << "\n";

  std::cout << std::is_same_v<tuple_element<0, decltype(a1)>::type, tuple_element_t<0, decltype(a1)>> << " "
            << std::is_same_v<tuple_element<1, decltype(a1)>::type, tuple_element_t<1, decltype(a1)>> << " "
            << std::is_same_v<tuple_element<2, decltype(a1)>::type, tuple_element_t<2, decltype(a1)>> << " "
            << std::is_same_v<tuple_element<3, decltype(a1)>::type, tuple_element_t<3, decltype(a1)>> << "\n";

  std::cout << get<0>(a1) << " " << get<1>(a1) << " "
            << get<2>(a1) << " " << get<3>(a1) << "\n";
  get<0>(a2) = 415411;
  std::cout << get<0>(a2) << " " << get<1>(a2) << " "
            << get<2>(a2) << " " << get<3>(a2) << "\n";
  return 0;
}
