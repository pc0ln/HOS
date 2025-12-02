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
SRCS_PARSER_TEST := \
    $(TEST_DIR)/test_parser.cpp \
    $(SRC_DIR)/input_parser.cpp

OBJS_PARSER_TEST := $(SRCS_PARSER_TEST:.cpp=.o)

# Engine test sources
SRCS_ENGINE_TEST := \
    $(TEST_DIR)/test_engine.cpp \
    $(SRC_DIR)/engine.cpp

OBJS_ENGINE_TEST := $(SRCS_ENGINE_TEST:.cpp=.o)

# Targets

all: scheduler parser_tests engine_tests

scheduler: $(OBJS_MAIN)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

parser_tests: $(OBJS_PARSER_TEST)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

engine_tests: $(OBJS_ENGINE_TEST)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# Generic rule
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Run all tests
test: parser_tests engine_tests
	./parser_tests
	./engine_tests

clean:
	rm -f $(OBJS_MAIN) $(OBJS_PARSER_TEST) $(OBJS_ENGINE_TEST) scheduler parser_tests engine_tests

.PHONY: all test clean
