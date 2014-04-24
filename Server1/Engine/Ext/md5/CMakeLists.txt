set(LIB_HEADERS
	md5.h
)
set(LIB_SRCS 
	md5.cpp
)

add_library(md5 ${LIB_SRCS} ${LIB_HEADERS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set_target_properties(md5 PROPERTIES OUTPUT_NAME "md5")

