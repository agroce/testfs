all: Tests.cpp *.c *.h
	${AFL_HOME}/afl-clang -c *.c
	${AFL_HOME}/afl-clang++ Tests.cpp -o Tests_AFL *.o -ldeepstate_AFL
	clang -c *.c -fsanitize=fuzzer
	clang++ Tests.cpp -o Tests_LF *.o -ldeepstate_LF -fsanitize=fuzzer
	clang -c *.c
	clang++ Tests.cpp -o Tests *.o -ldeepstate
	clang -c *.c --coverage
	clang++ Tests.cpp -o Tests_cov *.o -ldeepstate --coverage


