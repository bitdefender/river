## revtracer-wrapper CMakeLists.txt

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11")

add_library(librevtracerwrapper SHARED
	RevtracerWrapper.cpp
	Wrapper.Linux.cpp
	Wrapper.Windows.cpp
	)

target_link_libraries(librevtracerwrapper
	libbinloader
	dl
	)

install(TARGETS librevtracerwrapper DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
