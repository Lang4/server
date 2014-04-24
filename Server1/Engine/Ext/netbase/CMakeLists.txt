IF(UNIX)
include_directories(
	${PROJECT_SOURCE_DIR}/Engine/Ext/zlib
	${PROJECT_SOURCE_DIR}/Engine/Ext/netbase/inc
)
ENDIF(UNIX)
IF(WIN32)
include_directories(
	${PROJECT_SOURCE_DIR}/Engine/Ext/zlib
	${PROJECT_SOURCE_DIR}/Engine/Ext/netbase/win
)
ENDIF(WIN32)
IF(WIN32)
set(LIB_HEADERS
	win/EncDec.h
	win/LNetService.h
	win/LService.h
	win/LSocket.h
	win/LTCPClientTask.h
	win/LTCPClientTaskPool.h
	win/LTCPServer.h
	win/LTCPTask.h
	win/LTCPTaskPool.h
	win/LThread.h
	win/LTime.h
	win/miniCrypt.h
	win/LIocp.h
	win/LType.h
)
ENDIF(WIN32)
IF(UNIX)
set(LIB_SRCS 
	src/LDesEnc.cpp
	src/LNetService.cpp
	src/LService.cpp
	src/LSocket.cpp
	src/LTCPClientTask.cpp
	src/LTCPClientTaskPool.cpp
	src/LTCPServer.cpp
	src/LTCPTask.cpp
	src/LTCPTaskPool.cpp
	src/LThread.cpp
	src/LTime.cpp
)
ENDIF(UNIX)
IF(WIN32)
set(LIB_SRCS
	win/EncDec.cpp
	win/LNetService.cpp
	win/LService.cpp
	win/LSocket.cpp
	win/LTCPClientTask.cpp
	win/LTCPClientTaskPool.cpp
	win/LTCPServer.cpp
	win/LTCPTask.cpp
	win/LTCPTaskPool.cpp
	win/LThread.cpp
	win/LTime.cpp
	win/miniCrypt.c
	win/LIocp.cpp
)

ENDIF(WIN32)

add_library(Netbase ${LIB_SRCS} ${LIB_HEADERS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
target_link_libraries(Netbase zlib )
set_target_properties(Netbase PROPERTIES OUTPUT_NAME "Netbase")
