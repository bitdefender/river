lib_symbolicenvironment := libsymbolicenvironment.so
CC := gcc
CXX := g++
EXTERNAL_FILES := ../revtracer/Tracking.cpp ../revtracer/AddressContainer.cpp
CPP_FILES := Environment.cpp LargeStack.cpp OverlappedRegisters.cpp RevSymbolicEnvironment.cpp SymbolicEnvironment.cpp $(EXTERNAL_FILES)
OBJ_FILES := $(addprefix ,$(CPP_FILES:.cpp=.o))
CC_FLAGS += -g -m32 -march=i386 -std=c++11 -D_BUILDING_ENVIRONMENT_DLL -D_NO_TRACK_CALLBACKS_
prefix := /usr/local

all: $(lib_symbolicenvironment)

install: $(lib_symbolicenvironment)
	install -m 0755 $(lib_symbolicenvironment) -t $(prefix)/lib -D

$(lib_symbolicenvironment): $(OBJ_FILES)
	$(CXX) $(CC_FLAGS) -shared -o $@ $^

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_symbolicenvironment) $(OBJ_FILES)
