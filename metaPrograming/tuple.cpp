#include <iostream>
#include <type_traits>

template<typename ...Ts> struct tuple;

template<typename T> struct tuple<T> {
  using type = tuple;
  using value_type = T;

  T value_;

  template<typename Tv>
  requires std::is_same_v<std::decay_t<Tv>, T>
  tuple(Tv&& value) : value_(std::forward<Tv>(value)) {}

  tuple() {}

  ~ tuple() {}
};

template<typename T, typename ...Ts> struct tuple<T, Ts...> : tuple<Ts...> {
  using type = tuple;
  using value_type = T;
  using super_type = tuple<Ts...>;

  T value_;

  template<typename Tv, typename ...Tvs>
  requires std::is_same_v<std::decay_t<Tv>, T>
  tuple(Tv&& value, Tvs&& ...rest)
      : value_(std::forward<Tv>(value)),
        super_type(std::forward<Tvs>(rest)...) {}

  tuple() {}

  ~ tuple() {}
};

template<typename T> struct tuple_size {};

template<typename ...Ts>
struct tuple_size<tuple<Ts...>> : std::integral_constant<int, sizeof...(Ts)> {};

template<typename T>
constexpr static int tuple_size_v = tuple_size<T>::value;

template<int N, typename T> struct tuple_element {};

template<int N, typename ...Ts>
struct tuple_element<N, tuple<Ts...>> : tuple_element<N - 1, typename tuple<Ts...>::super_type> {};

template<typename ...Ts>
struct tuple_element<0, tuple<Ts...>> { using type = tuple<Ts...>::value_type; };

template<int N, typename T>
using tuple_element_t = tuple_element<N, T>::type;

template<int N, typename T> struct tuple_type_impl {};

template<int N, typename ...Ts>
struct tuple_type_impl<N, tuple<Ts...>>
    : tuple_type_impl<N - 1, typename tuple<Ts...>::super_type> {};

template<typename ...Ts>
struct tuple_type_impl<0, tuple<Ts...>> {
  using type = tuple<Ts...>::type;
};

template<int N, typename T>
using tuple_type = tuple_type_impl<N, T>::type;

template<int N, typename ...Ts>
auto&& get(tuple<Ts...> &tup) {
  using type = tuple_type<N, tuple<Ts...>>;
  return static_cast<type&>(tup).value_;
}

int main() {
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
