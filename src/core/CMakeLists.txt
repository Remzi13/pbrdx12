set(CORE_LIB "core")
set(CMAKE_CXX_STANDARD 20)

set(CORE_SRC
	debug.h
	std_types.h
	std_types.cpp
	memory.h
	memory.cpp
	math_utils.h
	math_utils.cpp
	colors.h
	colors.cpp
	file_system.h
	file_system.cpp
	string_utils.h
	string_utils.cpp
)

source_group( "src" FILES ${CORE_SRC} )

add_library( ${CORE_LIB} ${CORE_SRC} )

SET_PROPERTY( TARGET ${CORE_LIB} PROPERTY FOLDER "Libs"  )

target_include_directories (${CORE_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

