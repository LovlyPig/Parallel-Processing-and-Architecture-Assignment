CXX = g++
CXXFLAGS = -std=c++17 -O2 -g -march=native -Wall -Wextra -pthread
LDFLAGS = -ltbb

SRCS := main.cpp
HDRS := blocking_queue.hpp noblocking_queue.hpp tbb_queue.hpp
OBJDIR := .build
OBJS := $(patsubst %.cpp,$(OBJDIR)/%.o,$(SRCS))

TARGET := ./benchmark

.PHONY: all clean run

all: $(TARGET)

# ensure obj dir exists
$(OBJDIR):
	@mkdir -p $(OBJDIR)

# compile sources to .build/
$(OBJDIR)/%.o: %.cpp $(HDRS) | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# link into executable in current directory
$(TARGET): $(OBJS) | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)
	@echo "Built $(TARGET)"

# convenience: run the benchmark (default CSV output)
run: $(TARGET)
	@./benchmark

clean:
	@rm -rf $(OBJDIR) $(TARGET)
	@echo "Cleaned $(OBJDIR) and $(TARGET)"
	@rm -rf plots benchmark_results.csv
	@echo "Cleaned plots and benchmark_results.csv"