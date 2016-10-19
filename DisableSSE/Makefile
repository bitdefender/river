disable_sse_so := libdisablesse.so
CC := gcc
CXX := g++
NO_SSE := -mno-mmx -mno-sse -march=i386
CC_FLAGS += -m32 $(NO_SSE) $(CC_FLAGS_CROSS)
prefix := /usr/local

ASM_FILES := DisableSSE.S
C_FILES := DisableSSEHelper.c
OBJ_FILES :=  DisableSSE.o  DisableSSEHelper.o

all: $(disable_sse_so)

install: $(disable_sse_so)
	install -m 0755 $(disable_sse_so) -t $(prefix)/lib -D

$(disable_sse_so): $(OBJ_FILES)
	$(CC) $(CC_FLAGS) -shared -o $@ $^

DisableSSEHelper.o: DisableSSEHelper.c
	$(CC) $(CC_FLAGS) -c -o $@ $<

DisableSSE.o: DisableSSE.S
	$(CC) $(CC_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(disable_sse_so) $(OBJ_FILES)
