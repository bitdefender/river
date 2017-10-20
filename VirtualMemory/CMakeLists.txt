## VirtualMemory CMakeLists.txt

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11")

add_library(libvirtualmemory STATIC
	VirtualMem.cpp
	MemoryLayout.Linux.cpp
	../libproc/libproc.cpp
	)

install(TARGETS libvirtualmemory DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
