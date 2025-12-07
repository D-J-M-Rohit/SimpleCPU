START:
    LOAD A, 0            ; A = first Fibonacci number (F0 = 0)
    LOAD B, 1            ; B = second Fibonacci number (F1 = 1)
    LOAD C, 10           ; C = loop counter (print 10 Fibonacci numbers)

LOOP:
    PUSH A               ; Save A on stack (because A will be modified)
    PUSH B               ; Save B on stack

    MOV A, B             ; A = current Fibonacci number to print (B)
    LOAD D, 10           ; D = 10 used to compute tens digit

    DIV A, D             ; A = A / 10  → tens digit
    ADDI A, 48           ; Convert tens digit to ASCII
    OUT 0xFF00, A        ; Print tens digit

    ADDI D, 48           ; Convert ones digit (stored in D?) to ASCII
    OUT 0xFF00, D        ; Print ones digit

    LOAD A, 10           ; Newline
    OUT 0xFF00, A

    POP B                ; Restore original B
    POP A                ; Restore original A

    PUSH A               ; Save A temporarily
    ADD A, B             ; A = A + B  → next Fibonacci number
    MOV D, A             ; D = next Fibonacci number
    POP A                ; Restore A

    MOV A, B             ; Shift Fibonacci forward: A = old B
    MOV B, D             ; B = next Fibonacci number

    DEC C                ; Decrement loop counter
    CMPI C, 0            ; Compare C to zero
    JNZ LOOP             ; Repeat if not done

DONE:
    HLT                  ; End program
