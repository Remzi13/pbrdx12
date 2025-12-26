set(Core_LIB "core")

set(Core_SRC
	debug.h
	std_types.h
	memory.h
	memory.cpp
)

add_library( ${Core_LIB} ${Core_SRC})

source_group( "Core" FILES ${Core_SRC})

SET_PROPERTY( TARGET ${Core_LIB} PROPERTY FOLDER "Libs")