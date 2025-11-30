#include "assembler.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Enable strdup on some systems
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

//=========================================================
// Assembler initialization / helpers
//=========================================================

// Initialize assembler context to a clean state
void asm_init(Assembler *asm_ctx) {
    // Clear all fields in the Assembler struct
    memset(asm_ctx, 0, sizeof(Assembler));
    asm_ctx->output_size = 0;     // no bytes emitted yet
    asm_ctx->label_count = 0;     // no labels yet
    asm_ctx->current_line = 0;    // current line index for error messages
    asm_ctx->has_errors = false;  // no errors so far
}

// Parse register name (A, B, C, D, SP, PC) into numeric code
int asm_parse_register(const char *str) {
    if (strcmp(str, "A") == 0)  return REG_A;
    if (strcmp(str, "B") == 0)  return REG_B;
    if (strcmp(str, "C") == 0)  return REG_C;
    if (strcmp(str, "D") == 0)  return REG_D;
    if (strcmp(str, "SP") == 0) return REG_SP;
    if (strcmp(str, "PC") == 0) return REG_PC;
    return -1; // invalid register
}

// Parse a numeric literal (decimal or hex with 0x prefix) into uint16_t
int asm_parse_number(const char *str, uint16_t *value) {
    char *endptr;
    long num;
    
    // Detect hex using 0x or 0X prefix, otherwise treat as decimal
    if (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0) {
        num = strtol(str, &endptr, 16);
    } else {
        num = strtol(str, &endptr, 10);
    }
    
    // If we stopped at a non-space, the number is malformed
    if (*endptr != '\0' && !isspace(*endptr)) {
        return -1;  // Invalid number
    }
    
    *value = (uint16_t)num;
    return 0;
}

//=========================================================
// Label table management
//=========================================================

// Add a label to the symbol table with its address
int asm_add_label(Assembler *asm_ctx, const char *name, uint16_t address) {
    // Check label capacity
    if (asm_ctx->label_count >= MAX_LABELS) {
        fprintf(stderr, "Error: Too many labels\n");
        return -1;
    }
    
    // Check for duplicate label names
    for (int i = 0; i < asm_ctx->label_count; i++) {
        if (strcmp(asm_ctx->labels[i].name, name) == 0) {
            fprintf(stderr, "Error: Duplicate label '%s'\n", name);
            return -1;
        }
    }
    
    // Store label name and address
    strncpy(asm_ctx->labels[asm_ctx->label_count].name, name, MAX_LABEL_LEN - 1);
    asm_ctx->labels[asm_ctx->label_count].address = address;
    asm_ctx->label_count++;
    
    return 0;
}

// Find label by name and return its address through 'address'
int asm_find_label(Assembler *asm_ctx, const char *name, uint16_t *address) {
    for (int i = 0; i < asm_ctx->label_count; i++) {
        if (strcmp(asm_ctx->labels[i].name, name) == 0) {
            *address = asm_ctx->labels[i].address;
            return 0;
        }
    }
    return -1; // not found
}

//=========================================================
// Output emission helpers
//=========================================================

// Emit a single byte into the output buffer (if there's space)
static void emit_byte(Assembler *asm_ctx, uint8_t byte) {
    if (asm_ctx->output_size < MAX_PROGRAM_SIZE) {
        asm_ctx->output[asm_ctx->output_size++] = byte;
    }
    // If we ran out of space, we silently drop bytes; could be improved
}

// Emit a 16-bit word in little-endian order (low byte first)
static void emit_word(Assembler *asm_ctx, uint16_t word) {
    emit_byte(asm_ctx, word & 0xFF);
    emit_byte(asm_ctx, (word >> 8) & 0xFF);
}

//=========================================================
// String / parsing utilities
//=========================================================

// Trim leading and trailing whitespace in-place.
// Returns pointer to first non-space character.
static char *trim(char *str) {
    // Skip leading spaces
    while (isspace(*str)) str++;
    // If all spaces, this returns pointer to '\0' at end
    
    // Find end of string
    char *end = str + strlen(str) - 1;
    // Move backward over trailing spaces
    while (end > str && isspace(*end)) end--;
    // Null-terminate right after last non-space
    *(end + 1) = '\0';
    return str;
}

//=========================================================
// Core line parser: one line of assembly → emitted bytes
//=========================================================

