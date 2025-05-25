CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wno-unused-variable -Wno-sign-compare -I. -I..

test-roundtrip: test-roundtrip.cpp ../map-parser-tables.cpp ../macro-encode.cpp ../macro-decode.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test: test-roundtrip
	./test-roundtrip

clean:
	rm -f test-roundtrip

.PHONY: test clean