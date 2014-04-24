set(LIB_HEADERS
	cJSON.h
)
set(LIB_SRCS 
	cJSON.c
)

add_library(cjson ${LIB_SRCS} ${LIB_HEADERS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set_target_properties(cjson PROPERTIES OUTPUT_NAME "cjson")

