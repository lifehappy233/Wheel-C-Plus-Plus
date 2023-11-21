#ifndef MYVARIANT_H_
#define MYVARIANT_H_

#include <type_traits>
#include <functional>

namespace myVariant {

template <typename... Ts>
union _union;

template <typename T, typename... Ts>
union _union<T, Ts...> {
  using type = _union;
  using rest_type = _union<Ts...>;
  using value_type = T;

  T value_;
  rest_type rest_;

  template <typename Tv>
    requires std::is_same_v<std::decay_t<Tv>, T>
  _union(Tv &&value) : value_(std::forward<Tv>(value)) {}

  template <typename Tv>
  _union(Tv &&rest) : rest_(std::forward<Tv>(rest)) {}

  _union() {}
  ~_union() {}
};

template <typename T>
union _union<T> {
  using type = _union;
  using value_type = T;

  T value_;

  template <typename Tv>
    requires std::is_same_v<std::decay_t<Tv>, T>
  _union(Tv &&value) : value_(std::forward<Tv>(value)) {}

  _union() {}
  ~_union() {}
};

template <typename Tu, typename T>
struct type_index_impl : std::integral_constant<int, type_index_impl<typename Tu::rest_type, T>::value + 1> {};

template <typename Tu>
struct type_index_impl<Tu, typename Tu::value_type> : std::integral_constant<int, 0> {};

template <typename Tu, typename T>
constexpr static int type_index = type_index_impl<Tu, T>::value;

template <typename... Ts>
struct variant {
  using type = variant;
  using data_type = _union<Ts...>;

  _union<Ts...> data_;

  int type_{-1};

  std::function<void(void *)> destructors[sizeof...(Ts)] = {[](void *ptr) { static_cast<Ts *>(ptr)->~Ts(); }...};

  template <typename Tv>
  variant(Tv &&data) : data_(std::forward<Tv>(data)), type_(type_index<data_type, std::decay_t<Tv>>) {}

  template <typename Tv>
  variant &operator=(Tv &&data) {
    if (~type_) {
      destructors[type_](&data_);
    }
    new (&data_) data_type(std::forward<Tv>(data));
    type_ = type_index<data_type, std::decay_t<Tv>>;
    return *this;
  }

  variant() {}
  ~variant() {
    if (~type_) {
      destructors[type_](&data_);
    }
  }
};

template <int N, typename... Ts>
struct variant_alternative {};

template <int N, typename... Ts>
struct variant_alternative<N, variant<Ts...>> : variant_alternative<N, typename variant<Ts...>::data_type> {};

template <int N, typename... Ts>
struct variant_alternative<N, _union<Ts...>> : variant_alternative<N - 1, typename _union<Ts...>::rest_type> {};

template <typename... Ts>
struct variant_alternative<0, _union<Ts...>> {
  using type = _union<Ts...>::value_type;
};

template <int N, typename... Ts>
using variant_alternative_t = variant_alternative<N, Ts...>::type;

template <typename... Ts>
struct variant_size {};

template <typename... Ts>
struct variant_size<variant<Ts...>> : std::integral_constant<int, sizeof...(Ts)> {};

template <typename... Ts>
constexpr static int variant_size_v = variant_size<Ts...>::value;

template <int N, typename T>
struct get_n_impl {
  static auto &&get(T &data) { return get_n_impl<N - 1, typename T::rest_type>::get(data.rest_); }
};

template <typename T>
struct get_n_impl<0, T> {
  static T::value_type &get(T &data) { return data.value_; }
};

template <int N, typename T>
static auto &&get(T &var) {
  return get_n_impl<N, typename T::data_type>::get(var.data_);
}

template <typename T, typename Tv>
struct get_type_impl {
  static auto &&get(Tv &data) { return get_type_impl<T, typename Tv::rest_type>::get(data.rest_); }
};

template <typename Tv>
struct get_type_impl<typename Tv::value_type, Tv> {
  static Tv::value_type &get(Tv &data) { return data.value_; }
};

template <typename T, typename Tv>
static auto &&get(Tv &var) {
  return get_type_impl<T, typename Tv::data_type>::get(var.data_);
}
}

#endif
