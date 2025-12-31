#pragma once
#include <string>
#include <vector>
#include <map>
#include <stack>
#include <set>
#include <unordered_map>
#include <cstdio>
#include <cstring>
#include <tuple>
#include <array>
#include <limits>
#include <functional>
#include <optional>
#include <deque>
#include <queue>
//#include <utility>
//#include <variant>

using string = std::string;
using stringView = std::string_view;

template<typename T, class _Alloc = std::allocator<T>>
using vector = std::vector<T, _Alloc>;

template <class Key, class T, class Comparator = std::less<Key>>
using map = std::map<Key, T, Comparator>;

template <class Key, class T>
using unordered_map = std::unordered_map<Key, T>;

template <class T, class Comparator = std::less<T>>
using set = std::set<T, Comparator>;

template<class T>
using stack = std::stack<T>;

using string = std::string;

template< class... Types >
using tuple = std::tuple< Types... >;

template <class T, size_t Size>
using array = std::array<T, Size>;

template<class T, class Container = std::deque<T>> 
using queue = std::queue<T, Container>;

// numbers 
using uint64 = std::uint64_t;
using uint32 = std::uint32_t;
using uint16 = std::uint16_t;
using uint8 = std::uint8_t;


template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
constexpr inline bool EnumHasAnyFlags(Enum Flags, Enum Contains)
{
	return (((__underlying_type(Enum))Flags) & (__underlying_type(Enum))Contains) != 0;
}

template<typename Enum>
constexpr bool EnumHasAllFlags(Enum flags, Enum contains)
{
	using U = std::underlying_type_t<Enum>;
	return (static_cast<U>(flags) & static_cast<U>(contains)) == static_cast<U>(contains);
}
