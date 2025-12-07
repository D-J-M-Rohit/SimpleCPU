# SimpleCPU Architecture and Getting Started Guide

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Downloading the Project](#downloading-the-project)
3. [Compiling the Project](#compiling-the-project)
4. [Running Programs](#running-programs)
5. [Quick Start Examples](#quick-start-examples)
6. [Troubleshooting](#troubleshooting)
7. [Advanced Usage](#advanced-usage)
8. [Build Targets Reference](#build-targets-reference)
9. [File Structure After Building](#file-structure-after-building)

---

## System Requirements

### Operating System

* Linux (Ubuntu 20.04 or newer, Debian, Fedora, and similar)
* macOS (10.14 or newer)
* Windows (via WSL2 or MinGW)

### Required Software

* GCC or Clang  C compiler with C11 support
* Make  GNU Make 3.81 or newer
* Standard C library (glibc or equivalent)

### Optional Tools

* Git  for version control
* GDB  for debugging
* Valgrind  for memory analysis

---

## Downloading the Project

### Option 1  From Git Repository (if available)

```bash
# Clone the repository
git clone https://github.com/D-J-M-Rohit/SimpleCPU.git
cd SimpleCPU
```

### Option 2  From Archive File

```bash
# Extract the archive
unzip SimpleCPU.zip
# OR
tar -xzf SimpleCPU.tar.gz

# Navigate to directory
cd SimpleCPU
```

### Option 3  Manual File Organization

If you have the files separately, organize them as follows.

```text
SimpleCPU/
├── Makefile
├── README.md
├── docs/
│   ├── report.pdf
├── src/
│   ├── cpu.h
│   ├── cpu.c
│   ├── assembler.h
│   ├── assembler.c
│   └── main.c
└── programs/
    ├── hello.asm
    ├── timer.asm
    ├── fibonacci.asm
    └── factorial.asm
```

---

## Compiling the Project

### Step 1  Verify Prerequisites

```bash
# Check if GCC is installed
gcc --version

# Check if Make is installed
make --version
```

Expected versions.

* GCC  version 7.0 or newer
* Make  version 3.81 or newer

If they are not installed.

```bash
# Ubuntu or Debian
sudo apt update
sudo apt install build-essential

# macOS (requires Xcode tools)
xcode-select --install

# Fedora or RHEL
sudo dnf install gcc make
```

### Step 2  Navigate to Project Directory

```bash
cd /path/to/SimpleCPU
```

### Step 3  Build the Project

```bash
# Clean any previous builds (optional)
make clean

# Build the emulator and all programs
make all
```

Example output.

```text
mkdir -p build
gcc -Wall -Wextra -std=c11 -O2 -g -c src/main.c -o build/main.o
gcc -Wall -Wextra -std=c11 -O2 -g -c src/cpu.c -o build/cpu.o
gcc -Wall -Wextra -std=c11 -O2 -g -c src/assembler.c -o build/assembler.o
gcc -Wall -Wextra -std=c11 -O2 -g -o simple-cpu build/main.o build/cpu.o build/assembler.o
Built simple-cpu successfully!
./simple-cpu assemble programs/timer.asm build/timer.bin
Assembling programs/timer.asm...
Assembled 40 bytes to build/timer.bin
Success! Output written to build/timer.bin
./simple-cpu assemble programs/hello.asm build/hello.bin
Assembling programs/hello.asm...
Assembled 113 bytes to build/hello.bin
Success! Output written to build/hello.bin
./simple-cpu assemble programs/fibonacci.asm build/fibonacci.bin
Assembling programs/fibonacci.asm...
Assembled 74 bytes to build/fibonacci.bin
Success! Output written to build/fibonacci.bin
./simple-cpu assemble programs/factorial.asm build/factorial.bin
Assembling programs/factorial.asm...
Assembled 119 bytes to build/factorial.bin
Success! Output written to build/factorial.bin
```

### Step 4  Verify Build

```bash
# Check if the executable was created
ls -lh simple-cpu

# Check if example programs were assembled
ls -lh build/*.bin
```

---

## Running Programs

The SimpleCPU emulator supports several modes.

### Mode 1  Run Pre assembled Binary (factorial)

```bash
./simple-cpu run build/factorial.bin
```

Example output.

```text
=== Program Output ===
3! = 6
=== End Output ===
```

### Mode 2  Assemble and Run in One Step (factorial)

```bash
./simple-cpu asm-run programs/factorial.asm
```

This assembles `programs/factorial.asm` to an in memory binary and immediately runs it on the emulator.

### Mode 3  Debug Mode (detailed execution)

```bash
./simple-cpu debug build/factorial.bin
```

This prints.

* Initial register state
* Program output
* Final register state
* Cycle count

### Mode 4  Assemble Only

Assemble Only

```bash
./simple-cpu assemble programs/myprogram.asm build/myprogram.bin
```

---

## Quick Start Examples

### Example 1  Factorial of 3

```bash
make
./simple-cpu run build/factorial.bin
```

Expected output.

```text
=== Program Output ===
3! = 6
=== End Output ===
```

### Example 2  Hello World (optional)

```bash
./simple-cpu run build/hello.bin
```

Expected output.

```text
=== Program Output ===
Hello, World!
=== End Output ===
```

### Example 3  Fibonacci Sequence (optional)

```bash
./simple-cpu run build/fibonacci.bin
```

Typical output for a simple Fibonacci program might look like.

```text
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

### Example 4  Write Your Own Program

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

### Problem 1  `gcc: command not found`

Install a C compiler.

```bash
# Ubuntu or Debian
sudo apt install gcc

# macOS
xcode-select --install
```

### Problem 2  `make: command not found`

Install Make.

```bash
# Ubuntu or Debian
sudo apt install make

# macOS
xcode-select --install
```

### Problem 3  Compilation Errors

Common issues.

**Missing includes.**

```text
error: unknown type name 'size_t'
```

Solution  add `#include <stddef.h>` to the affected file.

**Undefined references.**

```text
undefined reference to 'function_name'
```

Solution  ensure all source files are listed in the Makefile.

### Problem 4  Assembly Errors

```text
Error on line X: Invalid instruction 'XXX'
```

Checks.

* Instruction names are valid mnemonics  NOP, LOAD, STORE, ADD, JZ and so on
* Register names are correct  A, B, C, D, SP, PC
* Hex addresses use the `0x` prefix  for example `0xFF00`

### Problem 5  Program Does Not Run as Expected

Check the following.

1. The binary exists.  `ls build/program.bin`
2. The binary is not empty.  `ls -lh build/program.bin`
3. Run in debug mode to see registers and flags.  `./simple-cpu debug build/program.bin`

### Problem 6  Permission Denied for Executable

```bash
chmod +x simple-cpu
```

---

## Advanced Usage

### Running with Valgrind (memory check)

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

Edit the `Makefile`.

* Change compiler.  set `CC = clang` instead of `CC = gcc`
* Change optimization.  `-O0` for easy debugging, `-O3` for speed
* Change debug symbols.  add or remove the `-g` flag

---

## Build Targets Reference

| Command              | Description                                  |
| -------------------- | -------------------------------------------- |
| `make` or `make all` | Build emulator and all example programs      |
| `make clean`         | Remove all build artifacts                   |
| `make run-all`       | Run all example programs                     |
| `make test`          | Run all quick tests  hello, timer, fibonacci |

---

## File Structure After Building

```text
SimpleCPU/
├── simple-cpu           # Main executable
├── Makefile
├── README.md        
├── docs/
│   └── report.pdf  
│   
├── src/
│   ├── cpu.h           # CPU header
│   ├── cpu.c           # CPU implementation
│   ├── assembler.h     # Assembler header
│   ├── assembler.c     # Assembler implementation
│   └── main.c          # Main program
├── programs/
│   ├── hello.asm       # Hello World program
│   ├── timer.asm       # Timer or number example
│   ├── fibonacci.asm   # Fibonacci sequence example
│   └── factorial.asm   # Recursive factorial example
└── build/              # Generated directory
    ├── main.o          # Compiled object files
    ├── cpu.o
    ├── assembler.o
    ├── hello.bin       # Assembled binaries
    ├── timer.bin
    ├── fibonacci.bin
    └── factorial.bin
```

---

*Last Updated  November 2025*
*SimpleCPU Version  1.0*
