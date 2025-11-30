#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

//=========================================================
// Global CPU configuration
//=========================================================

// Total memory size: 64KB (0x0000–0xFFFF)
#define MEMORY_SIZE 0x10000

//=========================================================
// Register indices
//=========================================================
//
// We store all registers in a small array regs[6].
// These indices define where each logical register lives:
//
//   regs[REG_A]  → A  (general purpose)
//   regs[REG_B]  → B  (general purpose)
//   regs[REG_C]  → C  (general purpose)
//   regs[REG_D]  → D  (general purpose)
//   regs[REG_SP] → SP (stack pointer)
//   regs[REG_PC] → PC (program counter)
//
#define REG_A  0
#define REG_B  1
#define REG_C  2
#define REG_D  3
#define REG_SP 4
#define REG_PC 5

//=========================================================
// Status flag bits (FLAGS register)
//=========================================================
//
// FLAGS is an 8-bit register. We use the high nibble:
//
//  Bit 7 (0x80): Z - Zero flag      (result == 0)
//  Bit 6 (0x40): C - Carry flag     (unsigned overflow / borrow)
//  Bit 5 (0x20): N - Negative flag  (MSB of result = 1)
//  Bit 4 (0x10): O - Overflow flag  (signed overflow)
//  Bits 3–0: reserved for future use.
//
#define FLAG_ZERO     0x80  // Bit 7
#define FLAG_CARRY    0x40  // Bit 6
#define FLAG_NEGATIVE 0x20  // Bit 5
#define FLAG_OVERFLOW 0x10  // Bit 4

//=========================================================
// Memory-mapped I/O ports
//=========================================================
//
// These addresses live in the top of the address space and are
// interpreted specially by cpu_read_byte / cpu_write_byte.
//
//  0xFF00: PORT_STDOUT      - write a character to host stdout
//  0xFF01: PORT_STDIN       - read a character from host stdin
//  0xFF02: PORT_TIMER_CTRL  - enable/disable timer (non-zero = on)
//  0xFF03: PORT_TIMER_VALUE - current timer value (incremented each step)
//
#define PORT_STDOUT      0xFF00
#define PORT_STDIN       0xFF01
#define PORT_TIMER_CTRL  0xFF02
#define PORT_TIMER_VALUE 0xFF03

//=========================================================
// Opcode definitions (ISA)
//=========================================================
//
// Each opcode is a single byte. The encoding and operand formats
// are documented in ARCHITECTURE.md. Here we just name them so
// the assembler and emulator can agree.
//

// Data movement
#define OP_NOP        0x00
#define OP_LOAD_IMM   0x01   // LOAD r, imm16
#define OP_LOAD_MEM   0x02   // LOAD r, [addr]
#define OP_STORE      0x03   // STORE [addr], r
#define OP_MOV        0x04   // MOV r1, r2
#define OP_PUSH       0x05   // PUSH r
#define OP_POP        0x06   // POP r

// Arithmetic
#define OP_ADD        0x10   // ADD r1, r2
#define OP_ADDI       0x11   // ADDI r, imm16
#define OP_SUB        0x12   // SUB r1, r2
#define OP_SUBI       0x13   // SUBI r, imm16
#define OP_MUL        0x14   // MUL r1, r2
#define OP_DIV        0x15   // DIV r1, r2
#define OP_INC        0x16   // INC r
#define OP_DEC        0x17   // DEC r

// Logic
#define OP_AND        0x20   // AND r1, r2
#define OP_OR         0x21   // OR r1, r2
#define OP_XOR        0x22   // XOR r1, r2
#define OP_NOT        0x23   // NOT r
#define OP_SHL        0x24   // SHL r, imm8
#define OP_SHR        0x25   // SHR r, imm8

// Comparison
#define OP_CMP        0x30   // CMP r1, r2
#define OP_CMPI       0x31   // CMPI r, imm16

// Control flow
#define OP_JMP        0x40   // JMP addr
#define OP_JZ         0x41   // JZ addr
#define OP_JNZ        0x42   // JNZ addr
#define OP_JC         0x43   // JC addr
#define OP_JNC        0x44   // JNC addr
#define OP_CALL       0x45   // CALL addr
#define OP_RET        0x46   // RET

