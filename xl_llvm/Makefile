# build libnative binary

.PHONY: clean
CFLAGS  += -Iinclude -I"$(JAVA_HOME)/include" -I"$(JAVA_HOME)/include/linux" -fPIC -fembed-bitcode
LDFLAGS += -shared

OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

%.o: %.c
	clang-16 -c $< $(CFLAGS) -o $@

libnative.so: $(OBJS)
	clang-16 -o $@ $(LDFLAGS) $(OBJS)
	llvm-objcopy-16 --dump-section .llvmbc=- libnative.so > libnative.bc
	cat libnative.bc | llvm-dis-16 > libnative.ll

clean:
	rm -f libnative.so libnative.bc libnative.ll $(OBJS)
