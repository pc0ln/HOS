# Compiler setup
CXX = g++
CXXFLAGS = -Wall -Wextra

# Directories
SRC_DIR = src
BUILD_DIR = build

# Source files
SRCS = \
    $(SRC_DIR)/main.cpp \
    $(SRC_DIR)/engine.cpp \
    $(SRC_DIR)/input_parser.cpp

# Object files
OBJS = \
    $(BUILD_DIR)/main.o \
    $(BUILD_DIR)/engine.o \
    $(BUILD_DIR)/input_parser.o

# Final executable in ROOT directory
TARGET = scheduler

# Default rule
all: $(TARGET)

# Link the final program
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rules to compile each .cpp into .o
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/main.cpp -o $(BUILD_DIR)/main.o

$(BUILD_DIR)/engine.o: $(SRC_DIR)/engine.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/engine.cpp -o $(BUILD_DIR)/engine.o

$(BUILD_DIR)/input_parser.o: $(SRC_DIR)/input_parser.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/input_parser.cpp -o $(BUILD_DIR)/input_parser.o

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean build files and the scheduler binary
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)

.PHONY: all clean
