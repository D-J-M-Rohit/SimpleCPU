START:
    LOAD A, 72           ; ASCII 'H'
    OUT 0xFF00, A        ; print 'H'

    LOAD A, 101          ; ASCII 'e'
    OUT 0xFF00, A        ; print 'e'

    LOAD A, 108          ; ASCII 'l'
    OUT 0xFF00, A        ; print 'l'

    LOAD A, 108          ; ASCII 'l'
    OUT 0xFF00, A        ; print 'l'

    LOAD A, 111          ; ASCII 'o'
    OUT 0xFF00, A        ; print 'o'

    LOAD A, 44           ; ASCII ','
    OUT 0xFF00, A        ; print ','

    LOAD A, 32           ; ASCII space
    OUT 0xFF00, A        ; print space

    LOAD A, 87           ; ASCII 'W'
    OUT 0xFF00, A        ; print 'W'

    LOAD A, 111          ; ASCII 'o'
    OUT 0xFF00, A        ; print 'o'

    LOAD A, 114          ; ASCII 'r'
    OUT 0xFF00, A        ; print 'r'

    LOAD A, 108          ; ASCII 'l'
    OUT 0xFF00, A        ; print 'l'

    LOAD A, 100          ; ASCII 'd'
    OUT 0xFF00, A        ; print 'd'

    LOAD A, 33           ; ASCII '!'
    OUT 0xFF00, A        ; print '!'

    LOAD A, 10           ; newline character '\n'
    OUT 0xFF00, A        ; print newline

    HLT                  ; halt program
