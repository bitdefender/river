## revtracer-wrapper CMakeLists.txt

set(LIBRARY_NAME "revtracerwrapper")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11")

add_library(${LIBRARY_NAME} SHARED
	RevtracerWrapper.cpp
	Wrapper.Linux.cpp
	Wrapper.Windows.cpp
	)

target_link_libraries(${LIBRARY_NAME}
	binloader
	dl
	)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
