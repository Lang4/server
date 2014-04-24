set(LIB_HEADERS
	db.h
	sqlite3.h
	sqlite3ext.h
)
set(LIB_SRCS 
	sqlite3.c
)

add_library(sqlite3 ${LIB_SRCS} ${LIB_HEADERS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set_target_properties(sqlite3 PROPERTIES OUTPUT_NAME "sqlite3")
