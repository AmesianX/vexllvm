BIN_BASE="0x8000000"
CFLAGS=-g -DBIN_BASE="$(BIN_BASE)"
# XXX, MAKES BINARY SIZE EXPLODE
LDFLAGS="-Wl,-Ttext-segment=$(BIN_BASE)"
#LDFLAGS=


OBJDEPS=	vexxlate.o	\
		vexstmt.o	\
		vexsb.o		\
		vexexpr.o	\
		vexop.o		\
		genllvm.o	\
		guestcpustate.o	\
		gueststate.o	\
		vex_dispatch.o	\
		syscalls.o	\
		vexhelpers.o	\
		vexexec.o	\
		symbols.o

ELFDEPS=	elfimg.o	\
		dllib.o		\
		elfsegment.o	\
		elf_main.o

ELFTRACEDEPS=	elfimg.o	\
		dllib.o		\
		elfsegment.o	\
		elf_trace.o

BITCODE_FILES=	bitcode/libvex_amd64_helpers.bc	\
		bitcode/vexops.bc


OBJDIRDEPS=$(OBJDEPS:%=obj/%)
ELFDIRDEPS=$(ELFDEPS:%=obj/%)
ELFTRACEDIRDEPS=$(ELFTRACEDEPS:%=obj/%)

#TODO: use better config options

#VEXLIB="/usr/lib64/valgrind/libvex-amd64-linux.a"
VEXLIB="/usr/local/lib64/valgrind/libvex-amd64-linux.a"
LLVMLDFLAGS=$(shell llvm-config --ldflags)
LLVM_FLAGS_ORIGINAL=$(shell llvm-config --ldflags --cxxflags --libs all)
LLVMFLAGS:=$(shell echo "$(LLVM_FLAGS_ORIGINAL)" |  sed "s/-Woverloaded-virtual//;s/-fPIC//;s/-DNDEBUG//g;s/-O3/ /g;") -Wall $(LDFLAGS)

all: bin/elf_test bin/elf_trace bitcode

bitcode: $(BITCODE_FILES)

clean:
	rm -f obj/* bin/*

tests/traces-bin/% : tests/traces-obj/%.o
	gcc $< -o $@

tests/traces-obj/%.o: tests/traces-src/%.c
	gcc -g -c -o $@ $<

bitcode/%.bc: support/%.c
	llvm-gcc -emit-llvm -O3 -c $< -o $@

tests: test-traces
tests-clean:
	rm -f tests/*-bin/* tests/*-obj/*

TRACEDEPS= nested_call ret_strlen print
TRACEDEPS_PATH=$(TRACEDEPS:%=tests/traces-bin/%)
test-traces: $(TRACEDEPS_PATH)
	tests/traces.sh

bin/elf_trace: $(OBJDIRDEPS) $(ELFTRACEDIRDEPS)
	g++ $(CFLAGS) -ldl  $^ $(VEXLIB) $(LLVMFLAGS) -o $@ $(LDRELOC)

bin/elf_test: $(OBJDIRDEPS) $(ELFDIRDEPS)
	g++ $(CFLAGS) -ldl  $^ $(VEXLIB) $(LLVMFLAGS) -o $@ $(LDRELOC)

obj/%.o: src/%.s
	gcc $(CFLAGS) -c -o $@ $<

obj/%.o: src/%.cc src/%.h
	g++ $(CFLAGS) $(LLVMFLAGS) -c -o $@ $<

obj/%.o: src/%.cc
	g++ $(CFLAGS) $(LLVMFLAGS) -c -o $@ $<

	
