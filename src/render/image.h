#pragma once

#include "core/std_types.h"
#include "core/file_system.h"
#include "core/memory.h"

#include "render/formats.h"

namespace elm::render {

	class Image
	{
	public:
		bool load(const char* path);

		ResourceFormat format() const { return format_; }

		uint32 width()		const { return width_;	}
		uint32 height()		const { return height_;	}
		uint32 depth()		const { return depth_;	}
		uint32 mipLevel()	const { return mipLevels_; }

		bool isCube() const { return isCubemap_; }

		const unsigned char* data(uint32 mip) const;
		const Image* next() const { return next_.get(); }

	private:
		bool loadDDS(core::File& file);

	private:
		ResourceFormat format_;
		bool isCubemap_{ false };
		bool isArray_{ false };
		uint32 width_ = 0;
		uint32 height_ = 0;
		uint32 depth_ = 1;
		uint32 mipLevels_ = 1;
		vector<uint8> data_;
		memory::UniquePtr<Image> next_;
	};

}