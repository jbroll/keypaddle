.PHONY: test

build:
	arduino-cli compile --fqbn arduino:avr:leonardo .

test:
	cd test; $(MAKE) test

