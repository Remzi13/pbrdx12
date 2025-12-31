set(RENDER_LIB "render")


set(RENDER_SRC
	render.h
	render.cpp
	rgraph.h
	rgraph.cpp
	utils.cpp
	utils.h	
	formats.h
	image.h
	image.cpp
)

set(DEVICE_SRC
	device/texture.h
	device/allocator.h

	device/device_interface.h
	device/dx12/fence.h
	device/dx12/fence.cpp
	device/dx12/resource.h
	device/dx12/resource.cpp
	device/dx12/page_allocator.h
	device/dx12/page_allocator.cpp
	device/dx12/shader.h
	device/dx12/shader.cpp

	device/dx12/dx12.h
	device/dx12/dx12.cpp
	device/dx12/device.h
	device/dx12/device.cpp
	device/dx12/texture.h
	device/dx12/texture.cpp
	device/dx12/swap_chain.h
	device/dx12/swap_chain.cpp
	device/dx12/command_queue.h
	device/dx12/command_queue.cpp
	device/dx12/command_context.h
	device/dx12/command_context.cpp
	device/dx12/descriptorHeap.h
	device/dx12/descriptorHeap.cpp
	device/dx12/pipeline_state.h
	device/dx12/pipeline_state.cpp
	device/dx12/root_signature.h
	device/dx12/root_signature.cpp
	device/dx12/ringbuffer_allocator.h
	device/dx12/ringbuffer_allocator.cpp
)

add_library( ${RENDER_LIB} ${RENDER_SRC} ${DEVICE_SRC}  )

source_group( "device" FILES ${DEVICE_SRC} )
source_group( "src" FILES ${RENDER_SRC})

SET_PROPERTY( TARGET ${RENDER_LIB} PROPERTY FOLDER "Libs")
message("CMAKE_BINARY_DIR  ${CMAKE_BINARY_DIR}")
target_include_directories (${RENDER_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


