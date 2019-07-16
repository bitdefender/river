## disablesse CMakeLists.txt

set(LIBRARY_NAME "disablesse")
set(CMAKE_C_FLAGS "-m32 -mno-mmx -mno-sse -march=i386")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")


add_library(${LIBRARY_NAME} SHARED
	DisableSSE.S
	DisableSSEHelper.c
	)

set_target_properties(${LIBRARY_NAME}
	PROPERTIES
	DEFINE_SYMBOL ""
	)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
