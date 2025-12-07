START:
    LOAD A, 0            ; A = counter starting at 0
    LOAD B, 10           ; B = stopping value (loop ends when A == 10)
    LOAD C, 1            ; C = increment step

LOOP:
    MOV D, A             ; Save current value of A in D (temp register)

    ADDI A, 48           ; Convert A to ASCII digit
    OUT 0xFF00, A        ; Print digit

    MOV A, D             ; Restore original numeric A

    LOAD D, 10           ; ASCII newline '\n'
    OUT 0xFF00, D        ; Print newline

    ADD A, C             ; A = A + 1
    CMP A, B             ; Compare A to 10
    JNZ LOOP             ; If A != 10, continue looping

    HLT                  ; End program
