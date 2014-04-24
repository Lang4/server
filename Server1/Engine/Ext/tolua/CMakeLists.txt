include_directories(
	${PROJECT_SOURCE_DIR}/Ext/lua
)
set(LIB_HEADERS
	tolua_event.h
	tolua++.h
	tolua_fix.h
)
set(LIB_SRCS 
	tolua_event.c
	tolua_is.c
	tolua_map.c
	tolua_push.c
	tolua_to.c
	tolua_fix.c
)

add_library(tolua ${LIB_SRCS} ${LIB_HEADERS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set_target_properties(tolua PROPERTIES OUTPUT_NAME "tolua")

