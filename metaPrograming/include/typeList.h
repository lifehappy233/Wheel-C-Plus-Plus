#ifndef TYPELIST_H_
#define TYPELIST_H_

#include <type_traits>

namespace typeList {
template <typename... Ts>
struct type_list {};

// is_empty
template <typename T>
struct is_empty_impl : std::integral_constant<bool, false> {};

template <>
struct is_empty_impl<type_list<>> : std::integral_constant<bool, true> {};

template <typename T>
constexpr static bool is_empty = is_empty_impl<T>::value;

// front
template <typename T>
struct front_impl {};

template <typename T, typename... Ts>
struct front_impl<type_list<T, Ts...>> {
  using type = T;
};

template <typename T>
using front = front_impl<T>::type;

// pop front
template <typename T>
struct pop_front_impl {};

template <typename T, typename... Ts>
struct pop_front_impl<type_list<T, Ts...>> {
  using type = type_list<Ts...>;
};

template <typename T>
using pop_front = pop_front_impl<T>::type;

// push front
template <typename Tl, typename T>
struct push_front_impl {};

template <typename... Ts, typename T>
struct push_front_impl<type_list<Ts...>, T> {
  using type = type_list<T, Ts...>;
};

template <typename Tl, typename T>
using push_front = push_front_impl<Tl, T>::type;

// push back
template <typename Tl, typename T>
struct push_back_impl {};

template <typename... Ts, typename T>
struct push_back_impl<type_list<Ts...>, T> {
  using type = type_list<Ts..., T>;
};

template <typename Tl, typename T>
using push_back = push_back_impl<Tl, T>::type;

// back
template <typename Tl>
struct back_impl {};

template <typename T, typename... Ts>
struct back_impl<type_list<T, Ts...>> : back_impl<type_list<Ts...>> {};

template <typename T>
struct back_impl<type_list<T>> {
  using type = T;
};

template <typename Tl>
using back = back_impl<Tl>::type;

// pop back
template <typename Tl>
struct pop_back_impl {};

template <typename T, typename... Ts>
struct pop_back_impl<type_list<T, Ts...>> : push_front_impl<typename pop_back_impl<type_list<Ts...>>::type, T> {};

template <typename T>
struct pop_back_impl<type_list<T>> {
  using type = type_list<>;
};

template <typename Tl>
using pop_back = pop_back_impl<Tl>::type;

// reverse
template <typename Tl, bool empty = is_empty<Tl>>
struct reverse_impl {};

template <typename Tl>
struct reverse_impl<Tl, false> : push_back_impl<typename reverse_impl<pop_front<Tl>>::type, front<Tl>> {};

template <typename Tl>
struct reverse_impl<Tl, true> {
  using type = type_list<>;
};

template <typename Tl>
using reverse = reverse_impl<Tl>::type;

// largest
template <bool judge, typename T1, typename T2>
struct type_choose {
  using type = T2;
};

template <typename T1, typename T2>
struct type_choose<true, T1, T2> {
  using type = T1;
};

template <typename T1, typename T2>
struct compare : std::integral_constant<bool, false> {};

template <typename T1, typename T2>
  requires(sizeof(T1) > sizeof(T2))
struct compare<T1, T2> : std::integral_constant<bool, true> {};

template <typename Tl, template <typename...> typename C>
struct largest_impl {};

template <typename T, template <typename...> typename C>
struct largest_impl<type_list<T>, C> {
  using type = T;
};

template <typename T, typename... Ts, template <typename...> typename C>
struct largest_impl<type_list<T, Ts...>, C> : type_choose<C<T, typename largest_impl<type_list<Ts...>, C>::type>::value,
                                                          T, typename largest_impl<type_list<Ts...>, C>::type> {};

template <typename Tl, template <typename...> typename C>
using largest = largest_impl<Tl, C>::type;

// merge
template <typename Tl1, typename Tl2>
struct merge_impl {};

template <typename... Ts1, typename... Ts2>
struct merge_impl<type_list<Ts1...>, type_list<Ts2...>> {
  using type = type_list<Ts1..., Ts2...>;
};

template <typename Tl1, typename Tl2>
using merge = merge_impl<Tl1, Tl2>::type;

// insert
template <typename Tl, int index, typename T>
struct insert_impl {};

template <typename Tf, typename... Ts, int index, typename T>
  requires(index > 0)
struct insert_impl<type_list<Tf, Ts...>, index, T>
    : push_front_impl<typename insert_impl<type_list<Ts...>, index - 1, T>::type, Tf> {};

template <typename... Ts, typename T>
struct insert_impl<type_list<Ts...>, 0, T> : push_front_impl<type_list<Ts...>, T> {};

template <typename Tl, int index, typename T>
using insert = insert_impl<Tl, index, T>::type;

// insert in sorted
template <typename Tl, typename T, template <typename...> typename C, bool empty = is_empty<Tl>>
struct insert_in_sorted_impl {};

template <typename... Ts, typename T, template <typename...> typename C>
struct insert_in_sorted_impl<type_list<Ts...>, T, C, false>
    : type_choose<(C<front<type_list<Ts...>>, T>::value), push_front<type_list<Ts...>, T>,
                  push_front<typename insert_in_sorted_impl<pop_front<type_list<Ts...>>, T, C>::type,
                             front<type_list<Ts...>>>> {};

template <typename... Ts, typename T, template <typename...> typename C>
struct insert_in_sorted_impl<type_list<Ts...>, T, C, true> {
  using type = type_list<T>;
};

template <typename Tl, typename T, template <typename...> typename C>
using insert_in_sorted = insert_in_sorted_impl<Tl, T, C>::type;

// insert sort
template <typename Tl, template <typename...> typename C, bool empty = is_empty<Tl>>
struct insert_sort_impl {};

template <typename... Ts, template <typename...> typename C>
struct insert_sort_impl<type_list<Ts...>, C, true> {
  using type = type_list<>;
};

template <typename... Ts, template <typename...> typename C>
struct insert_sort_impl<type_list<Ts...>, C, false>
    : insert_in_sorted_impl<typename insert_sort_impl<pop_front<type_list<Ts...>>, C>::type, front<type_list<Ts...>>,
                            C> {};

template <typename Tl, template <typename...> typename C>
using insert_sort = insert_sort_impl<Tl, C>::type;
}

#endif
