#pragma once

#include "core/std_types.h"

namespace math {
	template<typename T>
	constexpr T Max(const T& a, const T& b)
	{
		return a < b ? b : a;
	}

	template<typename T>
	constexpr T Min(const T& a, const T& b)
	{
		return a < b ? a : b;
	}

	constexpr inline uint32 divideAndRoundUp(uint32 nominator, uint32 denominator)
	{
		return (nominator + denominator - 1) / denominator;
	}

}