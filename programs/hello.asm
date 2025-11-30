;---------------------------------------------------------
; Hello, World! Program
; Prints "Hello, World!" followed by a newline to STDOUT
; using the memory-mapped I/O port 0xFF00.
;---------------------------------------------------------

start:
    LOAD A, 72         ; A = ASCII 'H'  (72)
    OUT 0xFF00, A      ; print 'H'
    
    LOAD A, 101        ; A = ASCII 'e'  (101)
    OUT 0xFF00, A      ; print 'e'
    
    LOAD A, 108        ; A = ASCII 'l'  (108)
    OUT 0xFF00, A      ; print 'l'
    
    LOAD A, 108        ; A = ASCII 'l'
    OUT 0xFF00, A      ; print 'l'
    
    LOAD A, 111        ; A = ASCII 'o'  (111)
    OUT 0xFF00, A      ; print 'o'
    
    LOAD A, 44         ; A = ASCII ','  (44)
    OUT 0xFF00, A      ; print ','
    
    LOAD A, 32         ; A = ASCII ' '  (space, 32)
    OUT 0xFF00, A      ; print space
    
    LOAD A, 87         ; A = ASCII 'W'  (87)
    OUT 0xFF00, A      ; print 'W'
    
    LOAD A, 111        ; A = ASCII 'o'
    OUT 0xFF00, A      ; print 'o'
    
    LOAD A, 114        ; A = ASCII 'r'  (114)
    OUT 0xFF00, A      ; print 'r'
    
    LOAD A, 108        ; A = ASCII 'l'
    OUT 0xFF00, A      ; print 'l'
    
    LOAD A, 100        ; A = ASCII 'd'  (100)
    OUT 0xFF00, A      ; print 'd'
    
    LOAD A, 33         ; A = ASCII '!'  (33)
    OUT 0xFF00, A      ; print '!'
    
    LOAD A, 10         ; A = ASCII '\n' (newline, 10)
    OUT 0xFF00, A      ; print newline
    
    HLT                ; halt CPU (program finished)

;---------------------------------------------------------
; Alternative, loop-based version (conceptual)
; This version is more efficient: it stores the string
; in memory and walks over it with a pointer and length.
; (Requires placing the bytes of "Hello, World!\n" in memory.)
;---------------------------------------------------------
;
; start:
;     LOAD B, msg_addr   ; B = address of first character
;     LOAD C, 13         ; C = message length (13 chars)
;     
; print_loop:
;     LOAD A, [B]        ; A = memory[B] (current character)
;     OUT 0xFF00, A      ; print character
;     ADDI B, 1          ; B = B + 1 (advance pointer)
;     DEC C              ; C = C - 1 (one less char to print)
;     CMPI C, 0          ; done?
;     JNZ print_loop     ; if C != 0, keep printing
;     
;     HLT                ; done
;
; msg_addr:
;     ; Here you would place the bytes for: "Hello, World!\n"
;     ; e.g. in a data section, once your assembler/loader
;     ; supports mixing code and data properly.
