# SimpleCPU Build System
# ======================
# - Builds the emulator binary (TARGET)
# - Assembles example programs (hello / timer / fibonacci)
# - Provides convenience targets to run/debug/test them

# C compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g

# Name of the emulator executable
TARGET = simple-cpu

# Directory layout
SRC_DIR = src
PROGRAMS_DIR = programs
BUILD_DIR = build

# C sources and headers for the emulator + assembler
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/cpu.c $(SRC_DIR)/assembler.c
HEADERS = $(SRC_DIR)/cpu.h $(SRC_DIR)/assembler.h
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/cpu.o $(BUILD_DIR)/assembler.o

# Example assembly programs to build
ASM_PROGRAMS = timer hello fibonacci
ASM_SOURCES = $(addprefix $(PROGRAMS_DIR)/, $(addsuffix .asm, $(ASM_PROGRAMS)))
BIN_PROGRAMS = $(addprefix $(BUILD_DIR)/, $(addsuffix .bin, $(ASM_PROGRAMS)))

# Phony (non-file) targets
.PHONY: all clean programs run-all test help \
        run-hello run-timer run-fibonacci \
        debug-hello debug-timer debug-fibonacci \
        test-hello test-timer test-fibonacci

# Default target:
# - Creates build directory
# - Builds emulator
# - Assembles all example programs
all: $(BUILD_DIR) $(TARGET) programs

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link step: produce the emulator executable from object files
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo "Built $(TARGET) successfully!"

# Compile each .c file to a .o file into build/
# Depends on headers so changes to cpu.h/assembler.h also trigger rebuilds
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble all example .asm programs into .bin using the emulator's assembler
programs: $(TARGET) $(BIN_PROGRAMS)

# Pattern rule: assemble programs/foo.asm → build/foo.bin
$(BUILD_DIR)/%.bin: $(PROGRAMS_DIR)/%.asm $(TARGET)
	./$(TARGET) assemble $< $@

# Remove generated files (object files, binaries, emulator)
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Cleaned build artifacts"

# -----------------------------
# Convenience run targets
# -----------------------------

# Run Hello World example
run-hello: $(TARGET) $(BUILD_DIR)/hello.bin
	@echo "Running Hello World program:"
	@echo "=============================="
	./$(TARGET) run $(BUILD_DIR)/hello.bin

# Run Timer example
run-timer: $(TARGET) $(BUILD_DIR)/timer.bin
	@echo "Running Timer program:"
	@echo "======================"
	./$(TARGET) run $(BUILD_DIR)/timer.bin

# Run Fibonacci example
run-fibonacci: $(TARGET) $(BUILD_DIR)/fibonacci.bin
	@echo "Running Fibonacci program:"
	@echo "==========================="
	./$(TARGET) run $(BUILD_DIR)/fibonacci.bin

# -----------------------------
# Debug (run with register dump)
# -----------------------------

debug-hello: $(TARGET) $(BUILD_DIR)/hello.bin
	./$(TARGET) debug $(BUILD_DIR)/hello.bin

debug-timer: $(TARGET) $(BUILD_DIR)/timer.bin
	./$(TARGET) debug $(BUILD_DIR)/timer.bin

debug-fibonacci: $(TARGET) $(BUILD_DIR)/fibonacci.bin
	./$(TARGET) debug $(BUILD_DIR)/fibonacci.bin

# Run all examples in one go (normal mode)
run-all: run-hello run-timer run-fibonacci

# -----------------------------
# Quick tests: assemble + run in one step
# -----------------------------

test-hello: $(TARGET)
	./$(TARGET) asm-run $(PROGRAMS_DIR)/hello.asm

test-timer: $(TARGET)
	./$(TARGET) asm-run $(PROGRAMS_DIR)/timer.asm

test-fibonacci: $(TARGET)
	./$(TARGET) asm-run $(PROGRAMS_DIR)/fibonacci.asm

# Run all quick tests
test: test-hello test-timer test-fibonacci

# Help target: prints a summary of useful targets
help:
	@echo "SimpleCPU Build System"
	@echo "======================"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build everything (default)"
	@echo "  clean            - Remove build artifacts"
	@echo "  programs         - Assemble all example programs"
	@echo ""
	@echo "Run Programs:"
	@echo "  run-hello        - Run Hello World"
	@echo "  run-timer        - Run Timer example"
	@echo "  run-fibonacci    - Run Fibonacci generator"
	@echo "  run-all          - Run all programs"
	@echo ""
	@echo "Debug Programs:"
	@echo "  debug-hello      - Run Hello World with debug output"
	@echo "  debug-timer      - Run Timer with debug output"
	@echo "  debug-fibonacci  - Run Fibonacci with debug output"
	@echo ""
	@echo "Quick Test (assemble + run):"
	@echo "  test-hello       - Quick test Hello World"
	@echo "  test-timer       - Quick test Timer"
	@echo "  test-fibonacci   - Quick test Fibonacci"
	@echo "  test             - Run all quick tests"
