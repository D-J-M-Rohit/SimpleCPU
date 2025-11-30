#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//=========================================================
// Core CPU lifecycle
//=========================================================

// Initialize CPU registers, memory, and control flags
void cpu_init(CPU *cpu) {
    // Clear entire CPU state (registers, flags, memory, etc.)
    memset(cpu, 0, sizeof(CPU));

    // Set initial stack pointer:
    // Stack grows downwards from near the top of general RAM.
    cpu->regs[REG_SP] = 0xFEFF;  // Stack starts at top of general RAM

    // Set initial program counter:
    // We treat 0x0000–0x00FF as a vector/metadata area and
    // load programs starting at 0x0100.
    cpu->regs[REG_PC] = 0x0100;  // Program starts after vector table

    cpu->running = false;
    cpu->halted  = false;
    cpu->cycles  = 0;            // instruction cycle counter
}

// Reset CPU to initial state (same as fresh init)
void cpu_reset(CPU *cpu) {
    cpu_init(cpu);
}

//=========================================================
// Program loading
//=========================================================

// Copy a program into memory and set PC to its start address
int cpu_load_program(CPU *cpu, const uint8_t *program, size_t size, uint16_t start_addr) {
    // Bounds check: program must fit into memory
    if (start_addr + size > MEMORY_SIZE) {
        fprintf(stderr, "Program too large for memory\n");
        return -1;
    }
    
    // Copy program bytes into the CPU's memory
    memcpy(&cpu->memory[start_addr], program, size);

    // Set PC to start of program
    cpu->regs[REG_PC] = start_addr;
    return 0;
}

//=========================================================
// Memory access (with memory-mapped I/O)
//=========================================================

// Read a single byte from memory or I/O port
uint8_t cpu_read_byte(CPU *cpu, uint16_t addr) {
    // Handle memory-mapped I/O
    if (addr == PORT_STDIN) {
        // STDIN: read character from host stdin
        int ch = getchar();
        return (ch == EOF) ? 0 : (uint8_t)ch;
    } else if (addr == PORT_TIMER_VALUE) {
        // Timer value register (low byte)
        return (uint8_t)(cpu->timer_value & 0xFF);
    } else if (addr == PORT_TIMER_CTRL) {
        // Timer control: 1 = enabled, 0 = disabled
        return cpu->timer_enabled ? 1 : 0;
    }
    
    // Default: normal RAM access
    return cpu->memory[addr];
}

// Read a 16-bit word from memory (little-endian)
uint16_t cpu_read_word(CPU *cpu, uint16_t addr) {
    uint8_t low  = cpu_read_byte(cpu, addr);
    uint8_t high = cpu_read_byte(cpu, addr + 1);
    return (high << 8) | low;
}

// Write a single byte to memory or I/O port
void cpu_write_byte(CPU *cpu, uint16_t addr, uint8_t value) {
    // Handle memory-mapped I/O
    if (addr == PORT_STDOUT) {
        // STDOUT: write character to host stdout
        putchar(value);
        fflush(stdout);
        return;
    } else if (addr == PORT_TIMER_CTRL) {
        // Timer control register: non-zero = enable, zero = disable
        cpu->timer_enabled = (value != 0);
        if (cpu->timer_enabled) {
            cpu->timer_value = 0;   // reset timer when enabled
        }
        return;
    } else if (addr == PORT_TIMER_VALUE) {
        // Directly set timer value
        cpu->timer_value = value;
        return;
    }
    
    // Default: normal RAM write
    cpu->memory[addr] = value;
}

// Write a 16-bit word to memory (little-endian)
void cpu_write_word(CPU *cpu, uint16_t addr, uint16_t value) {
    cpu_write_byte(cpu, addr,     value & 0xFF);
    cpu_write_byte(cpu, addr + 1, (value >> 8) & 0xFF);
}

//=========================================================
// Register and flag helpers
//=========================================================

