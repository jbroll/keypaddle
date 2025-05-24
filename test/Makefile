# test/Makefile
test-parser: test-parser.cpp ../map-parser.cpp ../macro-decompiler.cpp ../map-parser-tables.cpp
	g++ -I. -I.. -o test-parser $^

test: test-parser
	./test-parser

clean:
	rm -f test-parser