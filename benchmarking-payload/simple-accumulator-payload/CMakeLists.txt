set(LIBRARY_NAME "simple-accumulator")

set(CMAKE_C_FLAGS "-m32 -g")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -D_PAYLOAD_EXPORTS")

include_directories($ENV{RIVER_SDK_DIR}/include)

add_library(${LIBRARY_NAME} SHARED
	simple-accumulator.c
	Payload.cpp
	)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
