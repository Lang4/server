IF(WIN32)
include_directories(
	${PROJECT_SOURCE_DIR}/Engine/Pack
	${PROJECT_SOURCE_DIR}/Engine/Ext/netbase/win
	${PROJECT_SOURCE_DIR}/Engine/Ext/zlib
)
ENDIF(WIN32)
IF(UNIX)
include_directories(
	${PROJECT_SOURCE_DIR}/Engine/Pack
	${PROJECT_SOURCE_DIR}/Engine/Ext/netbase/inc
	${PROJECT_SOURCE_DIR}/Engine/Ext/zlib
)
ENDIF(UNIX)
set(LIB_HEADERS
	Net.h
	RemoteObject.h
	message.h
)
set(LIB_SRCS 
	Net.cpp
)

add_library(Remote ${LIB_SRCS} ${LIB_HEADERS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set_target_properties(Remote PROPERTIES OUTPUT_NAME "Remote")
