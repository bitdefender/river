## http-parser CMakeLists.txt
# https://software.intel.com/en-us/articles/performance-tools-for-software-developers-bsymbolic-can-cause-dangerous-side-effects

set(LIBRARY_NAME "http-parser")

set(CMAKE_C_FLAGS "-m32 -g -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -nostdlib -std=c++11 -D_PAYLOAD_EXPORTS")

include_directories($ENV{RIVER_SDK_DIR}/include)

add_library(${LIBRARY_NAME} SHARED
	http_parser.c
	Payload.cpp
	)

set_target_properties(${LIBRARY_NAME} PROPERTIES LINK_FLAGS
	"${LINK_FLAGS} \
	-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/http-parser.version")

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
