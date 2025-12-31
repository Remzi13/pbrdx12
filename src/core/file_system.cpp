#include "core/file_system.h"

#include "core/std_types.h"
#include "core/debug.h"

#include <windows.h>


namespace core {

	File::~File()
	{
		close();
	}
		
	bool File::open(const char* path, Mode mode)
	{
		uint32 access = 0;
		if (mode & Mode::Read)
			access |= GENERIC_READ;
		if (mode & Mode::Write)
			access |= GENERIC_WRITE;

		uint32 disposition = 0;
		if (mode & Mode::Create)
			disposition = CREATE_ALWAYS;
		else if (mode & Mode::Write)
			disposition = OPEN_ALWAYS;
		else
			disposition = OPEN_EXISTING;


		file_ = ::CreateFileA(path, access, 0, nullptr, disposition, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file_ != INVALID_HANDLE_VALUE)
		{
			length_ = ::GetFileSize(file_, nullptr);			
			mode_ = mode;
			position_ = 0;
		}
		return file_ != INVALID_HANDLE_VALUE;
	}

	void File::close()
	{
		if (isOpen())
		{
			CloseHandle(file_);
			file_ = nullptr;
		}
	}

	uint32 File::read(void* data, uint32 size)
	{
		ASSERT(isOpen());
		ASSERT(mode_& Mode::Read);

		DWORD read = 0;
		BOOL result = ::ReadFile(file_, data, size, &read, nullptr);
		if (result)
			position_ += read;
		
		return read;
	}

	bool File::write(const string& str)
	{
		return write(str.data(), str.size());
	}

	bool File::write(const void* data, size_t size)
	{
		ASSERT(isOpen());
		ASSERT(mode_ & Mode::Write);

		DWORD written = 0;
		BOOL result = ::WriteFile(file_, data, (DWORD)size, &written, nullptr);
		position_ += written;
		return result == TRUE;
	}

	bool File::isOpen() const
	{
		return file_ != INVALID_HANDLE_VALUE;
	}
}