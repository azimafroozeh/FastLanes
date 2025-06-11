CXX = g++
CXXFLAGS = -std=c++17
LDFLAGS = -lstdc++fs
SRC = jpeg_decode_to_csv.cpp
TARGET = jpeg_decode_to_csv
TMPDIR = tmp

# default dir
INPUT_DIR ?= ./example/example_image
OUTPUT_DIR ?= ./tmp

all: $(TMPDIR) $(TARGET)

$(TMPDIR):
	mkdir -p $(TMPDIR)

$(TARGET): $(SRC)
	mkdir -p $(dir $(TARGET))
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET) $(INPUT_DIR) $(OUTPUT_DIR)

clean:
	rm -rf $(TARGET) $(TMPDIR)

.PHONY: all clean run
