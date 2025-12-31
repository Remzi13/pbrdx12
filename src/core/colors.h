#pragma once

namespace core {
	class Color
	{
	public:
		float r{ 0 };
		float g{ 0 };
		float b{ 0 };
		float a{ 0 };

		Color() {}
		constexpr Color(float _r, float _g, float _b, float _a) : r(_r), g(_g), b(_b), a(_a) {}
	};

	namespace Colors
	{
		constexpr Color Transparent = Color(0.0f, 0.0f, 0.0f, 0.0f);
		constexpr Color White = Color(1.0f, 1.0f, 1.0f, 1.0f);
		constexpr Color Black = Color(0.0f, 0.0f, 0.0f, 1.0f);
		constexpr Color Red = Color(1.0f, 0.0f, 0.0f, 1.0f);
		constexpr Color Green = Color(0.0f, 1.0f, 0.0f, 1.0f);
		constexpr Color Blue = Color(0.0f, 0.0f, 1.0f, 1.0f);
		constexpr Color Yellow = Color(1.0f, 1.0f, 0.0f, 1.0f);
		constexpr Color Magenta = Color(1.0f, 0.0f, 1.0f, 1.0f);
		constexpr Color Cyan = Color(0.0f, 1.0f, 1.0f, 1.0f);
		constexpr Color Gray = Color(0.5f, 0.5f, 0.5f, 1.0f);
	}
}