#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

//=========================================================
// Assembler configuration constants
//=========================================================

// Maximum number of distinct labels allowed in a single program
#define MAX_LABELS 256

// Maximum length (in characters) of a label name, including '\0'
#define MAX_LABEL_LEN 64

// Maximum length of a single source line (for reading from file)
#define MAX_LINE_LEN 256

// Maximum size of assembled program in bytes (64 KB, matches MEMORY_SIZE)
#define MAX_PROGRAM_SIZE 0x10000

//=========================================================
// Label structure
//=========================================================
//
// Each label has:
//   - name: its textual identifier in the assembly source
//   - address: its resolved 16-bit address in memory (e.g., 0x0100 + offset)
//
typedef struct {
    char name[MAX_LABEL_LEN];
    uint16_t address;
} Label;

//=========================================================
// Assembler state
//=========================================================
//
// The Assembler struct holds all state needed during a single
// assembly pass:
//   - output:   raw machine code bytes being generated
//   - output_size: number of bytes currently in 'output'
//   - labels:   the symbol table mapping label names → addresses
//   - label_count: how many labels have been defined so far
//   - current_line: 1-based line number in the source (for errors)
//   - has_errors: set to true if any error occurred while assembling
//
typedef struct {
    uint8_t output[MAX_PROGRAM_SIZE];  // machine code buffer
    size_t  output_size;               // number of valid bytes in 'output'
    
    Label labels[MAX_LABELS];          // label symbol table
    int   label_count;                 // number of labels currently stored
    
    int  current_line;                 // current source line (for diagnostics)
    bool has_errors;                   // flag: did any error occur?
} Assembler;

//=========================================================
// Public assembler API
//=========================================================
//
// asm_init:
//   Initialize an Assembler struct to a clean state.
//
// asm_assemble_file:
//   Parse and assemble a program from a file on disk into asm_ctx->output.
//
// asm_assemble_string:
//   Parse and assemble a program from an in-memory string.
//
// asm_write_binary:
//   Write the assembled bytes in asm_ctx->output to a binary file.
//
void asm_init(Assembler *asm_ctx);
int  asm_assemble_file(Assembler *asm_ctx, const char *filename);
int  asm_assemble_string(Assembler *asm_ctx, const char *source);
int  asm_write_binary(Assembler *asm_ctx, const char *filename);

//=========================================================
// Helper functions (also part of the public API)
//=========================================================
//
// asm_parse_register:
//   Convert a register name string (e.g., "A", "SP") into its numeric code.
//   Returns -1 if the name is invalid.
//
// asm_parse_number:
//   Parse a numeric string in decimal or hex (0x...) into a uint16_t.
//   Returns 0 on success, -1 on parse error.
//
// asm_add_label:
//   Store a label and its address in the assembler's symbol table.
//   Returns 0 on success, -1 on error (duplicate or table full).
//
// asm_find_label:
//   Look up a label name and return its address via 'address'.
//   Returns 0 if found, -1 if not found.
//
int asm_parse_register(const char *str);
int asm_parse_number(const char *str, uint16_t *value);
int asm_add_label(Assembler *asm_ctx, const char *name, uint16_t address);
int asm_find_label(Assembler *asm_ctx, const char *name, uint16_t *address);

#endif // ASSEMBLER_H
