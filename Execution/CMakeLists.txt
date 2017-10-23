## Execution CMakeLists.txt

set(FLAGS_CROSS "-D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11 -D_EXECUTION_EXPORTS \
	-DBLOCK_CACHE_READ_ONLY ${FLAGS_CROSS}")

include_directories(../BinLoader)
add_library(libexecution SHARED
	Debugger.cpp
	ExternExecutionController.Linux.cpp
	DualAllocator.Linux.cpp
	TokenRingInit.Linux.cpp
	LargeStack.cpp
	CommonExecutionController2.cpp
	InprocessExecutionController.cpp
	CommonExecutionController.cpp
	Main.cpp
	../revtracer/sync.cpp
	../revtracer/crc32.cpp
	../revtracer/cb.cpp
	../libproc/libproc.cpp
	)

target_link_libraries(libexecution
	libbinloader
	libwrappersetup
	libvirtualmemory
	rt)

install(TARGETS libexecution DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