static int asm_parse_line(Assembler *asm_ctx, char *line) {
    // First trim leading/trailing whitespace
    line = trim(line);
    
    // Skip empty lines
    if (line[0] == '\0') {
        return 0;
    }
    
    // Strip inline comments.
    // We treat both ';' and '#' as comment starters.
    char *comment = strchr(line, ';');
    if (comment) *comment = '\0';
    comment = strchr(line, '#');
    if (comment) *comment = '\0';
    
    // Trim again after chopping comments
    line = trim(line);
    
    // If nothing remains after removing comments, skip
    if (line[0] == '\0') {
        return 0;
    }
    
    //------------------------------
    // Label handling: "LABEL:"
    //------------------------------
    char *colon = strchr(line, ':');
    if (colon) {
        *colon = '\0';
        char *label = trim(line);
        // Normalize label to uppercase to avoid case issues
        for (char *p = label; *p; p++) *p = toupper(*p);
        // Compute label address as base program address (0x0100)
        // plus current output size (instruction bytes so far).
        uint16_t label_addr = 0x0100 + asm_ctx->output_size;
        printf("DEBUG: Storing label '%s' at address 0x%04X\n", label, label_addr);
        if (asm_add_label(asm_ctx, label, label_addr) < 0) {
            return -1;
        }
        // Move to text after the colon; might be an instruction
        line = trim(colon + 1);
        if (line[0] == '\0') return 0;  // Label-only line
    }
    
    //------------------------------
    // Split into instruction and arguments
    //------------------------------
    char instr[32] = {0};
    char args[MAX_LINE_LEN] = {0};
    
    // Find first space to split mnemonic from argument string
    char *space = strchr(line, ' ');
    if (space) {
        size_t instr_len = space - line;
        strncpy(instr, line, instr_len);
        instr[instr_len] = '\0';
        strcpy(args, trim(space + 1));  // everything after mnemonic is args
    } else {
        // No space → whole line is instruction (no arguments)
        strcpy(instr, line);
    }
    
    // Normalize instruction mnemonic to uppercase
    for (char *p = instr; *p; p++) *p = toupper(*p);
    
    //------------------------------
    // Split arguments into arg1, arg2 (comma-separated)
    //------------------------------
    char arg1[64] = {0}, arg2[64] = {0};
    if (args[0]) {
        char *comma = strchr(args, ',');
        if (comma) {
            // Two-operand instruction: split at comma
            size_t len = comma - args;
            strncpy(arg1, args, len);
            arg1[len] = '\0';
            strcpy(arg2, comma + 1);
            // Trim both arguments
            char *t1 = trim(arg1);
            char *t2 = trim(arg2);
            if (t1 != arg1) memmove(arg1, t1, strlen(t1) + 1);
            if (t2 != arg2) memmove(arg2, t2, strlen(t2) + 1);
            // Uppercase non-literal arguments (register names, labels)
            if (arg1[0] != '0' && arg1[0] != '[') {
                for (char *p = arg1; *p; p++) *p = toupper(*p);
            }
            if (arg2[0] != '0' && arg2[0] != '[') {
                for (char *p = arg2; *p; p++) *p = toupper(*p);
            }
        } else {
            // Single-operand instruction: whole args string is arg1
            strcpy(arg1, args);
            char *t1 = trim(arg1);
            if (t1 != arg1) memmove(arg1, t1, strlen(t1) + 1);
            // Uppercase if not a numeric literal or memory address
            if (arg1[0] != '0' && arg1[0] != '[') {
                for (char *p = arg1; *p; p++) *p = toupper(*p);
            }
        }
    }
    
    //------------------------------
    // Instruction encoding
    //------------------------------

    if (strcmp(instr, "NOP") == 0) {
        emit_byte(asm_ctx, OP_NOP);
    }
    else if (strcmp(instr, "HLT") == 0) {
        emit_byte(asm_ctx, OP_HLT);
    }
    //--------------- LOAD r, imm / LOAD r, [addr] ---------------
    else if (strcmp(instr, "LOAD") == 0) {
        int reg = asm_parse_register(arg1);
        if (reg < 0) {
            fprintf(stderr, "Line %d: Invalid register '%s'\n", asm_ctx->current_line, arg1);
            return -1;
        }
        
        // Memory load: LOAD r, [addr]
        if (arg2[0] == '[') {
            char *end = strchr(arg2, ']');
            if (!end) {
                fprintf(stderr, "Line %d: Missing ']'\n", asm_ctx->current_line);
                return -1;
            }
            *end = '\0';
            uint16_t addr;
            if (asm_parse_number(arg2 + 1, &addr) < 0) {
                fprintf(stderr, "Line %d: Invalid address\n", asm_ctx->current_line);
                return -1;
            }
            emit_byte(asm_ctx, OP_LOAD_MEM);
            emit_byte(asm_ctx, reg);
            emit_word(asm_ctx, addr);
        } else {
            // Immediate load: LOAD r, imm
            uint16_t imm;
            if (asm_parse_number(arg2, &imm) < 0) {
                fprintf(stderr, "Line %d: Invalid immediate value '%s'\n", asm_ctx->current_line, arg2);
                return -1;
            }
            emit_byte(asm_ctx, OP_LOAD_IMM);
            emit_byte(asm_ctx, reg);
            emit_word(asm_ctx, imm);
        }
    }
    //--------------- STORE [addr], r ---------------
    else if (strcmp(instr, "STORE") == 0) {
        // Destination must be [addr]
        if (arg1[0] != '[') {
            fprintf(stderr, "Line %d: STORE requires [addr] format\n", asm_ctx->current_line);
            return -1;
        }
        char *end = strchr(arg1, ']');
        if (!end) {
            fprintf(stderr, "Line %d: Missing ']'\n", asm_ctx->current_line);
            return -1;
        }
        *end = '\0';
        uint16_t addr;
        if (asm_parse_number(arg1 + 1, &addr) < 0) {
            fprintf(stderr, "Line %d: Invalid address\n", asm_ctx->current_line);
            return -1;
        }
        int reg = asm_parse_register(arg2);
        if (reg < 0) {
            fprintf(stderr, "Line %d: Invalid register '%s'\n", asm_ctx->current_line, arg2);
            return -1;
        }
        emit_byte(asm_ctx, OP_STORE);
        emit_word(asm_ctx, addr);
        emit_byte(asm_ctx, reg);
    }
    //--------------- MOV r1, r2 ---------------
    else if (strcmp(instr, "MOV") == 0) {
        int r1 = asm_parse_register(arg1);
        int r2 = asm_parse_register(arg2);
        if (r1 < 0 || r2 < 0) {
            fprintf(stderr, "Line %d: Invalid registers\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_MOV);
        emit_byte(asm_ctx, (r1 << 4) | r2);
    }
    //--------------- PUSH r ---------------
    else if (strcmp(instr, "PUSH") == 0) {
        int reg = asm_parse_register(arg1);
        if (reg < 0) {
            fprintf(stderr, "Line %d: Invalid register\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_PUSH);
        emit_byte(asm_ctx, reg);
    }
    //--------------- POP r ---------------
    else if (strcmp(instr, "POP") == 0) {
        int reg = asm_parse_register(arg1);
        if (reg < 0) {
            fprintf(stderr, "Line %d: Invalid register\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_POP);
        emit_byte(asm_ctx, reg);
    }
    //--------------- ADD r1, r2 ---------------
    else if (strcmp(instr, "ADD") == 0) {
        int r1 = asm_parse_register(arg1);
        int r2 = asm_parse_register(arg2);
        if (r1 < 0 || r2 < 0) {
            fprintf(stderr, "Line %d: Invalid registers\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_ADD);
        emit_byte(asm_ctx, (r1 << 4) | r2);
    }
    //--------------- ADDI r, imm ---------------
    else if (strcmp(instr, "ADDI") == 0) {
        int reg = asm_parse_register(arg1);
        uint16_t imm;
        if (reg < 0 || asm_parse_number(arg2, &imm) < 0) {
            fprintf(stderr, "Line %d: Invalid ADDI arguments\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_ADDI);
        emit_byte(asm_ctx, reg);
        emit_word(asm_ctx, imm);
    }
    //--------------- SUB r1, r2 ---------------
    else if (strcmp(instr, "SUB") == 0) {
        int r1 = asm_parse_register(arg1);
        int r2 = asm_parse_register(arg2);
        if (r1 < 0 || r2 < 0) {
            fprintf(stderr, "Line %d: Invalid registers\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_SUB);
        emit_byte(asm_ctx, (r1 << 4) | r2);
    }
    //--------------- SUBI r, imm ---------------
    else if (strcmp(instr, "SUBI") == 0) {
        int reg = asm_parse_register(arg1);
        uint16_t imm;
        if (reg < 0 || asm_parse_number(arg2, &imm) < 0) {
            fprintf(stderr, "Line %d: Invalid SUBI arguments\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_SUBI);
        emit_byte(asm_ctx, reg);
        emit_word(asm_ctx, imm);
    }
    //--------------- MUL r1, r2 ---------------
    else if (strcmp(instr, "MUL") == 0) {
        int r1 = asm_parse_register(arg1);
        int r2 = asm_parse_register(arg2);
        if (r1 < 0 || r2 < 0) {
            fprintf(stderr, "Line %d: Invalid registers\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_MUL);
        emit_byte(asm_ctx, (r1 << 4) | r2);
    }
    //--------------- DIV r1, r2 ---------------
    else if (strcmp(instr, "DIV") == 0) {
        int r1 = asm_parse_register(arg1);
        int r2 = asm_parse_register(arg2);
        if (r1 < 0 || r2 < 0) {
            fprintf(stderr, "Line %d: Invalid registers\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_DIV);
        emit_byte(asm_ctx, (r1 << 4) | r2);
    }
    //--------------- INC r ---------------
    else if (strcmp(instr, "INC") == 0) {
        int reg = asm_parse_register(arg1);
        if (reg < 0) {
            fprintf(stderr, "Line %d: Invalid register\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_INC);
        emit_byte(asm_ctx, reg);
    }
    //--------------- DEC r ---------------
    else if (strcmp(instr, "DEC") == 0) {
        int reg = asm_parse_register(arg1);
        if (reg < 0) {
            fprintf(stderr, "Line %d: Invalid register\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_DEC);
        emit_byte(asm_ctx, reg);
    }
    //--------------- AND/OR/XOR r1, r2 ---------------
    else if (strcmp(instr, "AND") == 0 || strcmp(instr, "OR") == 0 || 
             strcmp(instr, "XOR") == 0) {
        int r1 = asm_parse_register(arg1);
        int r2 = asm_parse_register(arg2);
        if (r1 < 0 || r2 < 0) {
            fprintf(stderr, "Line %d: Invalid registers\n", asm_ctx->current_line);
            return -1;
        }
        uint8_t op = (strcmp(instr, "AND") == 0) ? OP_AND :
                     (strcmp(instr, "OR") == 0)  ? OP_OR  : OP_XOR;
        emit_byte(asm_ctx, op);
        emit_byte(asm_ctx, (r1 << 4) | r2);
    }
    //--------------- NOT r ---------------
    else if (strcmp(instr, "NOT") == 0) {
        int reg = asm_parse_register(arg1);
        if (reg < 0) {
            fprintf(stderr, "Line %d: Invalid register\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_NOT);
        emit_byte(asm_ctx, reg);
    }
    //--------------- SHL/SHR r, imm8 ---------------
    else if (strcmp(instr, "SHL") == 0 || strcmp(instr, "SHR") == 0) {
        int reg = asm_parse_register(arg1);
        uint16_t shift;
        if (reg < 0 || asm_parse_number(arg2, &shift) < 0) {
            fprintf(stderr, "Line %d: Invalid shift arguments\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, strcmp(instr, "SHL") == 0 ? OP_SHL : OP_SHR);
        emit_byte(asm_ctx, reg);
        emit_byte(asm_ctx, shift & 0xFF); // shift amount as 8-bit immediate
    }
    //--------------- CMP r1, r2 ---------------
    else if (strcmp(instr, "CMP") == 0) {
        int r1 = asm_parse_register(arg1);
        int r2 = asm_parse_register(arg2);
        if (r1 < 0 || r2 < 0) {
            fprintf(stderr, "Line %d: Invalid registers\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_CMP);
        emit_byte(asm_ctx, (r1 << 4) | r2);
    }
    //--------------- CMPI r, imm ---------------
    else if (strcmp(instr, "CMPI") == 0) {
        int reg = asm_parse_register(arg1);
        uint16_t imm;
        if (reg < 0 || asm_parse_number(arg2, &imm) < 0) {
            fprintf(stderr, "Line %d: Invalid CMPI arguments\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_CMPI);
        emit_byte(asm_ctx, reg);
        emit_word(asm_ctx, imm);
    }
    //--------------- Jumps/CALL label_or_addr ---------------
    else if (strcmp(instr, "JMP") == 0 || strcmp(instr, "JZ") == 0 ||
             strcmp(instr, "JNZ") == 0 || strcmp(instr, "JC") == 0 ||
             strcmp(instr, "JNC") == 0 || strcmp(instr, "CALL") == 0) {
        uint8_t op = (strcmp(instr, "JMP") == 0) ? OP_JMP :
                     (strcmp(instr, "JZ")  == 0) ? OP_JZ  :
                     (strcmp(instr, "JNZ") == 0) ? OP_JNZ :
                     (strcmp(instr, "JC")  == 0) ? OP_JC  :
                     (strcmp(instr, "JNC") == 0) ? OP_JNC : OP_CALL;
        
        uint16_t addr;
        // First, try to parse the operand as a numeric literal
        if (asm_parse_number(arg1, &addr) < 0) {
            // Not a number → treat as label and look it up in symbol table
            printf("DEBUG: Looking up label '%s'\n", arg1);
            if (asm_find_label(asm_ctx, arg1, &addr) < 0) {
                fprintf(stderr, "Line %d: Undefined label '%s'\n", 
                        asm_ctx->current_line, arg1);
                return -1;
            }
            printf("DEBUG: Found label '%s' at address 0x%04X\n", arg1, addr);
            emit_byte(asm_ctx, op);
            emit_word(asm_ctx, addr);  // Use resolved label address
        } else {
            // Direct numeric address
            emit_byte(asm_ctx, op);
            emit_word(asm_ctx, addr);
        }
    }
    //--------------- RET ---------------
    else if (strcmp(instr, "RET") == 0) {
        emit_byte(asm_ctx, OP_RET);
    }
    //--------------- OUT port, r ---------------
    else if (strcmp(instr, "OUT") == 0) {
        uint16_t port;
        if (asm_parse_number(arg1, &port) < 0) {
            fprintf(stderr, "Line %d: Invalid port number\n", asm_ctx->current_line);
            return -1;
        }
        int reg = asm_parse_register(arg2);
        if (reg < 0) {
            fprintf(stderr, "Line %d: Invalid register\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_OUT);
        emit_word(asm_ctx, port);
        emit_byte(asm_ctx, reg);
    }
    //--------------- IN r, port ---------------
    else if (strcmp(instr, "IN") == 0) {
        int reg = asm_parse_register(arg1);
        uint16_t port;
        if (reg < 0 || asm_parse_number(arg2, &port) < 0) {
            fprintf(stderr, "Line %d: Invalid IN arguments\n", asm_ctx->current_line);
            return -1;
        }
        emit_byte(asm_ctx, OP_IN);
        emit_byte(asm_ctx, reg);
        emit_word(asm_ctx, port);
    }
    //--------------- Unknown instruction ---------------
    else {
        fprintf(stderr, "Line %d: Unknown instruction '%s'\n", asm_ctx->current_line, instr);
        return -1;
    }
    
    return 0; // Success for this line
}

//=========================================================
// High-level assembly entry points
//=========================================================

// Assemble from an in-memory string (used by tests or tools)
int asm_assemble_string(Assembler *asm_ctx, const char *source) {
    // Make a modifiable copy for strtok
    char *source_copy = strdup(source);
    char *line = strtok(source_copy, "\n");
    
    asm_ctx->current_line = 0;
    
    // Process each line until strtok returns NULL
    while (line != NULL) {
        asm_ctx->current_line++;
        if (asm_parse_line(asm_ctx, line) < 0) {
            asm_ctx->has_errors = true;
            free(source_copy);
            return -1;
        }
        line = strtok(NULL, "\n");
    }
    
    free(source_copy);
    return 0;
}

// Assemble from a file on disk
int asm_assemble_file(Assembler *asm_ctx, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Cannot open file: %s\n", filename);
        return -1;
    }
    
    char line[MAX_LINE_LEN];
    asm_ctx->current_line = 0;
    
    // Read file line-by-line
    while (fgets(line, sizeof(line), f)) {
        asm_ctx->current_line++;
        // Strip newline characters
        line[strcspn(line, "\r\n")] = '\0';
        
        if (asm_parse_line(asm_ctx, line) < 0) {
            asm_ctx->has_errors = true;
            fclose(f);
            return -1;
        }
    }
    
    fclose(f);
    return 0;
}

// Write assembled program to a binary file
int asm_write_binary(Assembler *asm_ctx, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Cannot write file: %s\n", filename);
        return -1;
    }
    
    // Dump the raw output buffer to disk
    fwrite(asm_ctx->output, 1, asm_ctx->output_size, f);
    fclose(f);
    
    printf("Assembled %zu bytes to %s\n", asm_ctx->output_size, filename);
    return 0;
}
