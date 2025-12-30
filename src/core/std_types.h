#pragma once

#include <cstdint>
#include <string>
#include <algorithm>
#include <vector>
#include <queue>
#include <array>

template<typename T, class _Alloc = std::allocator<T>>
using vector = std::vector<T, _Alloc>;

template<class T, class Container = std::deque<T>>
using queue = std::queue<T, Container>;

using string = std::string;

using uint64 = std::uint64_t;
using uint32 = std::uint32_t;
using uint16 = std::uint16_t;
using uint8 = std::uint8_t;