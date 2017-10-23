## ipclib CMakeLists.txt

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11")

add_library(libipc SHARED
	ipclib.cpp
	)

install(TARGETS libipc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
