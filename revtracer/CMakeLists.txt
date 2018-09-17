## revtracer CMakeLists.txt

set(CMAKE_VERBOSE_MAKEFILE true)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)

set(FLAGS_CROSS "-D_cdecl=\"__attribute__((cdecl))\" \
	-D_stdcall=\"__attribute__((stdcall))\"")
set(CMAKE_CXX_FLAGS "-g -m32 -march=i386 -fno-exceptions \
	-fno-stack-protector -fcheck-new -std=c++11 ${FLAGS_CROSS} \
	-D_BUILDING_REVTRACER_DLL")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} -D_REVTRACER_DEBUG_PRINT_")

set(CMAKE_SHARED_LINKER_FLAGS "--disable-stdcall-fixup -static-libstdc++")

set(CMAKE_ASM_COMPILER i686-w64-mingw32-as)
set(CMAKE_ASM_FLAGS "--32")
set(CMAKE_ASM_FLAGS_RELEASE "${CMAKE_ASM_FLAGS}")
set(CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS} -g")

set_source_files_properties(revtracer.stub.S PROPERTIES COMPILE_FLAGS "${CMAKE_ASM_FLAGS}")

add_library(revtracer SHARED
	AddressContainer.cpp
	api.cpp
	callgates.cpp
	cb.cpp crc32.cpp
	crtStub.cpp
	execenv.cpp
	GenericX86Assembler.cpp
	mm.cpp modrm32.cpp
	PreTrackingX86Assembler.cpp
	RelocableCodeBuffer.cpp
	revtracer.cpp
	RiverAddress.cpp
	TranslatorUtil.cpp
	RiverMetaTranslator.cpp
	RiverRepTranslator.cpp
	RiverRepAssembler.cpp
	RiverPrintTable.cpp
	RiverReverseTranslator.cpp
	RiverSaveTranslator.cpp
	RiverTrackingX86Assembler.cpp
	RiverX86Assembler.cpp
	RiverX86Disassembler.cpp
	NativeX86Assembler.cpp
	SymbopReverseTranslator.cpp
	SymbopTranslator.cpp
	SymbopSaveTranslator.cpp
	sync.cpp
	Tracking.cpp
	TrackingX86Assembler.cpp
	X86Assembler.cpp
	X86AssemblerFuncs.cpp
	RiverX86DisassemblerTable.cpp
	codegen.cpp
	revtracer.stub.S
	)

# disable -revtracer_EXPORTS
set_target_properties(revtracer
	PROPERTIES
	DEFINE_SYMBOL ""
	NO_SONAME ON
	PREFIX ""
	SUFFIX ".dll"
  PUBLIC_HEADER "DebugPrintFlags.h;RiverAddress.h;common.h;environment.h;revtracer.h;river.h"
	)

install(TARGETS revtracer
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include/revtracer)
