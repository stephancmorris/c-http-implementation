# NanoServe Makefile
# High-Reliability Idempotent HTTP Server

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -Werror -std=c11 -pthread -I./include
LDFLAGS := -pthread

# Directories
SRC_DIR := src
INC_DIR := include
BIN_DIR := bin
OBJ_DIR := $(BIN_DIR)/obj

# Target executable
TARGET := $(BIN_DIR)/nanoserve

# Source files (currently only main.c, will expand as we add implementations)
SOURCES := $(wildcard $(SRC_DIR)/*.c)

# Object files
OBJECTS := $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Dependency files
DEPS := $(OBJECTS:.o=.d)

# Default target
.PHONY: all
all: $(TARGET)

# Link executable
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	@echo "Linking $@..."
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo "Build complete: $@"

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Create directories if they don't exist
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BIN_DIR)
	@echo "Clean complete"

# Debug build (with debug symbols and no optimization)
.PHONY: debug
debug: CFLAGS += -g -O0 -DDEBUG
debug: clean $(TARGET)
	@echo "Debug build complete"

# Release build (with optimization)
.PHONY: release
release: CFLAGS += -O2 -DNDEBUG
release: clean $(TARGET)
	@echo "Release build complete"

# Run the server
.PHONY: run
run: $(TARGET)
	@echo "Starting NanoServe..."
	@$(TARGET)

# Test target (placeholder for future tests)
.PHONY: test
test:
	@echo "No tests implemented yet"
	@echo "Run 'make run' to start the server and test manually"

# Help target
.PHONY: help
help:
	@echo "NanoServe Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build the server (default)"
	@echo "  clean    - Remove all build artifacts"
	@echo "  debug    - Build with debug symbols (-g -O0)"
	@echo "  release  - Build with optimizations (-O2)"
	@echo "  run      - Build and run the server"
	@echo "  test     - Run tests (not implemented yet)"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Compiler flags: $(CFLAGS)"
	@echo "Linker flags:   $(LDFLAGS)"

# Include dependency files (for automatic recompilation when headers change)
-include $(DEPS)

# Prevent make from deleting intermediate files
.PRECIOUS: $(OBJ_DIR)/%.o
