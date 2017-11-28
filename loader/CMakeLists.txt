## loader CMakeLists.txt

set(LIBRARY_NAME "loader")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEGUB "-g ${CMAKE_CXX_FLAGS}")

set(CMAKE_ASM_COMPILER as)
set(CMAKE_ASM_FLAGS "--32")
set(CMAKE_ASM_FLAGS_RELEASE "${CMAKE_ASM_FLAGS}")
set(CMAKE_ASM_FLAGS_DEBUG "-g ${CMAKE_ASM_FLAGS}")

add_library(${LIBRARY_NAME} SHARED
	Loader.Linux.cpp
	Loader.Linux.Stub.S
	)

target_link_libraries(${LIBRARY_NAME}
	rt
	dl
	disablesse
	)

# disable -${LIBRARY_NAME}_EXPORTS
set_target_properties(${LIBRARY_NAME}
	PROPERTIES
	DEFINE_SYMBOL ""
	)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
