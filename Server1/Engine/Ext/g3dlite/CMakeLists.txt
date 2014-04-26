set(LIB_HEADERS
	inc/AABox.h
	inc/Array.h
	inc/Box.h
	inc/CollisionDetection.h
	inc/CoordinateFrame.h
	inc/Crypto.h
	inc/debug.h
	inc/format.h
	inc/g3dmath.h
	inc/g3dmath.inl
	inc/GCamera.h
	inc/Line.h
	inc/Matrix3.h
	inc/Plane.h
	inc/platform.h
	inc/Quat.h
	inc/Quat.inl
	inc/Ray.h
	inc/RegistryUtil.h
	inc/Sphere.h
	inc/stringutils.h
	inc/System.h
	inc/Table.h
	inc/Triangle.h
	inc/Vector2.h
	inc/Vector2.inl
	inc/Vector2int16.h
	inc/Vector3.h
	inc/Vector3.inl
	inc/Vector3int16.h
	inc/Vector4.h
	inc/Vector4.inl
)
set(LIB_SRCS 
	src/AABox.cpp
	src/Box.cpp
	src/Crypto.cpp
	src/format.cpp
	src/license.html
	src/Matrix3.cpp
	src/Plane.cpp
	src/System.cpp
	src/Triangle.cpp
	src/Vector3.cpp
	src/Vector4.cpp
)
include_directories(
	inc
)
add_library(g3dlite ${LIB_SRCS} ${LIB_HEADERS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set_target_properties(g3dlite PROPERTIES OUTPUT_NAME "g3dlite")
