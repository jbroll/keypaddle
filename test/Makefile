CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wno-unused-variable -Wno-sign-compare -I. -I..

test: test-macros test-execution
	./test-macros
	./test-execution

test-macros: test-macros.cpp ../map-parser-tables.cpp ../macro-encode.cpp ../macro-decode.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test-execution: test-execution.cpp Keyboard.h ../map-parser-tables.cpp ../macro-encode.cpp ../macro-engine.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test-micro-test: test-micro-test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test-framework: test-micro-test
	./test-micro-test

clean:
	rm -f test-macros test-execution test-micro-test

.PHONY: test test-framework clean
