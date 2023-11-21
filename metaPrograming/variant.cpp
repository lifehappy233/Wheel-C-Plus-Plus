#include <iostream>

#include "myVariant.h"

int main() {
  using namespace myVariant;
  int a = 114;
  auto u1 = _union<short int, int, unsigned int, long long, float, double>(a);
  std::cout << u1.value_ << " " << u1.rest_.value_ << " " << u1.rest_.rest_.value_ << " "
            << u1.rest_.rest_.rest_.value_ << " " << u1.rest_.rest_.rest_.rest_.value_ << " "
            << u1.rest_.rest_.rest_.rest_.rest_.value_ << "\n";

  const double b = 115.514;
  auto u2 = _union<short int, int, unsigned int, long long, float, double>(b);
  std::cout << u2.value_ << " " << u2.rest_.value_ << " " << u2.rest_.rest_.value_ << " "
            << u2.rest_.rest_.rest_.value_ << " " << u2.rest_.rest_.rest_.rest_.value_ << " "
            << u2.rest_.rest_.rest_.rest_.rest_.value_ << "\n";

  std::string str1 = "lifehappy";
  auto u3 = _union<int, double, std::string>(std::move(str1));
  std::cout << u3.value_ << " " << u3.rest_.value_ << " " << u3.rest_.rest_.value_ << " : __union\n";
  std::cout << str1 << " : str\n";

  auto v3 = variant<int, double, std::string>();

  std::cout << std::is_same_v<int, variant_alternative_t<0, decltype(v3)>> << " "
            << std::is_same_v<double, variant_alternative_t<1, decltype(v3)>> << " "
            << std::is_same_v<std::string, variant_alternative_t<2, decltype(v3)>> << "\n";
  std::cout << std::is_same_v<int, variant_alternative_t<0, decltype(u3)>> << " "
            << std::is_same_v<double, variant_alternative_t<1, decltype(u3)>> << " "
            << std::is_same_v<std::string, variant_alternative_t<2, decltype(u3)>> << "\n";
  std::cout << std::is_same_v<variant_alternative_t<0, decltype(v3)>, variant_alternative<0, decltype(v3)>::type> << " "
            << std::is_same_v<variant_alternative_t<1, decltype(v3)>, variant_alternative<1, decltype(v3)>::type> << " "
            << std::is_same_v<variant_alternative_t<2, decltype(v3)>, variant_alternative<2, decltype(v3)>::type> << "\n";
  std::cout << std::is_same_v<variant_alternative_t<0, decltype(u3)>, variant_alternative<0, decltype(u3)>::type> << " "
            << std::is_same_v<variant_alternative_t<1, decltype(u3)>, variant_alternative<1, decltype(u3)>::type> << " "
            << std::is_same_v<variant_alternative_t<2, decltype(u3)>, variant_alternative<2, decltype(u3)>::type> << "\n";

  std::cout << variant_size<decltype(v3)>::value << " " << variant_size_v<decltype(v3)> << "\n";

  variant<int, unsigned int, long long, double, std::string> v4((int)114514);
  std::cout << get_n_impl<0, decltype(v4.data_)>::get(v4.data_) << " " << get_n_impl<1, decltype(v4.data_)>::get(v4.data_) << " "
            << get_n_impl<2, decltype(v4.data_)>::get(v4.data_) << " " << get_n_impl<3, decltype(v4.data_)>::get(v4.data_) << " "
            << get_n_impl<4, decltype(v4.data_)>::get(v4.data_) << "\n";
  std::cout << get<0>(v4) << " " << get<1>(v4) << " " << get<2>(v4) << " " << get<3>(v4) << " " << get<4>(v4) << "\n";


  std::cout << get_type_impl<int, decltype(v4.data_)>::get(v4.data_) << " " << get_type_impl<unsigned int, decltype(v4.data_)>::get(v4.data_) << " "
            << get_type_impl<long long, decltype(v4.data_)>::get(v4.data_) << " " << get_type_impl<double, decltype(v4.data_)>::get(v4.data_) << " "
            << get_type_impl<std::string, decltype(v4.data_)>::get(v4.data_) << "\n";
  std::cout << get<int>(v4) << " " << get<unsigned int>(v4) << " " << get<long long>(v4) << " " << get<double>(v4) << " " << get<std::string>(v4) << "\n";

  int int1 = 1;
  const int int2 = 1;
  int &int3 = int1;
  std::cout << variant<int, float, std::string>().type_ << " "
            << variant<int, float, std::string>(int1).type_ << " "
            << variant<int, float, std::string>(int2).type_ << " "
            << variant<int, float, std::string>(int3).type_ << " "
            << variant<int, float, std::string>(1.1f).type_ << " "
            << variant<int, float, std::string>(std::string("lifehappy")).type_ << "\n";

  struct Test1 {
    ~ Test1() {
      std::cout << "~ Test1()\n";
    }
  }ta;

  struct Test2 {
    ~ Test2() {
      std::cout << "~ Test2()\n";
    }
  }tb;

  variant<int, long long, Test1, Test2> variant_test(ta);
  std::cout << "OK\n";
  variant_test = tb;
  std::cout << "OK\n";
  return 0;
}
