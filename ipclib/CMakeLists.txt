## ipclib CMakeLists.txt

set(LIBRARY_NAME ipc)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11")

add_library(${LIBRARY_NAME} SHARED
	ipclib.cpp
	)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
