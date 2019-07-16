## SymbolicEnvironment CMakeLists.txt

set(LIBRARY_NAME symbolicenvironment)
set(FLAGS_CROSS "-D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11 \
	-D_BUILDING_ENVIRONMENT_DLL -D_NO_TRACK_CALLBACKS_ ${FLAGS_CROSS}")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")


add_library(${LIBRARY_NAME} SHARED
	Environment.cpp
	LargeStack.cpp
	OverlappedRegisters.cpp
	RevSymbolicEnvironment.cpp
	SymbolicEnvironment.cpp
	../revtracer/Tracking.cpp
	../revtracer/AddressContainer.cpp
	)

set_target_properties(${LIBRARY_NAME} PROPERTIES
  PUBLIC_HEADER "Environment.h;LargeStack.h;SymbolicEnvironment.h"
  )

install(TARGETS ${LIBRARY_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include/SymbolicEnvironment
  )
