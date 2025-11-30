;---------------------------------------------------------
; Timer Example Program
; Demonstrates the fetch/decode/execute cycle by counting
; from 0 up to 5, printing each number on its own line,
; then printing "Done!" and halting.
;
; Uses:
;   A = counter
;   B = target (stop value)
;   C = increment value
;   D = temporary register for printing
;---------------------------------------------------------

start:
    LOAD A, 0          ; A = 0 → initial counter value
    LOAD B, 5          ; B = 5 → stop when counter reaches 5
    LOAD C, 1          ; C = 1 → amount to increment each loop
    
loop:
    ;------------------ Print current counter ---------------------------
    MOV D, A           ; D = A → save current counter in D
    ADDI A, 48         ; A = A + '0' → convert counter to ASCII digit
    OUT 0xFF00, A      ; write ASCII digit to STDOUT port
    MOV A, D           ; A = D → restore counter value (undo ADDI)

    ; Print newline after the digit
    LOAD D, 10         ; D = 10 → ASCII '\n'
    OUT 0xFF00, D      ; print newline

    ;------------------ Increment counter -------------------------------
    ADD A, C           ; A = A + C → increment counter by 1

    ;------------------ Check loop condition ---------------------------
    CMP A, B           ; compare A (counter) with B (target)
    JNZ loop           ; if A != B, jump back to loop and continue

    ;------------------ Print "Done!" message --------------------------
    LOAD A, 68         ; 'D'
    OUT 0xFF00, A
    LOAD A, 111        ; 'o'
    OUT 0xFF00, A
    LOAD A, 110        ; 'n'
    OUT 0xFF00, A
    LOAD A, 101        ; 'e'
    OUT 0xFF00, A
    LOAD A, 33         ; '!'
    OUT 0xFF00, A
    LOAD A, 10         ; '\n'
    OUT 0xFF00, A

    HLT                ; halt CPU (program finished)
