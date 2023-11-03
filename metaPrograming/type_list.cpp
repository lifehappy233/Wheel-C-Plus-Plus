#include <iostream>
#include <type_traits>

template<typename ...Ts> struct type_list {};

// is_empty
template<typename T>
struct is_empty_impl : std::integral_constant<bool, false> {};

template<>
struct is_empty_impl<type_list<>> : std::integral_constant<bool, true> {};

template<typename T>
constexpr static bool is_empty = is_empty_impl<T>::value;

// front
template<typename T> struct front_impl {};

template<typename T, typename ...Ts>
struct front_impl<type_list<T, Ts...>> {
  using type = T;
};

template<typename T> using front = front_impl<T>::type;

// pop front
template<typename T> struct pop_front_impl {};

template<typename T, typename ...Ts>
struct pop_front_impl<type_list<T, Ts...>> {
  using type = type_list<Ts...>;
};

template<typename T> using pop_front = pop_front_impl<T>::type;

// push front
template<typename Tl, typename T> struct push_front_impl {};

template<typename ...Ts, typename T>
struct push_front_impl<type_list<Ts...>, T> {
  using type = type_list<T, Ts...>;
};

template<typename Tl, typename T>
using push_front = push_front_impl<Tl, T>::type;

// push back
template<typename Tl, typename T> struct push_back_impl {};

template<typename ...Ts, typename T>
struct push_back_impl<type_list<Ts...>, T> {
  using type = type_list<Ts..., T>;
};

template<typename Tl, typename T>
using push_back = push_back_impl<Tl, T>::type;

// back
template<typename Tl> struct back_impl {};

template<typename T, typename ...Ts>
struct back_impl<type_list<T, Ts...>> : back_impl<type_list<Ts...>> {};

template<typename T>
struct back_impl<type_list<T>> { using type = T; };

template<typename Tl> using back = back_impl<Tl>::type;

// pop back
template<typename Tl> struct pop_back_impl {};

template<typename T, typename ...Ts>
struct pop_back_impl<type_list<T, Ts...>>
    : push_front_impl<typename pop_back_impl<type_list<Ts...>>::type, T> {};

template<typename T>
struct pop_back_impl<type_list<T>> { using type = type_list<>; };

template<typename Tl> using pop_back = pop_back_impl<Tl>::type;

// reverse
template<typename Tl, bool empty = is_empty<Tl>> struct reverse_impl {};

template<typename Tl>
struct reverse_impl<Tl, false>
    : push_back_impl<typename reverse_impl<pop_front<Tl>>::type, front<Tl>> {};

template<typename Tl>
struct reverse_impl<Tl, true> { using type = type_list<>; };

template<typename Tl>
using reverse = reverse_impl<Tl>::type;

// largest
template<bool judge, typename T1, typename T2>
struct type_choose {
  using type = T2;
};

template<typename T1, typename T2>
struct type_choose<true, T1, T2> {
  using type = T1;
};

template<typename T1, typename T2>
struct compare : std::integral_constant<bool, false> {};

template<typename T1, typename T2>
requires (sizeof(T1) > sizeof(T2))
struct compare<T1, T2> : std::integral_constant<bool, true> {};

template<typename Tl, template<typename ...> typename C> struct largest_impl {};

template<typename T, template<typename ...> typename C>
struct largest_impl<type_list<T>, C> {
  using type = T;
};

template<typename T, typename ...Ts, template<typename ...> typename C>
struct largest_impl<type_list<T, Ts...>, C>
    : type_choose<C<T, typename largest_impl<type_list<Ts...>, C>::type>::value,
                  T, typename largest_impl<type_list<Ts...>, C>::type> {};

template<typename Tl, template<typename ...> typename C>
using largest = largest_impl<Tl, C>::type;

// merge
template<typename Tl1, typename Tl2> struct merge_impl {};

template<typename ...Ts1, typename ...Ts2>
struct merge_impl<type_list<Ts1...>, type_list<Ts2...>> {
  using type = type_list<Ts1..., Ts2...>;
};

template<typename Tl1, typename Tl2>
using merge = merge_impl<Tl1, Tl2>::type;

// insert
template<typename Tl, int index, typename T> struct insert_impl {};

template<typename Tf, typename ...Ts, int index, typename T>
requires (index > 0)
struct insert_impl<type_list<Tf, Ts...>, index, T>
    : push_front_impl<typename insert_impl<type_list<Ts...>, index - 1, T>::type, Tf> {};

template<typename ...Ts, typename T>
struct insert_impl<type_list<Ts...>, 0, T>
    : push_front_impl<type_list<Ts...>, T> {};

template<typename Tl, int index, typename T>
using insert = insert_impl<Tl, index, T>::type;

// insert in sorted
template<typename Tl, typename T, template<typename ...> typename C,
          bool empty = is_empty<Tl>>
struct insert_in_sorted_impl {};

template<typename ...Ts, typename T, template<typename ...> typename C>
struct insert_in_sorted_impl<type_list<Ts...>, T, C, false>
    : type_choose<(C<front<type_list<Ts...>>, T>::value),
                  push_front<type_list<Ts...>, T>,
                  push_front<
                      typename insert_in_sorted_impl<pop_front<type_list<Ts...>>, T, C>::type,
                      front<type_list<Ts...>>>
                  > {};

template<typename ...Ts, typename T, template<typename ...> typename C>
struct insert_in_sorted_impl<type_list<Ts...>, T, C, true> {
  using type = type_list<T>;
};

template<typename Tl, typename T, template<typename ...> typename C>
using insert_in_sorted = insert_in_sorted_impl<Tl, T, C>::type;

// insert sort
template<typename Tl, template<typename ...> typename C,
          bool empty = is_empty<Tl>> struct insert_sort_impl {};

template<typename ...Ts, template<typename ...> typename C>
struct insert_sort_impl<type_list<Ts...>, C, true> {
  using type = type_list<>;
};

template<typename ...Ts, template<typename ...> typename C>
struct insert_sort_impl<type_list<Ts...>, C, false>
    : insert_in_sorted_impl<
          typename insert_sort_impl<pop_front<type_list<Ts...>>, C>::type,
          front<type_list<Ts...>>, C> {};

template<typename Tl, template<typename ...> typename C>
using insert_sort = insert_sort_impl<Tl, C>::type;


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
