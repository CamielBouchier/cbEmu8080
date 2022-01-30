; vim: syntax=asm ts=8 sw=8 sts=8 sr et columns=100 lines=45

;
; Intel 8080-8085 Assembly Language Programming 1977.
;
; Page 2.12-2.14
;

; All should generate 65D/41H
        MVI B,      5+30*2
        MVI B,      (25/5)+30*2
        MVI B,      5+(-30*-2)
        MVI B,      (25 MOD 8) + 64
        DB       5+30*2
        DB       (25/5)+30*2
        DB       5+(-30*-2)
        DB       (25 MOD 8) + 64
NUMBR   EQU     00010101B
        DB      NUMBR SHR 2 ; Should be 5
        DB      NUMBR SHL 1 ; Should be 2AH
NEG     EQU     10010101B
        DB      NEG SHR 2   ; Should be 25H
NUM1    EQU     00010001B
NUM2    EQU     00010000B
        DB      NOT NUM1
        DB      NUM1 AND NUM2
        DB      NUM1 AND NUM1
        DB      NUM1 OR NUM2
        DB      NUM2 OR NUM2
        DB      NUM1 XOR NUM2
        DB      NUM1 XOR NUM1
        DB      5 EQ 5
        DB      5 NE 5
        DB      5 GT 4
        DB      4 GE 5
        DB      4 LT 5
        DB      4 LE 5
        DB      HIGH 5643H
        DB      LOW  5643H
        
