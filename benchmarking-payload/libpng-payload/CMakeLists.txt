set(LIBRARY_NAME png-payload)
set(CMAKE_C_FLAGS "-m32 -g")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -nostdlib -D_PAYLOAD_EXPORTS")

add_library(${LIBRARY_NAME} SHARED
	Payload.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libpng-src
	$ENV{RIVER_SDK_DIR}/include
	)

target_link_libraries(${LIBRARY_NAME}
	${CMAKE_CURRENT_SOURCE_DIR}/libpng-src/.libs/libpng16.a
	z
)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
