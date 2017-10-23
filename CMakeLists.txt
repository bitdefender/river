cmake_minimum_required(VERSION 2.8)
set(CMAKE_SYSTEM_NAME Linux)
project(RIVER)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)


# build targets
add_subdirectory(BinLoader)
add_subdirectory(VirtualMemory)
add_subdirectory(wrapper.setup)
add_subdirectory(revtracer-wrapper)
#add_subdirectory(revtracer)
add_subdirectory(ipclib)
add_subdirectory(Execution)
add_subdirectory(DisableSSE)
add_subdirectory(loader)
add_subdirectory(ParserPayload)
add_subdirectory(SymbolicEnvironment)
add_subdirectory(SymbolicDemo)
