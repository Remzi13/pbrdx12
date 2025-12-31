set(Render_LIB "render")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/imgui")

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
	imgui_system.cpp
	imgui_system.h
	pipeline_state.h
	pipeline_state.cpp
)

set(IMGUI_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/imgui/imconfig.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/imgui/imgui.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/imgui/imgui.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/imgui/imgui_draw.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/imgui/imgui_demo.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/imgui/imgui_widgets.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/imgui/imgui_tables.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/imgui/imgui_internal.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/imgui/imgui_impl_win32.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/imgui/imgui_impl_win32.cpp"
)

add_library( ${Render_LIB} ${RENDER_SRC} ${IMGUI_SRC})

target_link_libraries(${Render_LIB} debug dxguid.lib)

source_group( "tp/imgui" FILES ${IMGUI_SRC} )
source_group( "src" FILES ${RENDER_SRC})

SET_PROPERTY( TARGET ${Render_LIB} PROPERTY FOLDER "Libs")