.PHONY: test teensy rpipico kb2040

-include CONFIG

build:
	arduino-cli compile --fqbn $(FQBN) .

upload:
	arduino-cli upload --fqbn $(FQBN) -p $(PROG) .

mon:
	arduino-cli monitor --fqbn $(FQBN) -p $(PORT) .

test:
	cd test; $(MAKE) test

pico:
	rm -f hardware
	ln -s hardware.rpipico hardware
	$(MAKE) build

kb2040:
	rm -f hardware
	ln -s hardware.kb2040 hardware
	$(MAKE) build

teensy:
	rm -f hardware
	ln -s hardware.teensy hardware
	$(MAKE) build

# Dependencies for the main sketch
keypaddle.ino.hex: keypaddle.ino \
	config.h \
	switches.h switches.cpp \
	macro-engine.h macro-engine.cpp \
	macro-encode.h macro-encode.cpp \
	macro-decode.h macro-decode.cpp \
	storage.h storage.cpp \
	chordStorage.h chordStorage.cpp \
	serial-interface.h serial-interface.cpp \
	map-parser-tables.h map-parser-tables.cpp \
	commands/readline.h commands/readline.cpp \
	commands/cmd-help.cpp \
	commands/cmd-show.cpp \
	commands/cmd-map.cpp \
	commands/cmd-clear.cpp \
	commands/cmd-load.cpp \
	commands/cmd-save.cpp \
	commands/cmd-modifier.cpp \
	commands/cmd-chord.cpp \
	commands/cmd-stat.cpp

# Clean target
clean:
	rm -rf build/
	cd test && $(MAKE) clean

# Help target
help:
	@echo "Available targets:"
	@echo "  build  - Compile the Arduino sketch"
	@echo "  test   - Run unit tests"
	@echo "  clean  - Clean build artifacts"
	@echo "  help   - Show this help message"
