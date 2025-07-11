CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wno-unused-variable -Wno-sign-compare -I. -I..

test: clean test-macros 			\
				test-execution 		\
				test-storage 		\
				test-serial 		\
				test-parsing 		\
				test-chord-storage 	\
				test-chord-timing 	\
				test-chord-states

test-macros: test-macros.cpp Arduino.cpp ../map-parser-tables.cpp ../macro-encode.cpp ../macro-decode.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
	./$@

test-execution: test-execution.cpp Arduino.cpp Keyboard.h ../map-parser-tables.cpp ../macro-encode.cpp ../macro-engine.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
	./$@

test-storage: test-storage.cpp Arduino.cpp ../storage.cpp ../map-parser-tables.cpp ../macro-encode.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
	./$@

test-serial: test-serial.cpp \
				Arduino.cpp \
				../storage.cpp ../chordStorage.cpp \
				../chording.cpp \
				../map-parser-tables.cpp \
				../macro-engine.cpp ../macro-encode.cpp ../macro-decode.cpp \
				../serial-interface.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
	./$@

test-parsing: test-parsing.cpp \
				Arduino.cpp \
				../storage.cpp ../map-parser-tables.cpp \
				../macro-encode.cpp ../macro-decode.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
	./$@

test-chord-storage: test-chord-storage.cpp \
				Arduino.cpp \
				../storage.cpp ../chordStorage.cpp \
				../map-parser-tables.cpp ../macro-encode.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
	./$@

test-chord-timing: test-chord-timing.cpp \
				Arduino.cpp \
				../storage.cpp ../chordStorage.cpp \
				../chording.cpp \
				../map-parser-tables.cpp ../macro-encode.cpp ../macro-engine.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
	./$@

test-chord-states: test-chord-states.cpp \
				Arduino.cpp \
				../storage.cpp ../chordStorage.cpp \
				../chording.cpp \
				../map-parser-tables.cpp ../macro-encode.cpp ../macro-engine.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
	./$@

test-micro-test: test-micro-test.cpp Arduino.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test-framework: test-micro-test
	./test-micro-test

clean:
	rm -f test-macros test-execution test-storage test-serial test-parsing test-chord-storage test-micro-test

.PHONY: test test-storage test-framework test-chord-states clean
