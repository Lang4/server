
include_directories(
	${PROJECT_SOURCE_DIR}/Log
	${PROJECT_SOURCE_DIR}/Log/Log4cxx/include
)


set(LIB_HEADERS
	Logger.h
)
set(LIB_SRCS 
	Logger.cpp
)

add_library(logger ${LIB_SRCS} ${LIB_HEADERS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)


set_target_properties(logger PROPERTIES OUTPUT_NAME "logger")

