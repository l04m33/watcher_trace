all: watcher_trace test 

watcher_trace: watcher_trace.c watcher_trace.h linklist.h
	gcc -o watcher_trace watcher_trace.c \
		-DPLT_ADDR=0x`objdump -d /bin/bash | grep -P '^0[0-9a-fA-F]*\s*<chdir@plt>' | cut -f1 -d ' '`

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
