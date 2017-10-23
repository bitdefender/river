## VirtualMemory CMakeLists.txt

set(LIBRARY_NAME virtualmemory)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11")

add_library(${LIBRARY_NAME} STATIC
	VirtualMem.cpp
	MemoryLayout.Linux.cpp
	../libproc/libproc.cpp
	)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
