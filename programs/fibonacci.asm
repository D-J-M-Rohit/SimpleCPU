;---------------------------------------------------------
; Simple Fibonacci printer (10 terms, up to 2 digits)
; NOTE:
;   - Prints two characters for each number:
;       * tens digit (or '0' if < 10)
;       * ones digit
;   - Then prints a newline.
;   - Uses:
;       A = F(n-1)
;       B = F(n)
;       C = remaining count
;       D = temp (used for division / ASCII)
;---------------------------------------------------------

START:
    LOAD A, 0          ; A = F(n-1) = 0
    LOAD B, 1          ; B = F(n)   = 1
    LOAD C, 10         ; C = how many Fibonacci numbers to print

FIB_LOOP:
    PUSH A             ; Save F(n-1) on stack (we’ll need it later)

    MOV A, B           ; A = current Fibonacci number (F(n))
    LOAD D, 10         ; D = 10 (we'll use this as divisor)

    ;------------------ Print the number as two digits ------------------
    ; After DIV A, D:
    ;   A = A / 10   → tens digit
    ;   D = A % 10   → ones digit
    ; We then convert both to ASCII and print them.
    ; For numbers < 10, A becomes 0, so you get '0X' where X is ones digit.

    DIV A, D           ; A = A / 10 (tens), D = remainder (ones)
    ADDI A, 48         ; Convert tens digit to ASCII: '0' + tens
    OUT 0xFF00, A      ; Print tens digit

    ADDI D, 48         ; Convert ones digit to ASCII: '0' + ones
    OUT 0xFF00, D      ; Print ones digit

    LOAD A, 10         ; A = 10 = ASCII newline '\n'
    OUT 0xFF00, A      ; Print newline after the number

    POP A              ; Restore A = F(n-1) from stack

    ;------------------ Compute next Fibonacci term ---------------------
    ; next = F(n-1) + F(n)
    ; Then:
    ;   F(n-1) <- F(n)
    ;   F(n)   <- next

    PUSH A             ; Save F(n-1) again so we can reuse A
    ADD A, B           ; A = A + B = F(n-1) + F(n) = next
    MOV D, A           ; D = next term
    POP A              ; Restore A = F(n-1)

    MOV A, B           ; A = F(n)   → shift previous
    MOV B, D           ; B = next   → new current term

    ;------------------ Loop control ------------------------------------

    DEC C              ; C = C - 1 → one term printed
    CMPI C, 0          ; Are we done printing all terms?
    JNZ FIB_LOOP       ; If C != 0, continue with next Fibonacci number

DONE:
    HLT                ; Halt CPU
