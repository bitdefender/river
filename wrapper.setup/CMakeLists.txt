## wrapper.setup CMakeLists.txt

set(LIBRARY_NAME wrappersetup)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11")

add_library(${LIBRARY_NAME} STATIC
	Setup.Linux.cpp
	Setup.Windows.cpp
	)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
