CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pthread
SRC_DIR  = src
BUILD_DIR = build

TARGET   = $(BUILD_DIR)/benchmark

SRCS     = $(SRC_DIR)/benchmark.cpp
HEADERS  = $(SRC_DIR)/graph.hpp $(SRC_DIR)/naive.hpp $(SRC_DIR)/kbfs.hpp $(SRC_DIR)/rv.hpp

.PHONY: all clean verify run

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(SRCS) $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

verify: $(TARGET)
	./$(TARGET) --verify

clean:
	rm -rf $(BUILD_DIR) results/
