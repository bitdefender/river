## revtracer-wrapper CMakeLists.txt

set(LIBRARY_NAME "revtracerwrapper")



set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11")

add_library(${LIBRARY_NAME} SHARED
	RevtracerWrapper.cpp
	Wrapper.Linux.cpp
	Wrapper.Windows.cpp
)

target_compile_definitions(${LIBRARY_NAME} PRIVATE _BUILDING_REVTRACER_WRAPPER_DLL)

if (UNIX AND NOT APPLE)
set (OS_LIBS dl)
else ()
set (OS_LIBS)
endif ()

target_link_libraries(${LIBRARY_NAME}
	binloader
	${OS_LIBS}
)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
