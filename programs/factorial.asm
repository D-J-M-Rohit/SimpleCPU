START:
    LOAD SP, 0xFEFF          ; Initialize stack pointer near top of RAM

    LOAD A, 51               ; '3' + 0? Actually ASCII 51 = '3'
    OUT 0xFF00, A            ; Print '3'
    LOAD A, 33               ; ASCII '!' = 33
    OUT 0xFF00, A            ; Print '!'
    LOAD A, 32               ; ASCII space
    OUT 0xFF00, A            ; Print space
    LOAD A, 61               ; ASCII '='
    OUT 0xFF00, A            ; Print '='
    LOAD A, 32               ; ASCII space
    OUT 0xFF00, A            ; Print space
    
    LOAD A, 3                ; Compute factorial(3)
    CALL FACTORIAL           ; Call recursive function

    ADDI A, 48               ; Convert numeric result to ASCII digit
    OUT 0xFF00, A            ; Print the result character
    LOAD A, 10               ; Newline
    OUT 0xFF00, A
    HLT                      ; Stop execution

; ---------------------------------------------------------
; FUNCTION: FACTORIAL
; Input:  A = n
; Output: A = n!
; Uses recursion. Saves B, C, D as callee-saved registers.
; ---------------------------------------------------------

FACTORIAL:
    PUSH B                   ; Save caller's B
    PUSH C                   ; Save caller's C
    PUSH D                   ; Save caller's D
    
    CMPI A, 1                ; If A == 1
    JZ BASE_CASE             ; return 1
    CMPI A, 0                ; If A == 0
    JZ BASE_CASE             ; return 1
    
    PUSH A                   ; Save current A (the n)
    SUBI A, 1                ; Prepare A = n - 1
    CALL FACTORIAL           ; Recursively compute (n-1)!
    
    POP B                    ; Restore original n (from PUSH A)
    MUL A, B                 ; A = (n-1)! * n
    
    POP D                    ; Restore saved registers
    POP C
    POP B
    RET                      ; Return to caller with A = n!
    
BASE_CASE:
    LOAD A, 1                ; factorial(0) = factorial(1) = 1
    POP D                    ; Restore saved registers
    POP C
    POP B
    RET                      ; Return to caller