// Get a register value by index (A, B, C, D, SP, PC)
uint16_t cpu_get_reg(CPU *cpu, uint8_t reg) {
    if (reg < 6) {
        return cpu->regs[reg];
    }
    return 0; // invalid register index (defensive)
}

// Set a register value by index
void cpu_set_reg(CPU *cpu, uint8_t reg, uint16_t value) {
    if (reg < 6) {
        cpu->regs[reg] = value;
    }
}

// Set arithmetic flags (Z, N, C, O) based on a result
//
// result: 16-bit result of an operation (or wider, truncated)
// carry:  carry flag from the operation (unsigned overflow/borrow)
// overflow: signed overflow flag
void cpu_set_flags_arithmetic(CPU *cpu, uint16_t result, bool carry, bool overflow) {
    // Zero flag: set if result is 0
    cpu_set_flag(cpu, FLAG_ZERO, (result & 0xFFFF) == 0);
    
    // Negative flag: set if MSB (bit 15) is 1
    cpu_set_flag(cpu, FLAG_NEGATIVE, (result & 0x8000) != 0);
    
    // Carry flag: passed in explicitly
    cpu_set_flag(cpu, FLAG_CARRY, carry);
    
    // Overflow flag: passed in explicitly
    cpu_set_flag(cpu, FLAG_OVERFLOW, overflow);
}

// Set or clear a specific status flag bit
void cpu_set_flag(CPU *cpu, uint8_t flag, bool value) {
    if (value) {
        cpu->flags |= flag;
    } else {
        cpu->flags &= ~flag;
    }
}

// Read a specific status flag bit
bool cpu_get_flag(CPU *cpu, uint8_t flag) {
    return (cpu->flags & flag) != 0;
}

//=========================================================
// Stack operations (grows downward)
//=========================================================

// Push a 16-bit value onto the stack
void cpu_push(CPU *cpu, uint16_t value) {
    // Stack grows downward: decrement SP by 2, then store
    cpu->regs[REG_SP] -= 2;
    cpu_write_word(cpu, cpu->regs[REG_SP], value);
}

// Pop a 16-bit value from the stack
uint16_t cpu_pop(CPU *cpu) {
    uint16_t value = cpu_read_word(cpu, cpu->regs[REG_SP]);
    cpu->regs[REG_SP] += 2;
    return value;
}

//=========================================================
// Fetch–Decode–Execute: single step
//=========================================================

