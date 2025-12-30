set(Render_LIB "render")

set(RENDER_SRC
	common.h
	common.cpp
	render.h
	render.cpp
	fence.h
	fence.cpp
	device.h
	device.cpp
	sync_point.h
	sync_point.cpp
	command_queue.h
	command_queue.cpp
	command_context.h
	command_context.cpp
	descriptor_heap.h
	descriptor_heap.cpp
	swapchain.h
	swapchain.cpp
	resources.h
	resources.cpp
	texture.h
	texture.cpp
	ringbuffer_allocator.h
	ringbuffer_allocator.cpp
	page_allocator.h
	page_allocator.cpp
	utils.h
	utils.cpp
)

add_library( ${Render_LIB} ${RENDER_SRC})

target_link_libraries(${Render_LIB} debug dxguid.lib)

source_group( "Render" FILES ${RENDER_SRC})

SET_PROPERTY( TARGET ${Render_LIB} PROPERTY FOLDER "Libs")