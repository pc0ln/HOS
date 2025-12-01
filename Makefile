# Compiler + flags
CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2
INCLUDES := -Isrc -Iextern

SRC_DIR  := src
TEST_DIR := test

# Sources

# Main program sources
SRCS_MAIN := \
    $(SRC_DIR)/main.cpp \
    $(SRC_DIR)/input_parser.cpp \
    $(SRC_DIR)/engine.cpp

OBJS_MAIN := $(SRCS_MAIN:.cpp=.o)

# Parser test sources
SRCS_TEST := \
    $(TEST_DIR)/test_parser.cpp \
    $(SRC_DIR)/input_parser.cpp

OBJS_TEST := $(SRCS_TEST:.cpp=.o)

# Targets

# Build everything by default
all: scheduler parser_tests

scheduler: $(OBJS_MAIN)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

parser_tests: $(OBJS_TEST)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# Generic rule to build .o files from .cpp
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Run tests
test: parser_tests
	./parser_tests

clean: # Basic cleaning of files
	rm -f $(OBJS_MAIN) $(OBJS_TEST) scheduler parser_tests

.PHONY: all test clean
