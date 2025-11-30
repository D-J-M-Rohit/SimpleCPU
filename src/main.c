#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "assembler.h"

/**
 * print_usage
 *
 * Prints a help/usage summary for the SimpleCPU tool.
 * This is the "front panel" interface: users can either
 * assemble source, run binaries, or do both in one step.
 */
void print_usage(const char *prog_name) {
    printf("SimpleCPU Emulator and Assembler\n\n");
    printf("Usage:\n");
    printf("  %s assemble <input.asm> <output.bin>  - Assemble a program\n", prog_name);
    printf("  %s run <program.bin>                  - Run a binary program\n", prog_name);
    printf("  %s debug <program.bin>                - Run with debug output\n", prog_name);
    printf("  %s asm-run <program.asm>              - Assemble and run\n\n", prog_name);
    printf("  %s trace <program.bin>                - Step with per-cycle state\n\n", prog_name);
}

/**
 * cmd_assemble
 *
 * High-level wrapper around the assembler:
 *   1. Initializes assembler state.
 *   2. Reads and assembles the given .asm file.
 *   3. Writes a flat binary (.bin) suitable for the emulator.
 */
int cmd_assemble(const char *input_file, const char *output_file) {
    Assembler asm_ctx;
    asm_init(&asm_ctx);
    
    printf("Assembling %s...\n", input_file);
    
    // Pass 1 + 2 style assembly handled inside asm_assemble_file
    if (asm_assemble_file(&asm_ctx, input_file) < 0) {
        fprintf(stderr, "Assembly failed\n");
        return 1;
    }
    
    // Emit raw machine code bytes to output file
    if (asm_write_binary(&asm_ctx, output_file) < 0) {
        fprintf(stderr, "Failed to write output\n");
        return 1;
    }
    
    printf("Success! Output written to %s\n", output_file);
    return 0;
}

/**
 * cmd_run
 *
 * Takes a pre-assembled binary and:
 *   1. Creates a CPU instance.
 *   2. Loads program into memory at 0x0100.
 *   3. Optionally prints initial CPU state (debug mode).
 *   4. Calls cpu_run(), which repeatedly performs the
 *      Fetch–Decode–Execute cycle until HLT or error.
 */
