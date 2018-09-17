cmake_minimum_required(VERSION 2.8)
set(CMAKE_SYSTEM_NAME Linux)
project(RIVER C CXX ASM)

set(CMAKE_ASM_CREATE_SHARED_LIBRARY ${CMAKE_C_CREATE_SHARED_LIBRARY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)

set(CMAKE_POSITION_INDEPENDENT_CODE false)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS}")

# build targets
add_subdirectory(CommonCrossPlatform)
add_subdirectory(BinLoader)
add_subdirectory(VirtualMemory)
add_subdirectory(wrapper.setup)
add_subdirectory(revtracer-wrapper)
add_subdirectory(revtracer)
add_subdirectory(ipclib)
add_subdirectory(Execution)
add_subdirectory(loader)
add_subdirectory(SymbolicEnvironment)
add_subdirectory(benchmarking-payload)
