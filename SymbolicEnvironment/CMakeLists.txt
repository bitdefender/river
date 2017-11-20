## SymbolicEnvironment CMakeLists.txt

set(LIBRARY_NAME symbolicenvironment)
set(FLAGS_CROSS "-D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11 \
	-D_BUILDING_ENVIRONMENT_DLL -D_NO_TRACK_CALLBACKS_ ${FLAGS_CROSS}")

add_library(${LIBRARY_NAME} SHARED
	Environment.cpp
	LargeStack.cpp
	OverlappedRegisters.cpp
	RevSymbolicEnvironment.cpp
	SymbolicEnvironment.cpp
	../revtracer/Tracking.cpp
	../revtracer/AddressContainer.cpp
	)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
