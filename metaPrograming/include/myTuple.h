#ifndef MYTUPLE_H_
#define MYTUPLE_H_

#include <type_traits>

namespace myTuple {

template <typename... Ts>
struct tuple;

template <typename T>
struct tuple<T> {
  using type = tuple;
  using value_type = T;

  T value_;

  template <typename Tv>
    requires std::is_same_v<std::decay_t<Tv>, T>
  tuple(Tv &&value) : value_(std::forward<Tv>(value)) {}

  tuple() {}

  ~tuple() {}
};

template <typename T, typename... Ts>
struct tuple<T, Ts...> : tuple<Ts...> {
  using type = tuple;
  using value_type = T;
  using super_type = tuple<Ts...>;

  T value_;

  template <typename Tv, typename... Tvs>
    requires std::is_same_v<std::decay_t<Tv>, T>
  tuple(Tv &&value, Tvs &&...rest) : value_(std::forward<Tv>(value)), super_type(std::forward<Tvs>(rest)...) {}

  tuple() {}

  ~tuple() {}
};

template <typename T>
struct tuple_size {};

template <typename... Ts>
struct tuple_size<tuple<Ts...>> : std::integral_constant<int, sizeof...(Ts)> {};

template <typename T>
constexpr static int tuple_size_v = tuple_size<T>::value;

template <int N, typename T>
struct tuple_element {};

template <int N, typename... Ts>
struct tuple_element<N, tuple<Ts...>> : tuple_element<N - 1, typename tuple<Ts...>::super_type> {};

template <typename... Ts>
struct tuple_element<0, tuple<Ts...>> {
  using type = tuple<Ts...>::value_type;
};

template <int N, typename T>
using tuple_element_t = tuple_element<N, T>::type;

template <int N, typename T>
struct tuple_type_impl {};

template <int N, typename... Ts>
struct tuple_type_impl<N, tuple<Ts...>> : tuple_type_impl<N - 1, typename tuple<Ts...>::super_type> {};

template <typename... Ts>
struct tuple_type_impl<0, tuple<Ts...>> {
  using type = tuple<Ts...>::type;
};

template <int N, typename T>
using tuple_type = tuple_type_impl<N, T>::type;

template <int N, typename... Ts>
auto &&get(tuple<Ts...> &tup) {
  using type = tuple_type<N, tuple<Ts...>>;
  return static_cast<type &>(tup).value_;
}

}

#endif
