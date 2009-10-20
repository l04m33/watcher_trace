#
# This program supports only x86 and x86_64 systems,
# so I'm assuming that if it's not an x86_64, it's an x86.
#
CFLAGS := -O2 -Wall
ifeq ("$(shell uname -i)", "x86_64")
	CFLAGS += -DX86_64
endif

PLT_ADDR := 0x$(shell objdump -d /bin/bash | grep -P '^0[0-9a-fA-F]*\s*<chdir@plt>' | cut -f1 -d ' ')

all: watcher_trace test 

watcher_trace: watcher_trace.c watcher_trace.h linklist.h
	gcc -o watcher_trace watcher_trace.c -DPLT_ADDR=$(PLT_ADDR) $(CFLAGS)

test:
	make -C ./tests

do_test:
	cd ./tests; make do_test

help:
	echo 'This is Makefile for watcher_trace.'; \
		echo ' '; \
		echo '    all:            All stuff, including tests'; \
		echo '    watcher_trace:  The main program'; \
		echo '    test:           The tests'; \
		echo '    do_test:        Carry out the tests'; \
		echo '    help:           Display this message'; \
		echo '    clean:          clean up the source tree';

clean:
	rm -f watcher_trace; \
		cd ./tests; make clean