// I/O
#define OP_IN         0x50   // IN r, port
#define OP_OUT        0x51   // OUT port, r

// System
#define OP_HLT        0xFF   // HLT

//=========================================================
// CPU State Structure
//=========================================================
//
// This struct represents the entire visible state of the CPU:
// registers, flags, memory, execution status, and a very simple
// timer. The emulator operates purely by mutating this struct.
//
typedef struct {
    // ---------------- Registers ----------------
    // A, B, C, D: general-purpose 16-bit registers
    // SP:  stack pointer (16-bit)
    // PC:  program counter (16-bit)
    uint16_t regs[6];    // [A, B, C, D, SP, PC] in that order

    // Status flags (Z, C, N, O)
    uint8_t  flags;

    // ---------------- Memory ----------------
    // Flat 64KB byte-addressable memory.
    uint8_t memory[MEMORY_SIZE];

    // ---------------- Execution state ----------------
    bool running;        // true while cpu_run is actively stepping
    bool halted;         // set when HLT opcode is executed

    // ---------------- Statistics ----------------
    uint64_t cycles;     // number of instructions executed

    // ---------------- Timer ----------------
    // Simple software timer that increments each instruction
    // while timer_enabled is true. Exposed via memory-mapped I/O.
    uint16_t timer_value;
    bool     timer_enabled;

} CPU;

//=========================================================
// Public API: Core CPU functions
//=========================================================

// Initialize a CPU struct to a known reset state.
// Sets registers, flags, memory, PC, SP, and timer.
void cpu_init(CPU *cpu);

// Reset CPU to initial state (wrapper around cpu_init).
void cpu_reset(CPU *cpu);

// Load a program into memory starting at 'start_addr' and
// set PC to that address. Returns 0 on success, -1 on error.
int  cpu_load_program(CPU *cpu, const uint8_t *program,
                      size_t size, uint16_t start_addr);

// Run the CPU until a HLT instruction or an error occurs.
void cpu_run(CPU *cpu);

// Execute a single fetch–decode–execute step.
// Return >0 on success, 0 if already halted, <0 on error.
int  cpu_step(CPU *cpu);

//=========================================================
// Memory operations
//=========================================================

// Read a single byte from memory or a memory-mapped I/O port.
uint8_t  cpu_read_byte(CPU *cpu, uint16_t addr);

// Read a 16-bit little-endian word from memory.
uint16_t cpu_read_word(CPU *cpu, uint16_t addr);

// Write a single byte to memory or a memory-mapped I/O port.
void     cpu_write_byte(CPU *cpu, uint16_t addr, uint8_t value);

// Write a 16-bit little-endian word to memory.
void     cpu_write_word(CPU *cpu, uint16_t addr, uint16_t value);

//=========================================================
// Register operations
//=========================================================

// Get the value of a register (by REG_* index).
uint16_t cpu_get_reg(CPU *cpu, uint8_t reg);

// Set the value of a register (by REG_* index).
void     cpu_set_reg(CPU *cpu, uint8_t reg, uint16_t value);

//=========================================================
// Flag operations
//=========================================================
//
// cpu_set_flags_arithmetic:
//   Helper used by ALU operations to update Z, N, C, O based
//   on a result + explicit carry/overflow.
//
// cpu_set_flag / cpu_get_flag:
//   Manipulate individual bits in the FLAGS register.
//
void cpu_set_flags_arithmetic(CPU *cpu, uint16_t result,
                              bool carry, bool overflow);

void cpu_set_flag(CPU *cpu, uint8_t flag, bool value);
bool cpu_get_flag(CPU *cpu, uint8_t flag);

//=========================================================
// Stack operations
//=========================================================
//
// Stack grows downward in memory using SP. Each push/pop
// operates on a 16-bit word.
//
void     cpu_push(CPU *cpu, uint16_t value);
uint16_t cpu_pop(CPU *cpu);

//=========================================================
// Debug / inspection helpers
//=========================================================

// Print register file and flags.
void cpu_dump_registers(CPU *cpu);

// Print a hex dump of memory in [start, end].
void cpu_dump_memory(CPU *cpu, uint16_t start, uint16_t end);

// Print the top 'depth' entries of the stack.
void cpu_dump_stack(CPU *cpu, int depth);

#endif // CPU_H
