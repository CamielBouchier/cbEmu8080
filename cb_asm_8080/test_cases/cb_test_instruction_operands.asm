; vim: syntax=asm ts=8 sw=8 sts=8 sr et columns=100 lines=45

;
; Intel 8080-8085 Assembly Language Programming 1977.
;
; Page 2.7
;

        DB      5
X:      DB      4
        DB      3
INS:    DB      (ADD C)
FOO:    DB      (HLT)
BAR:    DB      (MOV B,C)
FIVE    EQU     5
        DB      (FIVE+1)
