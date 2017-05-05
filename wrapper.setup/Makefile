lib_wrappersetup := libwrappersetup.a
CC := gcc
CXX := g++ 
CPP_FILES := Setup.Linux.cpp Setup.Windows.cpp
OBJ_FILES := $(addprefix ,$(notdir $(CPP_FILES:.cpp=.o)))
LD_FLAGS :=
CXX_FLAGS += -g -m32 -std=c++11
CC_FLAGS += -g -m32 -std=c++11
prefix := /usr/local

all: $(lib_wrappersetup)

install: $(lib_wrappersetup)
	install -m 0755 $(lib_wrappersetup) -t $(prefix)/lib -D

$(lib_wrappersetup): $(OBJ_FILES)
	$(AR) rcs $@ $^

%.o: %.cpp
	   $(CXX) $(CXX_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_wrappersetup) $(OBJ_FILES) *.o main
