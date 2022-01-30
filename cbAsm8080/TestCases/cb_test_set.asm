; vim: syntax=asm ts=8 sw=8 sts=8 sr et columns=100 lines=45

;
; Intel 8080-8085 Assembly Language Programming 1977.
;
; Page 4.3 on SET Directive
;

IMMED   SET     5
        ADI     IMMED   ; C605
IMMED   SET     10H-6
        ADI     IMMED   ; C60A