int cmd_run(const char *binary_file, bool debug) {
    CPU cpu;
    cpu_init(&cpu);
    
    // ---- Load binary file into buffer ----
    FILE *f = fopen(binary_file, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open file: %s\n", binary_file);
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t *program = malloc(size);
    if (!program) {
        fclose(f);
        fprintf(stderr, "Out of memory\n");
        return 1;
    }

    fread(program, 1, size, f);
    fclose(f);
    
    // Load program bytes into SimpleCPU memory at 0x0100
    if (cpu_load_program(&cpu, program, size, 0x0100) < 0) {
        free(program);
        return 1;
    }
    
    free(program);
    
    // Optional: show initial register state before running
    if (debug) {
        printf("=== Starting Execution (Debug Mode) ===\n");
        printf("Program loaded at 0x0100, size: %zu bytes\n\n", size);
        cpu_dump_registers(&cpu);
    }
    
    // Main execution: repeatedly executes cpu_step() internally
    printf("=== Program Output ===\n");
    cpu_run(&cpu);
    printf("\n=== End Output ===\n\n");
    
    // Optional: show final registers and cycle count
    if (debug) {
        cpu_dump_registers(&cpu);
        printf("Program terminated after %llu cycles\n", 
               (unsigned long long)cpu.cycles);
    }
    
    return 0;
}

/**
 * cmd_trace
 *
 * Like cmd_run, but shows a per-instruction trace:
 *   CYC, PC, and register values before each cpu_step().
 * This is a very explicit view of the Fetch–Decode–Execute
 * cycle in action and is great for debugging / demonstration.
 */
int cmd_trace(const char *binary_file) {
    CPU cpu;
    cpu_init(&cpu);

    // ---- Load binary file into buffer ----
    FILE *f = fopen(binary_file, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open file: %s\n", binary_file);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *program = malloc(size);
    if (!program) {
        fclose(f);
        fprintf(stderr, "Out of memory\n");
        return 1;
    }

    fread(program, 1, size, f);
    fclose(f);

    // Load program into memory at 0x0100
    if (cpu_load_program(&cpu, program, size, 0x0100) < 0) {
        free(program);
        return 1;
    }

    free(program);

    printf("=== Execution Trace ===\n");
    // Step until HLT or error
    while (!cpu.halted) {
        // Show cycle count, PC, and all general-purpose registers
        printf("CYC=%10llu PC=%04X A=%04X B=%04X C=%04X D=%04X\n",
               (unsigned long long)cpu.cycles,
               cpu.regs[REG_PC],
               cpu.regs[REG_A],
               cpu.regs[REG_B],
               cpu.regs[REG_C],
               cpu.regs[REG_D]);
        if (cpu_step(&cpu) < 0) {
            // An error occurred (e.g. bad opcode / divide by zero)
            break;
        }
    }
    printf("=== End Trace ===\n");
    printf("Total cycles: %llu\n", (unsigned long long)cpu.cycles);

    return 0;
}

/**
 * cmd_asm_run
 *
 * Convenience command:
 *   1. Assemble a .asm file in memory.
 *   2. Load assembled bytes directly into a fresh CPU.
 *   3. Run the program (optionally with debug output).
 *
 * This shows the full toolchain: source → machine code → execution.
 */
int cmd_asm_run(const char *asm_file, bool debug) {
    Assembler asm_ctx;
    asm_init(&asm_ctx);
    
    printf("Assembling %s...\n", asm_file);
    
    if (asm_assemble_file(&asm_ctx, asm_file) < 0) {
        fprintf(stderr, "Assembly failed\n");
        return 1;
    }
    
    printf("Assembled %zu bytes\n\n", asm_ctx.output_size);
    
    // Create CPU and load assembled program into memory
    CPU cpu;
    cpu_init(&cpu);
    
    if (cpu_load_program(&cpu, asm_ctx.output, asm_ctx.output_size, 0x0100) < 0) {
        return 1;
    }
    
    if (debug) {
        printf("=== Starting Execution (Debug Mode) ===\n");
        cpu_dump_registers(&cpu);
    }
    
    // Execute until HLT
    printf("=== Program Output ===\n");
    cpu_run(&cpu);
    printf("\n=== End Output ===\n\n");
    
    if (debug) {
        cpu_dump_registers(&cpu);
        printf("Program terminated after %llu cycles\n", 
               (unsigned long long)cpu.cycles);
    }
    
    return 0;
}

/**
 * main
 *
 * Command dispatcher for the SimpleCPU environment.
 *
 * Conceptually, this acts as the "Control Unit" for the whole
 * software system, deciding whether we:
 *   - Just assemble (assemble)
 *   - Just emulate (run/debug/trace)
 *   - Assemble then emulate in one shot (asm-run/asm-debug)
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *command = argv[1];
    
    if (strcmp(command, "assemble") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Error: assemble requires input and output files\n");
            print_usage(argv[0]);
            return 1;
        }
        return cmd_assemble(argv[2], argv[3]);
    }
    else if (strcmp(command, "run") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: run requires a binary file\n");
            print_usage(argv[0]);
            return 1;
        }
        return cmd_run(argv[2], false);
    }
    else if (strcmp(command, "debug") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: debug requires a binary file\n");
            print_usage(argv[0]);
            return 1;
        }
        return cmd_run(argv[2], true);
    }
    else if (strcmp(command, "trace") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: trace requires a binary file\n");
            print_usage(argv[0]);
            return 1;
        }
        return cmd_trace(argv[2]);
    }
    else if (strcmp(command, "asm-run") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: asm-run requires an assembly file\n");
            print_usage(argv[0]);
            return 1;
        }
        return cmd_asm_run(argv[2], false);
    }
    else if (strcmp(command, "asm-debug") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: asm-debug requires an assembly file\n");
            print_usage(argv[0]);
            return 1;
        }
        return cmd_asm_run(argv[2], true);
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
}
