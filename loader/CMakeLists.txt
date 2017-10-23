## loader CMakeLists.txt

set(LIBRARY_NAME "loader")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11")

set(CMAKE_ASM_COMPILER as)
set(CMAKE_ASM_FLAGS "--32")

add_library(${LIBRARY_NAME} SHARED
	Loader.Linux.cpp
	Loader.Linux.Stub.S
	)

target_link_libraries(${LIBRARY_NAME}
	rt
	dl
	disablesse
	)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
