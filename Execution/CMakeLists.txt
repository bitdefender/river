## Execution CMakeLists.txt

set(LIBRARY_NAME execution)
if(WIN32)
set(FLAGS_CROSS "-D_cdecl=\"__cdecl\" \
	-D_stdcall=\"__stdcall\"")
else(WIN32)
set(FLAGS_CROSS "-D_cdecl=\"__attribute__((cdecl))\" \
	-D_stdcall=\"__attribute__((stdcall))\"")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11 -D_EXECUTION_EXPORTS \
	-DBLOCK_CACHE_READ_ONLY ${FLAGS_CROSS}")

include_directories(../BinLoader)
add_library(${LIBRARY_NAME} SHARED
	Debugger.cpp
	ExternExecutionController.Linux.cpp
	ExternExecutionController.Windows.cpp
	DualAllocator.Linux.cpp
	DualAllocator.Windows.cpp
	TokenRingInit.Linux.cpp
	TokenRingInit.Windows.cpp
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

if (UNIX AND NOT APPLE)
set (OS_LIBS rt dl)
else ()
set (OS_LIBS ntdll)
endif ()

target_link_libraries(${LIBRARY_NAME}
	binloader
	wrappersetup
	loadersetup
	virtualmemory
	${OS_LIBS})

set_target_properties(${LIBRARY_NAME} PROPERTIES PUBLIC_HEADER "Execution.h")
install(TARGETS ${LIBRARY_NAME}
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include/Execution
  )
