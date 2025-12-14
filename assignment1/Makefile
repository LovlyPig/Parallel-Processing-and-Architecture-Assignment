# Makefile
CXX = g++
CFLAGS = -Wall -o -std=c++17
LDFLAGS = -pthread -ltbb

BUILD_DIR = build

SRCS := $(wildcard *.cpp) $(wildcard program2/*.cpp)
TARGETS := $(notdir $(SRCS:.cpp=))
BUILDS := $(addprefix $(BUILD_DIR)/, $(TARGETS))

all: $(BUILDS)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: program2/%.cpp | $(BUILD_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%: $(BUILD_DIR)/%.o
	$(CXX) $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
