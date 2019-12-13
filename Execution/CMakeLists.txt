## Execution CMakeLists.txt

set(LIBRARY_NAME execution)
set(FLAGS_CROSS "-D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11 -D_EXECUTION_EXPORTS \
	-DBLOCK_CACHE_READ_ONLY ${FLAGS_CROSS}")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")


include_directories(../BinLoader)
add_library(${LIBRARY_NAME} SHARED
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

target_link_libraries(${LIBRARY_NAME}
		binloader
		wrappersetup
		virtualmemory
		rt
		dl
    )

set_target_properties(${LIBRARY_NAME} PROPERTIES PUBLIC_HEADER "Execution.h")
install(TARGETS ${LIBRARY_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include/Execution
  )
