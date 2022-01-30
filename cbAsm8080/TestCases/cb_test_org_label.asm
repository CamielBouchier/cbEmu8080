; vim: syntax=asm ts=8 sw=8 sts=8 sr et columns=100 lines=45

;
; Intel 8080-8085 Assembly Language Programming 1977.
;
; Page 4.12 on ORG with a label. 
;

        NOP
        NOP
        NOP
LBL     ORG     0100H   ;LBL should have value 3
        NOP
        LXI     B,LBL
        NOP
