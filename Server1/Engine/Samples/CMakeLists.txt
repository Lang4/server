
IF(UNIX)
include_directories(
	${PROJECT_SOURCE_DIR}/Engine/Ext/netbase/inc
	${PROJECT_SOURCE_DIR}/Engine/Ext/xml
	${PROJECT_SOURCE_DIR}/Engine/Ext/cjson
	${PROJECT_SOURCE_DIR}/Engine/Pack
	${PROJECT_SOURCE_DIR}/Engine/Remote
	${PROJECT_SOURCE_DIR}/Engine/MysqlOO
	/usr/include/mysql
)
ENDIF(UNIX)
IF(WIN32)
include_directories(
	${PROJECT_SOURCE_DIR}/Engine/Ext/netbase/win
	${PROJECT_SOURCE_DIR}/Engine/Ext/xml
	${PROJECT_SOURCE_DIR}/Engine/Ext/cjson
	${PROJECT_SOURCE_DIR}/Engine/Pack
	${PROJECT_SOURCE_DIR}/Engine/Remote
	${PROJECT_SOURCE_DIR}/Engine/MysqlOO
	${PROJECT_SOURCE_DIR}/Engine/MysqlOO/include
)
ENDIF(WIN32)

IF(UNIX)
LINK_DIRECTORIES(${PROJECT_BINARY_DIR}/lib /usr/lib64/mysql/)
ENDIF(UNIX)

IF(WIN32)
LINK_DIRECTORIES(${PROJECT_BINARY_DIR}/lib 
				 ${PROJECT_SOURCE_DIR}/Engine/MysqlOO/lib/debug)
ENDIF(WIN32)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
add_executable(TestXmlPack testXmlPack.cpp)
target_link_libraries(TestXmlPack  xml)

add_executable(TestJsonPack testJsonPack.cpp)
target_link_libraries(TestJsonPack cjson)

add_executable(TestMsgPack testMsgPack.cpp)


add_executable(TestRemote TestRemote.cpp)
IF(UNIX)
target_link_libraries(TestRemote Remote Netbase pthread)
ENDIF(UNIX)
IF(WIN32)
target_link_libraries(TestRemote Remote Netbase)
ENDIF(WIN32)

add_executable(TestMysql testMysql.cpp)
IF(UNIX)
target_link_libraries(TestMysql pthread mysqlclient dl rt zlib ssl mysqloo)
ENDIF(UNIX)

IF(WIN32)
target_link_libraries(TestMysql  mysqloo mysqlclient)
ENDIF(WIN32)


add_executable(TestMsgFunction testMsgFunction.cpp)