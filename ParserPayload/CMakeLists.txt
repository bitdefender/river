## http-parser CMakeLists.txt
# https://software.intel.com/en-us/articles/performance-tools-for-software-developers-bsymbolic-can-cause-dangerous-side-effects

set(LIBRARY_NAME "http-parser")

set(CMAKE_C_FLAGS "-m32 -O2 -g")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -D_PAYLOAD_EXPORTS")

add_library(${LIBRARY_NAME} SHARED
	http_parser.c
	Payload.cpp
	)

set_target_properties(${LIBRARY_NAME} PROPERTIES LINK_FLAGS
	"${LINK_FLAGS} \
	-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/http-parser.version")

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
