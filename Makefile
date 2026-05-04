# Makefile for HireMe AI - Interview Simulator
# Compiles C backend with SQLite3 support

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS = -lsqlite3

# Directories
SRC_DIR = backend-c
TEST_DIR = tests
BUILD_DIR = build

# Source files
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/menu.c \
          $(SRC_DIR)/interview.c \
          $(SRC_DIR)/api.c \
		  $(SRC_DIR)/gemini.c \
          $(SRC_DIR)/db.c \
          $(SRC_DIR)/json.c \
          $(SRC_DIR)/report.c \
          $(SRC_DIR)/utils.c

TEST_SOURCES = $(TEST_DIR)/test_db.c \
               $(TEST_DIR)/test_api.c \
               $(TEST_DIR)/test_score.c

# Object files
OBJECTS = $(SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

# Output executables
EXECUTABLE = hireme
TEST_EXECUTABLE = runtests

# Default target
all: $(BUILD_DIR)/$(EXECUTABLE)

# Build the main executable
$(BUILD_DIR)/$(EXECUTABLE): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $@"

# Compile C source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build and run tests
test: $(TEST_OBJECTS) $(filter-out $(SRC_DIR)/main.o,$(OBJECTS))
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$(TEST_EXECUTABLE) $^ $(LDFLAGS)
	@echo "Running tests..."
	@$(BUILD_DIR)/$(TEST_EXECUTABLE)

# Clean build artifacts
clean:
	@rm -rf $(BUILD_DIR)
	@rm -f $(OBJECTS) $(TEST_OBJECTS)
	@echo "Clean complete"

# Rebuild everything
rebuild: clean all

# Help target
help:
	@echo "HireMe AI Makefile"
	@echo "=================="
	@echo "make             - Build the application"
	@echo "make test        - Run unit tests"
	@echo "make clean       - Remove build artifacts"
	@echo "make rebuild     - Clean and rebuild"
	@echo "make help        - Show this help message"

.PHONY: all test clean rebuild help
