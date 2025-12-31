#pragma once

#include "render/common.h"

namespace render {
	DXGI_FORMAT convertFormat(ResourceFormat format);
	ResourceState convertState(D3D12_RESOURCE_STATES state);

	template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
	constexpr inline bool enumHasAnyFlags(Enum Flags, Enum Contains)
	{
		return (((__underlying_type(Enum))Flags) & (__underlying_type(Enum))Contains) != 0;
	}

	template<typename Enum>
	requires std::is_enum_v<Enum>
	constexpr bool enumHasAllFlags(Enum flags, Enum contains)
	{
		using U = std::underlying_type_t<Enum>;
		return (static_cast<U>(flags) & static_cast<U>(contains)) == static_cast<U>(contains);
	}

	struct FormatInfo
	{
		const char* name;
		ResourceFormat format;
		uint8  BytesPerBlock;
		uint8 BlockSize;
	};

	const FormatInfo& formatInfo(ResourceFormat format);
	uint64 rowPitch(ResourceFormat format, uint32 width, uint32 mipIndex = 0);
	uint64 slicePitch(ResourceFormat format, uint32 width, uint32 height, uint32 mipIndex = 0);

}