IF(UNIX)
include_directories(
	${PROJECT_SOURCE_DIR}/Engine/MysqlOO
	/usr/include/mysql	
)
ENDIF(UNIX)
IF(WIN32)
include_directories(
	${PROJECT_SOURCE_DIR}/Engine/MysqlOO
	${PROJECT_SOURCE_DIR}/Engine/MysqlOO/include
)
ENDIF(WIN32)

set(LIB_HEADERS
	mysqlDB.h
)
set(LIB_SRCS 
	mysqlDB.cpp
)
IF(WIN32)
LINK_DIRECTORIES(${PROJECT_BINARY_DIR}/lib 
				 ${PROJECT_SOURCE_DIR}/Engine/MysqlOO/lib/debug)
ENDIF(WIN32)

add_library(mysqloo ${LIB_SRCS} ${LIB_HEADERS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
target_link_libraries(mysqloo mysqlclient)
set_target_properties(mysqloo PROPERTIES OUTPUT_NAME "mysqloo")

