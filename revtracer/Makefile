lib_revtracer := revtracer.dll
CXX := i686-w64-mingw32-g++
CPP_FILES := AddressContainer.cpp api.cpp callgates.cpp cb.cpp crc32.cpp crtStub.cpp execenv.cpp GenericX86Assembler.cpp mm.cpp modrm32.cpp PreTrackingX86Assembler.cpp RelocableCodeBuffer.cpp revtracer.cpp RiverAddress.cpp RiverMetaTranslator.cpp RiverPrintTable.cpp RiverReverseTranslator.cpp RiverSaveTranslator.cpp RiverTrackingX86Assembler.cpp RiverX86Assembler.cpp RiverX86Disassembler.cpp NativeX86Assembler.cpp SymbopReverseTranslator.cpp SymbopTranslator.cpp SymbopSaveTranslator.cpp sync.cpp Tracking.cpp TrackingX86Assembler.cpp X86Assembler.cpp X86AssemblerFuncs.cpp RiverX86DisassemblerTable.cpp codegen.cpp
OBJ_FILES := $(addprefix ,$(notdir $(CPP_FILES:.cpp=.o)))
LD_FLAGS :=
CC_FLAGS_CROSS = -D_cdecl="" -D_stdcall=""
CC_FLAGS += -g -m32 -O0 -fno-exceptions -fomit-frame-pointer -fno-stack-protector -fcheck-new -std=c++11 -D_BUILDING_REVTRACER_DLL $(CC_FLAGS_CROSS)
prefix := /usr/local

all: $(lib_revtracer)

install: $(lib_revtracer)
	install -m 0755 $(lib_revtracer) -t $(prefix)/lib -D

$(lib_revtracer): $(OBJ_FILES)
	$(CXX) $(CC_FLAGS) --disable-stdcall-fixup -static-libstdc++ -shared -fPIC -o $@ $^

%.o: %.cpp
	$(CXX)  $(CC_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_revtracer) $(OBJ_FILES)
