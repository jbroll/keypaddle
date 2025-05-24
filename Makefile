
build:
	arduino-cli compile --fqbn arduino:avr:leonardo --verbose .

test:
	cd test; $(MAKE) test

