cmake_minimum_required (VERSION 2.6)
project (RiverExp)

set (Riverexp_VERSION_MAJOR 1)
set (Riverexp_VERSION_MINOR 0)

#set(STRING_VARIABLE "value")
#set(LIST_VARIABLE "value1 value2")
#message("The value of str var is ${STRING_VARIABLE}")


# Test if system has log and exp functions
include (CheckFunctionExists)
check_function_exists (log HAVE_LOG)
check_function_exists (exp HAVE_EXP)
check_function_exists (printf PRINTF_EXISTS)
 

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/RiverexpConfig.h.in"
  "${PROJECT_BINARY_DIR}/RiverexpConfig.h"
  )

option(USE_MYMATH "use tutorial provided math impl" OFF)

# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}")

# add the MathFunctions library? ?
if (USE_MYMATH)
	include_directories("${PROJECT_SOURCE_DIR}/MathFunctions")
	add_subdirectory(MathFunctions)
	set(EXTRA_LIBS ${EXTRA_LIBS} MathFunctions)
endif (USE_MYMATH)

# --------- Establish where z3 library should be -------------
if (NOT DEFINED ENV{Z3_ROOT})
      set(ENV{Z3_ROOT} "~/z3")
	    # message(FATAL_ERROR "Specify Z3_ROOT variable")
      message("Z3_ROOT seems to be not defined but maybe you are not using bash so it can't read from bashrc file. However i will default to ~/z3 and maybe you're lucky and we find it there")
else()
    	set(Z3_ROOT ENV{Z3_ROOT})
endif()


message("z3 root is $ENV{Z3_ROOT}")
message("build type ${CMAKE_BUILD_TYPE}")


if(NOT CMAKE_BUILD_TYPE) 
    set(CMAKE_BUILD_TYPE Release)
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
  message("build is debug")
  set(CMAKE_BINARY_DIR build_debug)
else ()
  set(CMAKE_BINARY_DIR build_release)
endif()

set(Z3_ROOT_INCLUDE "$ENV{Z3_ROOT}/${CMAKE_BUILD_TYPE}/include")
set(Z3_ROOT_LIBS "$ENV{Z3_ROOT}/${CMAKE_BUILD_TYPE}/lib")

include_directories(${Z3_ROOT_INCLUDE})
find_library (z3 
              NAMES libz3.so 
              HINTS ${Z3_ROOT_LIBS}
              NO_DEFAULT_PATH)

add_subdirectory (river.format/format.handler)
add_subdirectory (river.format/logger)
include_directories(river.format/include)

message("z3 lib path ${z3}")
set(EXTRA_LIBS ${EXTRA_LIBS} ${z3} format.handler)
#--------------------------------------------------------------

# add the executable
add_executable(Riverexp riverexp.cpp src/constraintsSolver.cpp 
              src/concolicExecutor.cpp 
              src/utils.cpp
              src/tracerExecutionStrategyExternal.cpp src/tracerExecutionStrategyMPI.cpp src/tracerExecutionStrategyIPC.cpp)
target_link_libraries(Riverexp ${EXTRA_LIBS})


message("All libs ${EXTRA_LIBS}")

install (TARGETS Riverexp DESTINATION bin)
install (FILES "${PROJECT_BINARY_DIR}/RiverexpConfig.h" DESTINATION include)

#define a macro to simplify adding tests, then use it
macro (do_test arg result)
  add_test (TutorialComp${arg} Riverexp ${arg})
  set_tests_properties (TutorialComp${arg}
    PROPERTIES PASS_REGULAR_EXPRESSION ${result})
endmacro (do_test)

# ---------------
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -pthread -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -pthread -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g" )

set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS})
set(CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS})

#target_include_directories(c_example PRIVATE ${Z3_C_INCLUDE_DIRS})
#target_link_libraries(c_example PRIVATE ${Z3_LIBRARIES})

#set(THREADS_PREFER_PTHREAD_FLAG ON)
#set(CMAKE_THREAD_PREFER_PTHREADS TRUE)
#find_package(Threads REQUIRED)