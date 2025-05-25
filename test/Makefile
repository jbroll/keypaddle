# test/Makefile

CXX = g++
CXXFLAGS = -I. -I.. -std=c++11 -Wall -Wextra -g

# Source files from parent directory
PARSER_SOURCES = ../macro-encode.cpp ../macro-decode.cpp ../map-parser-tables.cpp

# Test executables
TESTS = test-macro-encode-decode

.PHONY: all test clean

all: $(TESTS)

test-macro-encode-decode: test-macro-encode-decode.cpp $(PARSER_SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TESTS)
	@echo "Running macro encode/decode tests..."
	./test-macro-encode-decode

clean:
	rm -f $(TESTS)