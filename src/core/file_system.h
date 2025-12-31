#pragma once

#include "core/std_types.h"
//TODO : don`t use windows for file 
#include <windows.h>

namespace core {
	class File
	{
	public:
		enum Mode
		{
			None = 0,
			Read = 1 << 0,
			Write = 1 << 1,
			Create = 1 << 2,
		};

		~File();

		bool open(const char* name, Mode mode);
		void close();
		uint32 read(void* data, uint32 size);
		bool write(const void* pData, size_t size);
		bool write(const string& str);

		bool isOpen() const;
		uint32 length() const { return length_; }

	private:
		HANDLE file_;
		Mode mode_;
		uint32 length_;
		uint32 position_;
	};
}