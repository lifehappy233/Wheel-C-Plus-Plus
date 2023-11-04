#include <iostream>
#include <string>
#include <type_traits>
#include <functional>

template<typename ...Ts> union __union;

template<typename T, typename ...Ts>
union __union<T, Ts...> {
  using type = __union;
  using rest_type = __union<Ts...>;
  using value_type = T;

  T value_;
  rest_type rest_;

  template<typename Tv>
  requires std::is_same_v<std::decay_t<Tv>, T>
  __union(Tv&& value) : value_(std::forward<Tv>(value)) {}

  template <typename Tv>
  __union(Tv&& rest) : rest_(std::forward<Tv>(rest)) {}

  __union() {}
  ~ __union() {}
};

template<typename T>
union __union<T> {
  using type = __union;
  using value_type = T;

  T value_;

  template<typename Tv>
  requires std::is_same_v<std::decay_t<Tv>, T>
  __union(Tv&& value) : value_(std::forward<Tv>(value)) {}

  __union() {}
  ~ __union() {}
};

template<typename Tu, typename T> struct type_index_impl
    : std::integral_constant<int, type_index_impl<typename Tu::rest_type, T>::value + 1> {};

template<typename Tu>
struct type_index_impl<Tu, typename Tu::value_type>
    : std::integral_constant<int, 0> {};

template<typename Tu, typename T>
constexpr static int type_index = type_index_impl<Tu, T>::value;

template<typename ...Ts>
struct variant {
  using type = variant;
  using data_type = __union<Ts...>;

  __union<Ts...> data_;

  int type_{-1};

  std::function<void(void *)> destructors[sizeof...(Ts)] =
      { [](void *ptr) { static_cast<Ts*>(ptr)->~Ts(); }... };

  template<typename Tv>
  variant(Tv&& data) : data_(std::forward<Tv>(data)),
                       type_(type_index<data_type, std::decay_t<Tv>>) {}

  template<typename Tv>
  variant& operator = (Tv&& data) {
    if (~type_) {
      destructors[type_](&data_);
    }
    new (&data_) data_type(std::forward<Tv>(data));
    type_ = type_index<data_type, std::decay_t<Tv>>;
    return *this;
  }

  variant() {}
  ~ variant() {
    if (~type_) {
      destructors[type_](&data_);
    }
  }
};

template<int N, typename ...Ts> struct variant_alternative {};

template<int N, typename ...Ts>
struct variant_alternative<N, variant<Ts...>>
    : variant_alternative<N, typename variant<Ts...>::data_type> {};

template<int N, typename ...Ts>
struct variant_alternative<N, __union<Ts...>>
    : variant_alternative<N - 1, typename __union<Ts...>::rest_type> {};

template<typename ...Ts>
struct variant_alternative<0, __union<Ts...>> {
  using type = __union<Ts...>::value_type;
};

template<int N, typename ...Ts>
using variant_alternative_t = variant_alternative<N, Ts...>::type;

template<typename ...Ts> struct variant_size {};

template<typename ...Ts>
struct variant_size<variant<Ts...>>
    : std::integral_constant<int, sizeof...(Ts)> {};

template<typename ...Ts>
constexpr static int variant_size_v = variant_size<Ts...>::value;

template<int N, typename T>
struct get_n_impl {
  static auto&& get(T &data) {
    return get_n_impl<N - 1, typename T::rest_type>::get(data.rest_);
  }
};

template<typename T>
struct get_n_impl<0, T> {
  static T::value_type& get(T &data) {
    return data.value_;
  }
};

template<int N, typename T>
static auto&& get(T &var) {
  return get_n_impl<N, typename T::data_type>::get(var.data_);
}

template<typename T, typename Tv>
struct get_type_impl {
  static auto&& get(Tv &data) {
    return get_type_impl<T, typename Tv::rest_type>::get(data.rest_);
  }
};

template<typename Tv>
struct get_type_impl<typename Tv::value_type, Tv> {
  static Tv::value_type& get(Tv &data) {
    return data.value_;
  }
};

template<typename T, typename Tv>
static auto&& get(Tv &var) {
  return get_type_impl<T, typename Tv::data_type>::get(var.data_);
}

int main() {
  int a = 114;
  auto u1 = __union<short int, int, unsigned int, long long, float, double>(a);
  std::cout << u1.value_ << " " << u1.rest_.value_ << " " << u1.rest_.rest_.value_ << " "
            << u1.rest_.rest_.rest_.value_ << " " << u1.rest_.rest_.rest_.rest_.value_ << " "
            << u1.rest_.rest_.rest_.rest_.rest_.value_ << "\n";

  const double b = 115.514;
  auto u2 = __union<short int, int, unsigned int, long long, float, double>(b);
  std::cout << u2.value_ << " " << u2.rest_.value_ << " " << u2.rest_.rest_.value_ << " "
            << u2.rest_.rest_.rest_.value_ << " " << u2.rest_.rest_.rest_.rest_.value_ << " "
            << u2.rest_.rest_.rest_.rest_.rest_.value_ << "\n";

  std::string str1 = "lifehappy";
  auto u3 = __union<int, double, std::string>(std::move(str1));
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
