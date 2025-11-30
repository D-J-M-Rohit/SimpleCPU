;---------------------------------------------------------
; Number Printer 0–999
; Prints decimal numbers from 0 to 999, each on its own line.
; After finishing, prints "Done!\n" and halts.
;
; Uses:
;   A = current number
;   B = working copy of A for digit extraction
;   C = digit count (how many digits were pushed)
;   D = temp (for divisions and ASCII digits)
;   Stack = holds digits while we print them in correct order.
;---------------------------------------------------------

START:
    LOAD A, 0          ; A = 0 → starting number to print

LOOP:
    MOV B, A           ; B = current number (we'll destroy B during digit extraction)
    LOAD C, 0          ; C = 0 → digit count = 0

;---------------------------------------------------------
; Convert number in B into decimal digits on the stack.
; Algorithm:
;   while (true):
;       D = 10
;       DIV B, D     → B = B / 10 (quotient), D = old_B % 10 (remainder)
;       D += '0'     → convert remainder to ASCII digit
;       push D
;       C++
;       if B == 0: break
;---------------------------------------------------------

DIGIT_EXTRACT:
    LOAD D, 10         ; D = 10 (divisor for decimal)
    DIV B, D           ; B = B / 10 (quotient), D = remainder
    ADDI D, 48         ; D = D + '0' → convert remainder to ASCII digit
    PUSH D             ; push this ASCII digit onto stack
    ADDI C, 1          ; C = C + 1 → increment digit count
    CMPI B, 0          ; have we consumed all digits (quotient == 0)?
    JNZ DIGIT_EXTRACT  ; if B != 0, keep extracting digits

; At this point:
;   - Stack has all digits in reverse order (least significant at top)
;   - C tells us how many digits we pushed

;---------------------------------------------------------
; Pop digits and print them to STDOUT in correct order.
; Popping reverses the order, giving us the proper left-to-right number.
;---------------------------------------------------------

PRINT_DIGITS:
    POP D              ; get next digit from stack into D
    OUT 0xFF00, D      ; print digit character
    SUBI C, 1          ; C = C - 1 → one less digit to print
    CMPI C, 0          ; any digits left?
    JNZ PRINT_DIGITS   ; if C != 0, keep printing

    ; After all digits printed, output newline

    LOAD D, 10         ; D = ASCII '\n' (10)
    OUT 0xFF00, D      ; print newline

;---------------------------------------------------------
; Increment A and repeat until A == 1000
;---------------------------------------------------------

    ADDI A, 1          ; A = A + 1 → next number
    CMPI A, 1000       ; reached 1000 yet?
    JNZ LOOP           ; if A != 1000, go print next number

;---------------------------------------------------------
; After printing 0..999, print "Done!\n" and halt.
;---------------------------------------------------------

DONE:
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
    HLT                ; halt CPU
