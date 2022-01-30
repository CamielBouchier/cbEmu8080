; vim: syntax=asm ts=8 sw=8 sts=8 sr et columns=100 lines=45

;
; Intel 8080-8085 Assembly Language Programming 1977.
;
; Page 2.3-2.4 on labels
;

FOO:    NOP
BAR:    JMP XFOO        ;should be known as label !
NOP
?FO1:   NOP
?FO2:
?F?O3:  NOP
@FO4:
FO5:NOP
XFOO:   NOP

; Next ones should fail
;TOOLONG:NOP
;NOCOL1  NOP
;_FO6    NOP
;NOCOL   NOP
