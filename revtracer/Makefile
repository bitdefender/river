lib_revtracer := revtracer.dll
CC := i686-w64-mingw32-gcc
CXX := i686-w64-mingw32-g++
CPP_FILES := AddressContainer.cpp api.cpp callgates.cpp cb.cpp crc32.cpp crtStub.cpp execenv.cpp GenericX86Assembler.cpp mm.cpp modrm32.cpp PreTrackingX86Assembler.cpp RelocableCodeBuffer.cpp revtracer.cpp RiverAddress.cpp RiverMetaTranslator.cpp RiverPrintTable.cpp RiverReverseTranslator.cpp RiverSaveTranslator.cpp RiverTrackingX86Assembler.cpp RiverX86Assembler.cpp RiverX86Disassembler.cpp NativeX86Assembler.cpp SymbopReverseTranslator.cpp SymbopTranslator.cpp SymbopSaveTranslator.cpp sync.cpp Tracking.cpp TrackingX86Assembler.cpp X86Assembler.cpp X86AssemblerFuncs.cpp RiverX86DisassemblerTable.cpp
OBJ_FILES := $(addprefix ,$(notdir $(CPP_FILES:.cpp=.o)))
LD_FLAGS :=
CC_FLAGS_CROSS =-D_cdecl="" -D_stdcall=""
CC_FLAGS += -m32 -std=c++11 -fcheck-new -D_BUILDING_REVTRACER_DLL $(CC_FLAGS_CROSS)

all: $(lib_revtracer)

$(lib_revtracer): $(OBJ_FILES)
	$(CXX) -shared -o $@ \
		-Wl,--export-all-symbols \
		-Wl,--enable-auto-import $^

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_revtracer) $(OBJ_FILES)

CC_FLAGS += -MMD
-include $(OBJFILES:.o=.d)
