#pragma once

#include <memory>

namespace memory {

	constexpr float BytesToKiloBytes = 1.0f / (1 << 10);
	constexpr float BytesToMegaBytes = 1.0f / (1 << 20);
	constexpr float BytesToGigaBytes = 1.0f / (1 << 30);

	template<typename T>
	T alignUp(T value, T alignment)
	{
		return (value + ((T)alignment - 1)) & ~(alignment - 1);
	}

	template<class T>
	using SharedPtr = std::shared_ptr<T>;

	template <class T>
	using UniquePtr = std::unique_ptr<T>;

	template<class T, class _Alloc = std::allocator<T>, class... _Types>
	SharedPtr<T> makeShared(_Types&&... _Args)
	{
		static const _Alloc alloc;
		return std::allocate_shared<T, _Alloc>(alloc, std::forward<_Types>(_Args)...);
	}

	template<class T, class _Alloc = std::allocator<T>, class... _Types>
	UniquePtr<T> makeUnique(_Types&&... _Args)
	{
		return std::make_unique<T>(_Args...);
		//		return std::allocate_shared<T, _Alloc>(alloc, std::forward<_Types>(_Args)...);
	}
}