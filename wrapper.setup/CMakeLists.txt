## wrapper.setup CMakeLists.txt

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11")

add_library(libwrappersetup STATIC
	Setup.Linux.cpp
	Setup.Windows.cpp
	)

install(TARGETS libwrappersetup DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
