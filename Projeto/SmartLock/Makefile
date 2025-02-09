# Makefile for compiling and uploading an Arduino project

# Fully Qualified Board Name
FQBN := esp32:esp32:esp32doit-devkit-v1

# Serial port for uploading
PORT := /dev/ttyUSB0

# Default target
all: compile upload

# Compile the project
compile:
	arduino-cli compile --fqbn $(FQBN) .

# Upload the project
upload:
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) .

# Clean target (optional)
clean:
	rm -rf ./build