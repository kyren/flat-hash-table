run_test: test
	./test

test: include/*.hpp test.cpp
	c++ -Wall -std=c++14 -Iinclude test.cpp -o test

clean:
	rm -f test
