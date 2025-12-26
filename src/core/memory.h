#pragma once

#include <memory>

namespace memory {

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