// Execute one instruction at PC. Returns:
//   > 0: successfully executed one instruction
//   = 0: CPU was already halted (no-op)
//   < 0: error (e.g., division by zero, unknown opcode)
int cpu_step(CPU *cpu) {
    if (cpu->halted) {
        // Once halted, step is a no-op
        return 0;
    }
    
    // ===== FETCH =====
    uint16_t pc = cpu->regs[REG_PC];      // local copy of PC
    uint8_t opcode = cpu_read_byte(cpu, pc++);  // fetch opcode and advance PC
    
    // Update timer each instruction if enabled
    if (cpu->timer_enabled) {
        cpu->timer_value++;
    }
    
    // ===== DECODE & EXECUTE =====
    switch (opcode) {
        case OP_NOP:
            // No operation
            break;
            
        //------------- LOAD r, imm16 -------------
        case OP_LOAD_IMM: {
            uint8_t  reg = cpu_read_byte(cpu, pc++);
            uint16_t imm = cpu_read_word(cpu, pc);
            pc += 2;
            cpu_set_reg(cpu, reg, imm);
            break;
        }
        
        //------------- LOAD r, [addr] -------------
        case OP_LOAD_MEM: {
            uint8_t  reg  = cpu_read_byte(cpu, pc++);
            uint16_t addr = cpu_read_word(cpu, pc);
            pc += 2;
            // LOAD reads a 16-bit word from memory into the register
            uint16_t value = cpu_read_word(cpu, addr);
            cpu_set_reg(cpu, reg, value);
            break;
        }
        
        //------------- STORE [addr], r -------------
        case OP_STORE: {
            uint16_t addr = cpu_read_word(cpu, pc);
            pc += 2;
            uint8_t reg  = cpu_read_byte(cpu, pc++);
            cpu_write_word(cpu, addr, cpu_get_reg(cpu, reg));
            break;
        }
        
        //------------- MOV r1, r2 -------------
        case OP_MOV: {
            uint8_t byte = cpu_read_byte(cpu, pc++);
            uint8_t r1   = (byte >> 4) & 0x0F;
            uint8_t r2   = byte & 0x0F;
            cpu_set_reg(cpu, r1, cpu_get_reg(cpu, r2));
            break;
        }
        
        //------------- PUSH r -------------
        case OP_PUSH: {
            uint8_t reg = cpu_read_byte(cpu, pc++);
            cpu_push(cpu, cpu_get_reg(cpu, reg));
            break;
        }
        
        //------------- POP r -------------
        case OP_POP: {
            uint8_t reg = cpu_read_byte(cpu, pc++);
            cpu_set_reg(cpu, reg, cpu_pop(cpu));
            break;
        }
        
        //=================================================
        // ARITHMETIC
        //=================================================
        case OP_ADD: {
            uint8_t byte = cpu_read_byte(cpu, pc++);
            uint8_t r1   = (byte >> 4) & 0x0F;
            uint8_t r2   = byte & 0x0F;
            uint32_t a   = cpu_get_reg(cpu, r1);
            uint32_t b   = cpu_get_reg(cpu, r2);
            uint32_t result = a + b;
            cpu_set_reg(cpu, r1, result & 0xFFFF);
            bool carry    = result > 0xFFFF;
            bool overflow = ((a ^ result) & (b ^ result) & 0x8000) != 0;
            cpu_set_flags_arithmetic(cpu, result, carry, overflow);
            break;
        }
        
        case OP_ADDI: {
            uint8_t  reg = cpu_read_byte(cpu, pc++);
            uint16_t imm = cpu_read_word(cpu, pc);
            pc += 2;
            uint32_t a      = cpu_get_reg(cpu, reg);
            uint32_t result = a + imm;
            cpu_set_reg(cpu, reg, result & 0xFFFF);
            bool carry    = result > 0xFFFF;
            bool overflow = ((a ^ result) & (imm ^ result) & 0x8000) != 0;
            cpu_set_flags_arithmetic(cpu, result, carry, overflow);
            break;
        }
        
        case OP_SUB: {
            uint8_t byte = cpu_read_byte(cpu, pc++);
            uint8_t r1   = (byte >> 4) & 0x0F;
            uint8_t r2   = byte & 0x0F;
            uint32_t a   = cpu_get_reg(cpu, r1);
            uint32_t b   = cpu_get_reg(cpu, r2);
            uint32_t result = a - b;
            cpu_set_reg(cpu, r1, result & 0xFFFF);
            bool carry    = a < b;  // borrow in subtraction
            bool overflow = ((a ^ b) & (a ^ result) & 0x8000) != 0;
            cpu_set_flags_arithmetic(cpu, result, carry, overflow);
            break;
        }
        
        case OP_SUBI: {
            uint8_t  reg = cpu_read_byte(cpu, pc++);
            uint16_t imm = cpu_read_word(cpu, pc);
            pc += 2;
            uint32_t a      = cpu_get_reg(cpu, reg);
            uint32_t result = a - imm;
            cpu_set_reg(cpu, reg, result & 0xFFFF);
            bool carry    = a < imm;
            bool overflow = ((a ^ imm) & (a ^ result) & 0x8000) != 0;
            cpu_set_flags_arithmetic(cpu, result, carry, overflow);
            break;
        }
        
        case OP_MUL: {
            uint8_t byte = cpu_read_byte(cpu, pc++);
            uint8_t r1   = (byte >> 4) & 0x0F;
            uint8_t r2   = byte & 0x0F;
            uint32_t result = (uint32_t)cpu_get_reg(cpu, r1) * cpu_get_reg(cpu, r2);
            cpu_set_reg(cpu, r1, result & 0xFFFF);
            cpu_set_flags_arithmetic(cpu, result, result > 0xFFFF, false);
            break;
        }
        
        case OP_DIV: {
            uint8_t byte = cpu_read_byte(cpu, pc++);
            uint8_t r1   = (byte >> 4) & 0x0F;
            uint8_t r2   = byte & 0x0F;
            uint16_t divisor = cpu_get_reg(cpu, r2);
            if (divisor == 0) {
                fprintf(stderr, "Division by zero at PC=0x%04X\n", cpu->regs[REG_PC]);
                cpu->halted = true;
                return -1;
            }
            uint16_t dividend  = cpu_get_reg(cpu, r1);
            uint16_t quotient  = dividend / divisor;
            uint16_t remainder = dividend % divisor;
            cpu_set_reg(cpu, r1, quotient);
            cpu_set_reg(cpu, r2, remainder);
            cpu_set_flags_arithmetic(cpu, quotient, false, false);
            break;
        }
        
        case OP_INC: {
            uint8_t  reg   = cpu_read_byte(cpu, pc++);
            uint16_t value = cpu_get_reg(cpu, reg) + 1;
            cpu_set_reg(cpu, reg, value);
            cpu_set_flags_arithmetic(cpu, value, false, false);
            break;
        }
        
        case OP_DEC: {
            uint8_t  reg   = cpu_read_byte(cpu, pc++);
            uint16_t value = cpu_get_reg(cpu, reg) - 1;
            cpu_set_reg(cpu, reg, value);
            cpu_set_flags_arithmetic(cpu, value, false, false);
            break;
        }
        
        //=================================================
        // LOGIC
        //=================================================
        case OP_AND: {
            uint8_t byte = cpu_read_byte(cpu, pc++);
            uint8_t r1   = (byte >> 4) & 0x0F;
            uint8_t r2   = byte & 0x0F;
            uint16_t result = cpu_get_reg(cpu, r1) & cpu_get_reg(cpu, r2);
            cpu_set_reg(cpu, r1, result);
            cpu_set_flags_arithmetic(cpu, result, false, false);
            break;
        }
        
        case OP_OR: {
            uint8_t byte = cpu_read_byte(cpu, pc++);
            uint8_t r1   = (byte >> 4) & 0x0F;
            uint8_t r2   = byte & 0x0F;
            uint16_t result = cpu_get_reg(cpu, r1) | cpu_get_reg(cpu, r2);
            cpu_set_reg(cpu, r1, result);
            cpu_set_flags_arithmetic(cpu, result, false, false);
            break;
        }
        
        case OP_XOR: {
            uint8_t byte = cpu_read_byte(cpu, pc++);
            uint8_t r1   = (byte >> 4) & 0x0F;
            uint8_t r2   = byte & 0x0F;
            uint16_t result = cpu_get_reg(cpu, r1) ^ cpu_get_reg(cpu, r2);
            cpu_set_reg(cpu, r1, result);
            cpu_set_flags_arithmetic(cpu, result, false, false);
            break;
        }
        
        case OP_NOT: {
            uint8_t  reg    = cpu_read_byte(cpu, pc++);
            uint16_t result = ~cpu_get_reg(cpu, reg);
            cpu_set_reg(cpu, reg, result);
            cpu_set_flags_arithmetic(cpu, result, false, false);
            break;
        }
        
        case OP_SHL: {
            uint8_t reg   = cpu_read_byte(cpu, pc++);
            uint8_t shift = cpu_read_byte(cpu, pc++);
            uint16_t value  = cpu_get_reg(cpu, reg);
            uint16_t result = value << shift;
            cpu_set_reg(cpu, reg, result);
            // Carry: bit that falls off the left when shifting
            bool carry = shift > 0 && (value & (1 << (16 - shift))) != 0;
            cpu_set_flags_arithmetic(cpu, result, carry, false);
            break;
        }
        
        case OP_SHR: {
            uint8_t reg   = cpu_read_byte(cpu, pc++);
            uint8_t shift = cpu_read_byte(cpu, pc++);
            uint16_t value  = cpu_get_reg(cpu, reg);
            uint16_t result = value >> shift;
            cpu_set_reg(cpu, reg, result);
            // Carry: bit that falls off the right when shifting
            bool carry = shift > 0 && (value & (1 << (shift - 1))) != 0;
            cpu_set_flags_arithmetic(cpu, result, carry, false);
            break;
        }
        
        //=================================================
        // COMPARISON (sets flags, no writeback)
        //=================================================
        case OP_CMP: {
            uint8_t byte = cpu_read_byte(cpu, pc++);
            uint8_t r1   = (byte >> 4) & 0x0F;
            uint8_t r2   = byte & 0x0F;
            uint32_t a   = cpu_get_reg(cpu, r1);
            uint32_t b   = cpu_get_reg(cpu, r2);
            uint32_t result = a - b;
            bool carry    = a < b;
            bool overflow = ((a ^ b) & (a ^ result) & 0x8000) != 0;
            cpu_set_flags_arithmetic(cpu, result, carry, overflow);
            break;
        }
        
        case OP_CMPI: {
            uint8_t  reg = cpu_read_byte(cpu, pc++);
            uint16_t imm = cpu_read_word(cpu, pc);
            pc += 2;
            uint32_t a      = cpu_get_reg(cpu, reg);
            uint32_t result = a - imm;
            bool carry    = a < imm;
            bool overflow = ((a ^ imm) & (a ^ result) & 0x8000) != 0;
            cpu_set_flags_arithmetic(cpu, result, carry, overflow);
            break;
        }
        
        //=================================================
        // CONTROL FLOW
        //=================================================
        case OP_JMP: {
            uint16_t addr = cpu_read_word(cpu, pc);
            pc = addr;
            break;
        }
        
        case OP_JZ: {
            uint16_t addr = cpu_read_word(cpu, pc);
            pc += 2;
            if (cpu_get_flag(cpu, FLAG_ZERO)) {
                pc = addr;
            }
            break;
        }
        
        case OP_JNZ: {
            uint16_t addr = cpu_read_word(cpu, pc);
            pc += 2;
            if (!cpu_get_flag(cpu, FLAG_ZERO)) {
                pc = addr;
            }
            break;
        }
        
        case OP_JC: {
            uint16_t addr = cpu_read_word(cpu, pc);
            pc += 2;
            if (cpu_get_flag(cpu, FLAG_CARRY)) {
                pc = addr;
            }
            break;
        }
        
        case OP_JNC: {
            uint16_t addr = cpu_read_word(cpu, pc);
            pc += 2;
            if (!cpu_get_flag(cpu, FLAG_CARRY)) {
                pc = addr;
            }
            break;
        }
        
        case OP_CALL: {
            uint16_t addr = cpu_read_word(cpu, pc);
            pc += 2;
            // Push return address (next instruction) on stack
            cpu_push(cpu, pc);
            // Jump to subroutine
            pc = addr;
            break;
        }
        
        case OP_RET: {
            // Pop return address and jump to it
            pc = cpu_pop(cpu);
            break;
        }
        
        //=================================================
        // I/O
        //=================================================
        case OP_IN: {
            uint8_t  reg  = cpu_read_byte(cpu, pc++);
            uint16_t port = cpu_read_word(cpu, pc);
            pc += 2;
            uint8_t value = cpu_read_byte(cpu, port);
            cpu_set_reg(cpu, reg, value);
            break;
        }
        
        case OP_OUT: {
            uint16_t port = cpu_read_word(cpu, pc);
            pc += 2;
            uint8_t  reg  = cpu_read_byte(cpu, pc++);
            uint16_t value = cpu_get_reg(cpu, reg);
            cpu_write_byte(cpu, port, value & 0xFF);
            break;
        }
        
        //=================================================
        // SYSTEM
        //=================================================
        case OP_HLT:
            cpu->halted  = true;
            cpu->running = false;
            break;
            
        // Unknown opcode: treat as fatal error
        default:
            fprintf(stderr, "Unknown opcode 0x%02X at PC=0x%04X\n", opcode, cpu->regs[REG_PC]);
            cpu->halted = true;
            return -1;
    }
    
    // Commit updated PC and increment cycle counter
    cpu->regs[REG_PC] = pc;
    cpu->cycles++;
    
    return 1;
}

