CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wno-unused-variable -Wno-sign-compare -I. -I..

test: test-roundtrip
	./test-roundtrip

test-roundtrip: test-roundtrip.cpp ../map-parser-tables.cpp ../macro-encode.cpp ../macro-decode.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test-micro-test: test-micro-test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test-framework: test-micro-test
	./test-micro-test

clean:
	rm -f test-roundtrip test-framework-check

.PHONY: test test-framework clean
