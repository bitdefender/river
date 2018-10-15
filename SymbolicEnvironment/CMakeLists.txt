## SymbolicEnvironment CMakeLists.txt

set(LIBRARY_NAME symbolicenvironment)
if(WIN32)
set(FLAGS_CROSS "-D_cdecl=\"__cdecl\" \
	-D_stdcall=\"__stdcall\"")
else(WIN32)
set(FLAGS_CROSS "-D_cdecl=\"__attribute__((cdecl))\" \
	-D_stdcall=\"__attribute__((stdcall))\"")
endif()
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

set_target_properties(${LIBRARY_NAME} PROPERTIES
  PUBLIC_HEADER "Environment.h;LargeStack.h;SymbolicEnvironment.h"
  )

install(TARGETS ${LIBRARY_NAME}
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include/SymbolicEnvironment
  )
