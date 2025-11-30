# SimpleCPU Architecture Documentation

## Table of Contents

### Architecture Documentation
1. [CPU Architecture Schematic](#cpu-architecture-schematic)
2. [Instruction Set Architecture (ISA)](#instruction-set-architecture-isa)
   - [Instruction Format](#instruction-format)
   - [Registers](#registers)
   - [Flag Register](#flag-register-flags)
   - [Instruction Set](#instruction-set)
   - [Addressing Modes](#addressing-modes)
   - [Register Encoding](#register-encoding)
   - [Memory Map](#memory-map)
   - [I/O Ports](#io-ports)
   - [Execution Cycle](#execution-cycle)
   - [Flag Behavior](#flag-behavior)
   - [Stack](#stack)

### Getting Started Guide
3. [System Requirements](#system-requirements)
4. [Downloading the Project](#downloading-the-project)
5. [Compiling the Project](#compiling-the-project)
6. [Running Programs](#running-programs)
7. [Quick Start Examples](#quick-start-examples)
8. [Troubleshooting](#troubleshooting)
9. [Advanced Usage](#advanced-usage)
10. [Build Targets Reference](#build-targets-reference)
11. [File Structure After Building](#file-structure-after-building)

---

## CPU Architecture Schematic

```
┌─────────────────────────────────────────────────────────────────┐
│                         SimpleCPU                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────┐        ┌──────────────┐                       │
│  │   Registers  │        │  Control     │                       │
│  │              │        │  Unit        │                       │
│  │  PC (16-bit) │◄───────┤              │                       │
│  │  SP (16-bit) │        │  - Fetch     │                       │
│  │  A  (16-bit) │        │  - Decode    │                       │
│  │  B  (16-bit) │        │  - Execute   │                       │
│  │  C  (16-bit) │        └──────┬───────┘                       │
│  │  D  (16-bit) │               │                               │
│  │  FLAGS (8)   │               │                               │
│  └──────┬───────┘               │                               │
│         │                       │                               │
│         │      ┌────────────────▼─────────────┐                 │
│         │      │        ALU                   │                 │
│         └─────►│  - ADD, SUB, MUL, DIV       │                 │
│                │  - AND, OR, XOR, NOT         │                 │
│                │  - CMP, SHL, SHR             │                 │
│                └────────────┬─────────────────┘                 │
│                             │                                   │
│                  ┌──────────▼──────────┐                        │
│                  │      Data Bus       │                        │
│                  │      (16-bit)       │                        │
│                  └──────────┬──────────┘                        │
│                             │                                   │
│         ┌───────────────────┴────────────────────┐              │
│         │                                        │              │
│  ┌──────▼──────┐                        ┌───────▼──────┐       │
│  │   Memory    │                        │  Memory-     │       │
│  │             │                        │  Mapped I/O  │       │
│  │  64KB RAM   │                        │              │       │
│  │             │                        │  0xFF00-     │       │
│  │  0x0000-    │                        │  0xFFFF      │       │
│  │  0xFEFF     │                        │              │       │
│  └─────────────┘                        └──────────────┘       │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘

Memory Map:
  0x0000 - 0x00FF : Interrupt Vector Table
  0x0100 - 0x7FFF : Program Code & Data
  0x8000 - 0xFEFF : General Purpose RAM / Stack
  0xFF00 - 0xFF0F : I/O Registers
  0xFF10 - 0xFFFF : Reserved
```

---

## Instruction Set Architecture (ISA)

### Instruction Format

SimpleCPU uses a variable-length instruction format:

**Format 1: No Operands** (1 byte)
```
┌────────┐
│ OPCODE │
└────────┘
  8 bits
```

**Format 2: Register Operand** (2 bytes)
```
┌────────┬────────┐
│ OPCODE │  REG   │
└────────┴────────┘
  8 bits   8 bits
```

**Format 3: Register-Register** (2 bytes)
```
┌────────┬────┬────┐
│ OPCODE │ R1 │ R2 │
└────────┴────┴────┘
  8 bits  4bit 4bit
```

**Format 4: Register-Immediate** (3 or 4 bytes)
```
┌────────┬────────┬─────────────┐
│ OPCODE │  REG   │   IMM16     │
└────────┴────────┴─────────────┘
  8 bits   8 bits    16 bits
```

**Format 5: Memory Address** (3 bytes)
```
┌────────┬─────────────┐
│ OPCODE │   ADDR16    │
└────────┴─────────────┘
  8 bits    16 bits
```

---

### Registers

- **PC** - Program Counter (16-bit)
- **SP** - Stack Pointer (16-bit)
- **A, B, C, D** - General Purpose Registers (16-bit each)
- **FLAGS** - Status Flags (8-bit)

### Flag Register (FLAGS)

```
Bit 7: Z  - Zero Flag
Bit 6: C  - Carry Flag
Bit 5: N  - Negative Flag
Bit 4: O  - Overflow Flag
Bit 3-0:  - Reserved
```

---

### Instruction Set

| Mnemonic | Opcode | Format | Description |
|----------|--------|--------|-------------|
| **Data Movement** ||||
| NOP | 0x00 | 1 | No operation |
| LOAD r, imm16 | 0x01 | 4 | Load immediate to register |
| LOAD r, [addr] | 0x02 | 3 | Load from memory to register |
| STORE [addr], r | 0x03 | 3 | Store register to memory |
| MOV r1, r2 | 0x04 | 2 | Copy r2 to r1 |
| PUSH r | 0x05 | 2 | Push register to stack |
| POP r | 0x06 | 2 | Pop from stack to register |
| **Arithmetic** ||||
| ADD r1, r2 | 0x10 | 2 | r1 = r1 + r2 |
| ADDI r, imm16 | 0x11 | 4 | r = r + imm16 |
| SUB r1, r2 | 0x12 | 2 | r1 = r1 - r2 |
| SUBI r, imm16 | 0x13 | 4 | r = r - imm16 |
| MUL r1, r2 | 0x14 | 2 | r1 = r1 * r2 |
| DIV r1, r2 | 0x15 | 2 | r1 = r1 / r2, r2 = remainder |
| INC r | 0x16 | 2 | Increment register |
| DEC r | 0x17 | 2 | Decrement register |
| **Logic** ||||
| AND r1, r2 | 0x20 | 2 | r1 = r1 & r2 |
| OR r1, r2 | 0x21 | 2 | r1 = r1 \| r2 |
| XOR r1, r2 | 0x22 | 2 | r1 = r1 ^ r2 |
| NOT r | 0x23 | 2 | r = ~r |
| SHL r, imm | 0x24 | 3 | Shift left |
| SHR r, imm | 0x25 | 3 | Shift right |
| **Comparison** ||||
| CMP r1, r2 | 0x30 | 2 | Compare r1 with r2 (sets flags) |
| CMPI r, imm | 0x31 | 4 | Compare r with immediate |
| **Control Flow** ||||
| JMP addr | 0x40 | 3 | Unconditional jump |
| JZ addr | 0x41 | 3 | Jump if zero flag set |
| JNZ addr | 0x42 | 3 | Jump if zero flag clear |
| JC addr | 0x43 | 3 | Jump if carry flag set |
| JNC addr | 0x44 | 3 | Jump if carry flag clear |
| CALL addr | 0x45 | 3 | Call subroutine |
| RET | 0x46 | 1 | Return from subroutine |
| **I/O** ||||
| IN r, port | 0x50 | 3 | Read from I/O port |
| OUT port, r | 0x51 | 3 | Write to I/O port |
| **System** ||||
| HLT | 0xFF | 1 | Halt execution |

---

### Addressing Modes

1. **Immediate**: Value directly in instruction
   - `LOAD A, 0x1234`

2. **Register**: Value in register
   - `MOV A, B`

3. **Direct**: Memory address in instruction
   - `LOAD A, [0x8000]`

4. **Register Indirect**: Address stored in register (future extension)
   - `LOAD A, [B]`

---

### Register Encoding

```
0x00 - A register
0x01 - B register
0x02 - C register
0x03 - D register
0x04 - SP (Stack Pointer)
0x05 - PC (Program Counter) - special access
```

---

### Memory Map

| Address Range | Purpose |
|--------------|----------------------------------|
| 0x0000 - 0x00FF | Interrupt Vector Table (reserved) |
| 0x0100 - 0x7FFF | Program Code and Read-Only Data |
| 0x8000 - 0xFEFF | General RAM / Stack Space |
| 0xFF00 - 0xFF00 | STDOUT Port (character output) |
| 0xFF01 - 0xFF01 | STDIN Port (character input) |
| 0xFF02 - 0xFF02 | TIMER_CTRL (timer control) |
| 0xFF03 - 0xFF03 | TIMER_VALUE (timer value) |
| 0xFF04 - 0xFF0F | Reserved I/O |
| 0xFF10 - 0xFFFF | Reserved |

---

### I/O Ports

- **0xFF00**: STDOUT - Write character to output
- **0xFF01**: STDIN - Read character from input
- **0xFF02**: TIMER_CTRL - Timer control register
- **0xFF03**: TIMER_VALUE - Timer current value

---

### Execution Cycle

Each instruction follows a standard Fetch-Decode-Execute cycle:

1. **Fetch**: 
   - Read instruction at PC
   - Increment PC
   
2. **Decode**: 
   - Parse opcode
   - Identify operands
   - Determine addressing mode
   
3. **Execute**: 
   - Perform operation
   - Update flags
   - Write results

---

### Flag Behavior

- **Zero (Z)**: Set if result is zero
- **Carry (C)**: Set on unsigned overflow/borrow
- **Negative (N)**: Set if result MSB is 1
- **Overflow (O)**: Set on signed overflow

---

### Stack

- Grows downward from high memory
- Initial SP typically set to 0xFEFF
- PUSH decrements SP then writes
- POP reads then increments SP

---

## System Requirements

### Operating System
- **Linux** (Ubuntu 20.04+, Debian, Fedora, etc.)
- **macOS** (10.14+)
- **Windows** (via WSL2 or MinGW)

### Required Software
- **GCC** or **Clang** (C compiler with C11 support)
- **Make** (GNU Make 3.81 or higher)
- **Standard C library** (glibc or equivalent)

### Optional Tools
- **Git** (for version control)
- **GDB** (for debugging)
- **Valgrind** (for memory leak detection)

---

## Downloading the Project

### Option 1: From Git Repository (if available)
```bash
# Clone the repository
git clone https://github.com/yourusername/SimpleCPU.git
cd SimpleCPU
```

### Option 2: From Archive File
```bash
# Extract the archive
unzip SimpleCPU.zip
# OR
tar -xzf SimpleCPU.tar.gz

# Navigate to directory
cd SimpleCPU
```

### Option 3: Manual File Organization
If you have the files separately, organize them as follows:
```
SimpleCPU/
├── Makefile
├── docs/
│   ├── ARCHITECTURE.md
│   └── README.md
├── src/
│   ├── cpu.h
│   ├── cpu.c
│   ├── assembler.h
│   ├── assembler.c
│   └── main.c
└── programs/
    ├── hello.asm
    ├── timer.asm
    └── fibonacci.asm
```

---

## Compiling the Project

### Step 1: Verify Prerequisites
```bash
# Check if GCC is installed
gcc --version

# Check if Make is installed
make --version
```

**Expected Output:**
- GCC: Version 7.0 or higher
- Make: Version 3.81 or higher

If not installed:
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential

# macOS (requires Homebrew)
xcode-select --install

# Fedora/RHEL
sudo dnf install gcc make
```

### Step 2: Navigate to Project Directory
```bash
cd /path/to/SimpleCPU
```

### Step 3: Build the Project
```bash
# Clean any previous builds (optional)
make clean

# Build the emulator and all programs
make all
```

**Expected Output:**
```
mkdir -p build
gcc -Wall -Wextra -std=c11 -O2 -g -c -o build/main.o src/main.c
gcc -Wall -Wextra -std=c11 -O2 -g -c -o build/cpu.o src/cpu.c
gcc -Wall -Wextra -std=c11 -O2 -g -c -o build/assembler.o src/assembler.c
gcc -Wall -Wextra -std=c11 -O2 -g -o simple-cpu build/main.o build/cpu.o build/assembler.o
Built simple-cpu successfully!
./simple-cpu assemble programs/timer.asm build/timer.bin
./simple-cpu assemble programs/hello.asm build/hello.bin
./simple-cpu assemble programs/fibonacci.asm build/fibonacci.bin
```

### Step 4: Verify Build
```bash
# Check if the executable was created
ls -lh simple-cpu

# Check if example programs were assembled
ls -lh build/*.bin
```

---

## Running Programs

The SimpleCPU emulator has several modes of operation:

### Mode 1: Run Pre-assembled Binary
```bash
./simple-cpu run build/hello.bin
```

**Output:**
```
=== Program Output ===
Hello, World!
=== End Output ===
```

### Mode 2: Assemble and Run in One Step
```bash
./simple-cpu asm-run programs/hello.asm
```

### Mode 3: Debug Mode (Detailed Execution)
```bash
./simple-cpu debug build/hello.bin
```

**Output shows:**
- Initial register state
- Program execution
- Final register state
- Cycle count

### Mode 4: Assemble Only
```bash
./simple-cpu assemble programs/myprogram.asm build/myprogram.bin
```

---

## Quick Start Examples

### Example 1: Hello World
```bash
# Build and run
make
./simple-cpu run build/hello.bin
```

**Expected Output:**
```
=== Program Output ===
Hello, World!
=== End Output ===
```

### Example 2: Timer Program
```bash
./simple-cpu run build/timer.bin
```

**Expected Output:**
```
=== Program Output ===
0
1
2
3
4
Done!
=== End Output ===
```

### Example 3: Fibonacci Sequence
```bash
./simple-cpu run build/fibonacci.bin
```

**Expected Output:**
```
=== Program Output ===
0
1
1
2
3
5
8
13
21
34
Done!
=== End Output ===
```

### Example 4: Write Your Own Program
```bash
# 1. Create a new assembly file
cat > programs/myprogram.asm << 'EOF'
; My First Program
LOAD A, 65        ; ASCII 'A'
OUT 0xFF00, A     ; Print to STDOUT
HLT               ; Stop
EOF

# 2. Assemble and run
./simple-cpu asm-run programs/myprogram.asm
```

---

## Troubleshooting

### Problem 1: "gcc: command not found"
**Solution:** Install GCC compiler
```bash
# Ubuntu/Debian
sudo apt install gcc

# macOS
xcode-select --install
```

### Problem 2: "make: command not found"
**Solution:** Install Make utility
```bash
# Ubuntu/Debian
sudo apt install make

# macOS
xcode-select --install
```

### Problem 3: Compilation Errors
**Common Issues:**

**Missing includes:**
```
error: unknown type name 'size_t'
```
**Solution:** Add `#include <stddef.h>` to affected files

**Undefined references:**
```
undefined reference to 'function_name'
```
**Solution:** Ensure all source files are listed in Makefile

### Problem 4: Assembly Errors
```
Error on line X: Invalid instruction 'XXX'
```

**Solution:** Check assembly syntax in program
- Ensure instruction names are uppercase
- Check for typos in register names (A, B, C, D, SP, PC)
- Verify hex addresses use 0x prefix (e.g., 0xFF00)

### Problem 5: Program Doesn't Run
**Check:**
1. File exists: `ls build/program.bin`
2. File is not empty: `ls -lh build/program.bin`
3. Run with debug mode: `./simple-cpu debug build/program.bin`

### Problem 6: Permission Denied
```bash
# Make executable runnable
chmod +x simple-cpu
```

---

## Advanced Usage

### Running with Valgrind (Memory Check)
```bash
valgrind --leak-check=full ./simple-cpu run build/hello.bin
```

### Using GDB for Debugging
```bash
gdb ./simple-cpu
(gdb) run asm-run programs/hello.asm
(gdb) break cpu_step
(gdb) continue
```

### Customizing the Build
Edit `Makefile` to change:
- **Compiler:** Change `CC = gcc` to `CC = clang`
- **Optimization:** Change `-O2` to `-O0` (debug) or `-O3` (max speed)
- **Debug symbols:** Add or remove `-g` flag

---

## Build Targets Reference

| Command | Description |
|---------|-------------|
| `make` or `make all` | Build emulator and all example programs |
| `make clean` | Remove all build artifacts |
| `make run-all` | Run all example programs |
| `make test` | Run hello world (quick test) |

---

## File Structure After Building

```
SimpleCPU/
├── simple-cpu           # Main executable
├── Makefile
├── docs/
│   ├── ARCHITECTURE.md  # CPU architecture documentation
│   └── README.md        # Project overview
├── src/
│   ├── cpu.h           # CPU header
│   ├── cpu.c           # CPU implementation
│   ├── assembler.h     # Assembler header
│   ├── assembler.c     # Assembler implementation
│   └── main.c          # Main program
├── programs/
│   ├── hello.asm       # Hello World program
│   ├── timer.asm       # Timer example
│   └── fibonacci.asm   # Fibonacci calculator
└── build/              # Generated directory
    ├── main.o          # Compiled object files
    ├── cpu.o
    ├── assembler.o
    ├── hello.bin       # Assembled binaries
    ├── timer.bin
    └── fibonacci.bin
```

---

*Last Updated: November 2025*  
*SimpleCPU Version: 1.0*