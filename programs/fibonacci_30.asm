;---------------------------------------------------------
; Fibonacci printer (30 terms)
; Uses:
;   A = F(n-1)
;   B = F(n)
;   C = remaining count
;   D = digit counter / temp
; Prints each Fibonacci number in decimal followed by newline.
;---------------------------------------------------------

START:
    LOAD A, 0          ; A = F(n-1) = 0 (first previous term)
    LOAD B, 1          ; B = F(n)   = 1 (current term)
    LOAD C, 30         ; C = how many Fibonacci numbers to print

FIB_LOOP:
    PUSH A             ; Save F(n-1) on stack (we’ll need it later)
    PUSH B             ; Save F(n)   on stack while we print it

    MOV A, B           ; A = number to print (current Fibonacci)
    LOAD D, 0          ; D = digit count = 0 (how many digits we pushed)

;---------------------------------------------------------
; Convert number in A to decimal digits on the stack.
; We repeatedly divide by 10:
;   - quotient stays in A
;   - remainder (digit) is in B
; We turn the remainder into ASCII and push it.
; When A becomes 0, we’ve extracted all digits.
;---------------------------------------------------------

DIGIT_EXTRACT:
    LOAD B, 10         ; B = 10 (divisor for decimal)
    DIV A, B           ; A = A / 10 (quotient), B = A % 10 (remainder)
    ADDI B, 48         ; B = B + '0' → convert remainder to ASCII digit
    PUSH B             ; Push this ASCII digit on stack (least significant first)
    ADDI D, 1          ; D = D + 1 → increment digit count
    CMPI A, 0          ; Have we consumed all digits? (is quotient 0?)
    JNZ DIGIT_EXTRACT  ; If A != 0, keep extracting digits

; At this point:
;   - Stack has digits in reverse order (LSD at top)
;   - D holds how many digits we pushed

;---------------------------------------------------------
; Print all digits by popping them back off the stack
; This reverses them, giving us correct left-to-right order.
;---------------------------------------------------------

PRINT_DIGITS:
    POP B              ; Pop top digit into B
    OUT 0xFF00, B      ; Print digit character to STDOUT
    SUBI D, 1          ; D = D - 1 → one less digit left
    CMPI D, 0          ; Any digits left?
    JNZ PRINT_DIGITS   ; If D != 0, keep printing

    LOAD B, 10         ; B = 10 = '\n'
    OUT 0xFF00, B      ; Print newline after the number

;---------------------------------------------------------
; Restore original Fibonacci terms and compute next term:
;   next = F(n+1) = F(n-1) + F(n)
; Then shift:
;   F(n-1) <- F(n)
;   F(n)   <- next
;---------------------------------------------------------

    POP B              ; Restore B = F(n)   from stack
    POP A              ; Restore A = F(n-1) from stack

    PUSH A             ; Save F(n-1) again so we can reuse A safely
    ADD A, B           ; A = A + B = F(n-1) + F(n) = next term
    MOV D, A           ; D = next term (F(n+1))
    POP A              ; Restore A = F(n-1) (previous term)

    MOV A, B           ; A = F(n)   → shift previous: A now F(n)
    MOV B, D           ; B = F(n+1) → current term becomes next

;---------------------------------------------------------
; Decrement counter and loop until we’ve printed 30 terms
;---------------------------------------------------------

    DEC C              ; C = C - 1 → one term printed
    CMPI C, 0          ; Are we done?
    JNZ FIB_LOOP       ; If C != 0, print next Fibonacci number

DONE:
    HLT                ; Halt CPU - all terms printed
