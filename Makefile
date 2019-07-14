aq1: main.cpp
	g++ -o $@ -std=c++17 -g3 -O0 -Wall $^ -lreadline

test: aq1 test.sh
	./test.sh

.PHONY: test
