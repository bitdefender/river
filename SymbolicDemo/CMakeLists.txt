## SymbolicDemo CMakeLists.txt

set(EXECUTABLE_NAME symbolic_demo)
set(LIBRARY_NAME z3executor)
set(CROSS_FLAGS "-D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11 ${CROSS_FLAGS}")

link_directories(${CMAKE_SOURCE_DIR}/z3/z3-4.4.1-x86-ubuntu-14.04/bin)
add_executable(${EXECUTABLE_NAME}
	SymbolicDemoMain.cpp
	TrackingCookie.cpp
	Payload.cpp
	)

add_library(${LIBRARY_NAME}
	../SymbolicEnvironment/LargeStack.cpp
	Z3SymbolicExecutor.cpp
	Z3SymbolicFlags.cpp
	)


target_link_libraries(${EXECUTABLE_NAME}
	${LIBRARY_NAME}
	)

target_link_libraries(${LIBRARY_NAME}
	z3
	symbolicenvironment
	execution
	)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
