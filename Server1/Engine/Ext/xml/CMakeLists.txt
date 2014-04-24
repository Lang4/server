set(LIB_HEADERS
	tinyxml2.h
)
set(LIB_SRCS 
	tinyxml2.cpp
)

add_library(xml ${LIB_SRCS} ${LIB_HEADERS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set_target_properties(xml PROPERTIES OUTPUT_NAME "xml")
