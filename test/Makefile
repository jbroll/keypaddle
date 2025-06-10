CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wno-unused-variable -Wno-sign-compare -I. -I..

test: clean test-macros test-execution test-storage test-serial test-parsing
	./test-macros
	./test-execution
	./test-storage
	./test-serial
	./test-parsing

test-macros: test-macros.cpp Arduino.cpp ../map-parser-tables.cpp ../macro-encode.cpp ../macro-decode.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test-execution: test-execution.cpp Arduino.cpp Keyboard.h ../map-parser-tables.cpp ../macro-encode.cpp ../macro-engine.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test-storage: test-storage.cpp Arduino.cpp EEPROM.cpp ../storage.cpp ../map-parser-tables.cpp ../macro-encode.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test-serial: test-serial.cpp \
				Arduino.cpp Serial.cpp EEPROM.cpp \
				../storage.cpp ../chordStorage.cpp \
				../chording.cpp \
				../map-parser-tables.cpp \
				../macro-engine.cpp ../macro-encode.cpp ../macro-decode.cpp \
				../serial-interface.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test-parsing: test-parsing.cpp \
				Arduino.cpp Serial.cpp EEPROM.cpp \
				../storage.cpp ../map-parser-tables.cpp \
				../macro-encode.cpp ../macro-decode.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test-micro-test: test-micro-test.cpp Arduino.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test-framework: test-micro-test
	./test-micro-test

clean:
	rm -f test-macros test-execution test-storage test-serial test-parsing test-micro-test

.PHONY: test test-framework clean
