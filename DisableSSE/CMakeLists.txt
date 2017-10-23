## disablesse CMakeLists.txt

set(CMAKE_C_FLAGS "-m32 -mno-mmx -mno-sse -march=i386")

add_library(libdisablesse SHARED
	DisableSSE.S
	DisableSSEHelper.c
	)

install(TARGETS libdisablesse DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
