## disablesse CMakeLists.txt

set(LIBRARY_NAME "disablesse")
set(CMAKE_C_FLAGS "-m32 -mno-mmx -mno-sse -march=i386")

add_library(${LIBRARY_NAME} SHARED
	DisableSSE.S
	DisableSSEHelper.c
	)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
