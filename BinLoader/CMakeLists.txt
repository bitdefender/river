## BinLoader CMakeLists.txt

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11")

add_library(libbinloader STATIC
	Common.cpp
	ELF.Loader.cpp
	PE.Loader.cpp
	Unified.Loader.cpp
	LoaderAPI.cpp
	Inproc.Mapper.cpp
	Shm.Mapper.cpp
	Inproc.Native.Importer.cpp
	)

install(TARGETS libbinloader DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

