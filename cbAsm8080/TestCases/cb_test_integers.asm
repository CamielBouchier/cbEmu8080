; vim: syntax=asm ts=8 sw=8 sts=8 sr et columns=100 lines=45

;
; Intel 8080-8085 Assembly Language Programming 1977.
;
; Page 2.5
;

HERE:   MVI C,0BAH
ABC:    MVI E,15
        MVI E,15D
LABEL:  MVI A,72Q
NOW:    MVI D,11110110B
JOS:    MVI A,72O

;
; Page 2.6
;

GO:     JMP $+6
        MVI E,'*'
DATE:   DB  'TODAY''S DATE'

