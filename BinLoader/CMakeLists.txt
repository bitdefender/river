## BinLoader CMakeLists.txt

set(LIBRARY_NAME binloader)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")

add_library(${LIBRARY_NAME} STATIC
	Common.cpp
	ELF.Loader.cpp
	PE.Loader.cpp
	Unified.Loader.cpp
	LoaderAPI.cpp
	Inproc.Mapper.cpp
	Shm.Mapper.cpp
	Inproc.Native.Importer.cpp
	)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

