## loader CMakeLists.txt

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11")

set(CMAKE_ASM_COMPILER as)
set(CMAKE_ASM_FLAGS "--32")

add_library(libloader SHARED
	Loader.Linux.cpp
	Loader.Linux.Stub.S
	)

target_link_libraries(libloader
	rt
	dl
	libdisablesse
	)

install(TARGETS libloader DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