//=========================================================
// High-level execution
//=========================================================

// Run until a HLT instruction or an error occurs
void cpu_run(CPU *cpu) {
    cpu->running = true;
    cpu->halted  = false;
    
    while (cpu->running && !cpu->halted) {
        if (cpu_step(cpu) < 0) {
            // Error (e.g., divide by zero, unknown opcode)
            break;
        }
    }
}

//=========================================================
// Debug / inspection helpers
//=========================================================

// Print register file and flags in a human-readable format
void cpu_dump_registers(CPU *cpu) {
    printf("\n=== CPU Registers ===\n");
    printf("A:  0x%04X (%d)\n", cpu->regs[REG_A], cpu->regs[REG_A]);
    printf("B:  0x%04X (%d)\n", cpu->regs[REG_B], cpu->regs[REG_B]);
    printf("C:  0x%04X (%d)\n", cpu->regs[REG_C], cpu->regs[REG_C]);
    printf("D:  0x%04X (%d)\n", cpu->regs[REG_D], cpu->regs[REG_D]);
    printf("SP: 0x%04X\n",       cpu->regs[REG_SP]);
    printf("PC: 0x%04X\n",       cpu->regs[REG_PC]);
    printf("FLAGS: [%c%c%c%c] (0x%02X)\n",
           cpu_get_flag(cpu, FLAG_ZERO)     ? 'Z' : '-',
           cpu_get_flag(cpu, FLAG_CARRY)    ? 'C' : '-',
           cpu_get_flag(cpu, FLAG_NEGATIVE) ? 'N' : '-',
           cpu_get_flag(cpu, FLAG_OVERFLOW) ? 'O' : '-',
           cpu->flags);
    printf("Cycles: %llu\n", (unsigned long long)cpu->cycles);
    printf("=====================\n\n");
}

// Dump a block of memory as hex
void cpu_dump_memory(CPU *cpu, uint16_t start, uint16_t end) {
    printf("\n=== Memory Dump [0x%04X - 0x%04X] ===\n", start, end);
    for (uint16_t addr = start; addr <= end; addr += 16) {
        printf("0x%04X: ", addr);
        for (int i = 0; i < 16 && addr + i <= end; i++) {
            printf("%02X ", cpu->memory[addr + i]);
        }
        printf("\n");
    }
    printf("=====================================\n\n");
}

// Print the top of the stack (for debugging CALL/RET/PUSH/POP)
void cpu_dump_stack(CPU *cpu, int depth) {
    printf("\n=== Stack (top %d entries) ===\n", depth);
    uint16_t sp = cpu->regs[REG_SP];
    for (int i = 0; i < depth; i++) {
        uint16_t addr = sp + (i * 2);
        if (addr >= 0xFEFF) break;
        uint16_t value = cpu_read_word(cpu, addr);
        printf("SP+%d (0x%04X): 0x%04X\n", i * 2, addr, value);
    }
    printf("==============================\n\n");
}
