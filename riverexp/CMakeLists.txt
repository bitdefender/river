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
message("z3 root is $ENV{Z3_ROOT}")
message("build type ${CMAKE_BUILD_TYPE}")
if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
  message("build is debug")
  set(CMAKE_BINARY_DIR build_debug)
else ()
  set(CMAKE_BINARY_DIR build_release)
endif()

include_directories(${Z3_ROOT}/${CMAKE_BUILD_TYPE}/include)
find_library (z3 
              NAMES libz3.so 
              HINTS $ENV{Z3_ROOT}/${CMAKE_BUILD_TYPE}/lib
              NO_DEFAULT_PATH)

message("z3 lib path ${z3}")
set(EXTRA_LIBS ${EXTRA_LIBS} ${z3})
#--------------------------------------------------------------


# add the executable
add_executable(Riverexp riverexp.cpp)
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
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g" )

set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS})
set(CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS})

#target_include_directories(c_example PRIVATE ${Z3_C_INCLUDE_DIRS})
#target_link_libraries(c_example PRIVATE ${Z3_LIBRARIES})

