CXX ?= g++
CC ?= cc
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -Iinclude -Iruntime
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Iruntime

BUILD := build
CPP_SOURCES := src/lexer.cpp src/parser.cpp src/interpreter.cpp src/type_checker.cpp src/bytecode.cpp src/builtins.cpp

.PHONY: all test clean

all: $(BUILD)/tengine

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/t_runtime.o: runtime/t_runtime.c runtime/t_runtime.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/tengine: $(CPP_SOURCES) src/main.cpp $(BUILD)/t_runtime.o | $(BUILD)
	$(CXX) $(CXXFLAGS) $(CPP_SOURCES) src/main.cpp $(BUILD)/t_runtime.o -o $@

$(BUILD)/tengine_tests: $(CPP_SOURCES) tests/test_frontend.cpp $(BUILD)/t_runtime.o | $(BUILD)
	$(CXX) $(CXXFLAGS) $(CPP_SOURCES) tests/test_frontend.cpp $(BUILD)/t_runtime.o -o $@

test: $(BUILD)/tengine_tests
	./$(BUILD)/tengine_tests

clean:
	rm -rf $(BUILD)