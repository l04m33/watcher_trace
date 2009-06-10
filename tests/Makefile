test: list_test

do_test: list_test list_case*.exp
	for i in ./list_case*.exp ; do $$i ; done

list_test: list_test.c ../linklist.h
	gcc -o list_test list_test.c -I ../

clean:
	rm -f list_test

