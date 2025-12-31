#pragma once

#include "core/std_types.h"

#include <cmath>

namespace core {
	template<typename T>
	struct Size
	{
		T x;
		T y;

		Size operator/(T v) const
		{
			return { x / v, y / v };
		}
	};

	template<typename T>
	struct Point
	{
		T x;
		T y;
	};

	template<typename T>
	struct Rect
	{
		T Left;
		T Top;
		T Right;
		T Bottom;

		Rect(T left, T top, T right, T bottom) : Left(left), Top(top), Right(right), Bottom(bottom) {}

		T width() const { return Right - Left; }
		T height() const { return Bottom - Top; }
	};

	template<typename T>
	bool operator==(const Size<T>& a, const Size<T>& b)
	{
		return a.x == b.x && a.y == b.y;
	}

	using RectF = Rect<float>;
	using SizeI = Size<uint32>;


	constexpr inline uint32 divideAndRoundUp(uint32 nominator, uint32 denominator)
	{
		return (nominator + denominator - 1) / denominator;
	}

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

	template<typename T>
	T alignUp(T value, T alignment)
	{
		return (value + ((T)alignment - 1)) & ~(alignment - 1);
	}

}