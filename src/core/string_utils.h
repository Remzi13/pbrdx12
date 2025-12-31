#pragma once

#include "core/std_types.h"

namespace core {

	bool contains(const string& source, const string& sub);

	vector<string> split(const string& source, const string& delimiter);

	template<typename T>
	T toNumber(const string& source);

	template<typename T>
	inline vector<T> toNumbers(const vector<string>& source)
	{
		vector<T> r;
		for (const auto& s : source)
		{
			r.push_back(toNumber<T>(s));
		}
		return r;
	}


	template<typename Source, typename Dest>
	inline size_t stringConvert(const Source* source, Dest* destination, int destinationSize);

	template<>
	inline size_t stringConvert(const wchar_t* source, char* destination, int destinationSize)
	{
		size_t converted = 0;
		wcstombs_s(&converted, destination, destinationSize, source, destinationSize);
		return converted;
	}

	template<>
	inline size_t stringConvert(const char* source, wchar_t* destination, int destinationSize)
	{
		size_t converted = 0;
		mbstowcs_s(&converted, destination, destinationSize, source, destinationSize);
		return converted;
	}

	template<typename Source, typename Dest>
	struct StringConverter
	{
		StringConverter(const Source* pStr)
			: string_{}
		{
			stringConvert<Source, Dest>(pStr, string_, 128);
		}

		Dest* result()  { return string_; }

		
	private:
		Dest string_[128];
	};

	using UnicodeToMultibyte = StringConverter<wchar_t, char>;
	using MultibyteToUnicode = StringConverter<char, wchar_t>;

	vector<string> split(const string& str, char smb);
}

