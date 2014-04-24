set(LIB_HEADERS
	Code.h
	LangCode.h
	Lang.h
	LangScript.h
	LCalc.h
	LLib.h
	LNode.h
	Script.h
)
set(LIB_SRCS 
	Code.cpp
	Lang.cpp
)

add_library(lang ${LIB_SRCS} ${LIB_HEADERS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set_target_properties(lang PROPERTIES OUTPUT_NAME "lang")
