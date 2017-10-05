symbolic_demo := symbolic_demo

SRC_FILES := SymbolicDemoMain.cpp ../SymbolicEnvironment/LargeStack.cpp Payload.cpp  \
	TrackingCookie.cpp Z3SymbolicExecutor.cpp Z3SymbolicFlags.cpp
OBJ_FILES := $(addprefix ,$(SRC_FILES:.cpp=.o))

INCLUDE_PATHS :=

CROSS_FLAGS = -D__cdecl="" -D__stdcall=""
CXX_FLAGS += -g -m32 -std=c++11 $(INCLUDE_PATHS) $(CROSS_FLAGS)

LDFLAGS := -lz3 -lsymbolicenvironment -lexecution
LIB_PATH := -L../z3/z3-4.4.1-x86-ubuntu-14.04/bin -L../SymbolicEnvironment/ -L../Execution

prefix := /usr/local

all: $(symbolic_demo)

install: $(symbolic_demo)
	install -m 0755 $(symbolic_demo) -t $(prefix)/bin -D

$(symbolic_demo): $(OBJ_FILES)
	$(CXX) $(CXX_FLAGS) $(LIB_PATH) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(symbolic_demo) $(OBJ_FILES)
