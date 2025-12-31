#include "core/string_utils.h"

namespace core {

	bool contains(const string& source, const string& sub)
	{
		return source.find(sub) != string::npos;
	}	

	vector<string> split(const string& source, const string& delimiter)
	{
		vector<string> result;
		size_t pos = 0;
		string s = source;		
		while ((pos = s.find(delimiter)) != string::npos) {			
			result.push_back(s.substr(0, pos));
			s.erase(0, pos + delimiter.length());
		}
		result.push_back(s);
		return result;
	}

	template<>	
	size_t toNumber<>(const string& source)
	{
		size_t i;
		sscanf_s(source.c_str(), "%zu", &i);
		return i;
	}

	template<>
	float toNumber<>(const string& source)
	{
		float f;
		sscanf_s(source.c_str(), "%f", &f);
		return f;
	}

	template<>
	int toNumber<>(const string& source)
	{
		int i;
		sscanf_s(source.c_str(), "%d", &i);
		return i;
	}

	vector<string> split(const string& str, char smb)
	{
		vector<std::string> result;
		size_t start = 0;
		size_t end = str.find(smb);
		while (end != string::npos) {
			result.push_back(str.substr(start, end - start));
			start = end + 1;
			end = str.find(smb, start);
		}
		result.push_back(str.substr(start));
		return result;
	}
	
}