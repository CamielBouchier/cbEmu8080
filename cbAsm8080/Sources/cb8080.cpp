//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "cb8080.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const OpCode OpCodes[0x100] = 
    {
    /* 0000 */ {  4, "NOP",             c_Addr_Implicit   },
    /* 0001 */ { 10, "LXI   B,%04XH",   c_Addr_Immediate2 },
    /* 0002 */ {  7, "STAX  B",         c_Addr_Implicit   },
    /* 0003 */ {  5, "INX   B",         c_Addr_Implicit   },
    /* 0004 */ {  5, "INR   B",         c_Addr_Implicit   },
    /* 0005 */ {  5, "DCR   B",         c_Addr_Implicit   },
    /* 0006 */ {  7, "MVI   B,%02XH",   c_Addr_Immediate1 },
    /* 0007 */ {  4, "RLC",             c_Addr_Implicit   },

    /* 0010 */ {  0, "???",             c_Addr_Implicit   },
    /* 0011 */ { 10, "DAD   B",         c_Addr_Implicit   },
    /* 0012 */ {  7, "LDAX  B",         c_Addr_Implicit   },
    /* 0013 */ {  5, "DCX   B",         c_Addr_Implicit   },
    /* 0014 */ {  5, "INR   C",         c_Addr_Implicit   },
    /* 0015 */ {  5, "DCR   C",         c_Addr_Implicit   },
    /* 0016 */ {  7, "MVI   C,%02XH",   c_Addr_Immediate1 },
    /* 0017 */ {  4, "RRC",             c_Addr_Implicit   },

    /* 0020 */ {  0, "???",             c_Addr_Implicit   },  
    /* 0021 */ { 10, "LXI   D,%04XH",   c_Addr_Immediate2 },
    /* 0022 */ {  7, "STAX  D",         c_Addr_Implicit   },
    /* 0023 */ {  5, "INX   D",         c_Addr_Implicit   },
    /* 0024 */ {  5, "INR   D",         c_Addr_Implicit   },
    /* 0025 */ {  5, "DCR   D",         c_Addr_Implicit   },
    /* 0026 */ {  7, "MVI   D,%02XH",   c_Addr_Immediate1 },
    /* 0027 */ {  4, "RAL",             c_Addr_Implicit   },

    /* 0030 */ {  0, "???",             c_Addr_Implicit   },
    /* 0031 */ { 10, "DAD   D",         c_Addr_Implicit   },
    /* 0032 */ {  7, "LDAX  D",         c_Addr_Implicit   },
    /* 0033 */ {  5, "DCX   D",         c_Addr_Implicit   },
    /* 0034 */ {  5, "INR   E",         c_Addr_Implicit   },
    /* 0035 */ {  5, "DCR   E",         c_Addr_Implicit   },
    /* 0036 */ {  7, "MVI   E,%02XH",   c_Addr_Immediate1 },
    /* 0037 */ {  4, "RAR",             c_Addr_Implicit   },

    /* 0040 */ {  0, "???",             c_Addr_Implicit   },
    /* 0041 */ { 10, "LXI   H,%04XH",   c_Addr_Immediate2 },
    /* 0042 */ { 16, "SHLD  %04XH",     c_Addr_Direct2    },
    /* 0043 */ {  5, "INX   H",         c_Addr_Implicit   },
    /* 0044 */ {  5, "INR   H",         c_Addr_Implicit   },
    /* 0045 */ {  5, "DCR   H",         c_Addr_Implicit   },
    /* 0046 */ {  7, "MVI   H,%02XH",   c_Addr_Immediate1 },
    /* 0047 */ {  4, "DAA",             c_Addr_Implicit   },

    /* 0050 */ {  0, "???",             c_Addr_Implicit   },
    /* 0051 */ { 10, "DAD   H",         c_Addr_Implicit   },
    /* 0052 */ { 16, "LHLD  %04XH",     c_Addr_Direct2    },
    /* 0053 */ {  5, "DCX   H",         c_Addr_Implicit   },
    /* 0054 */ {  5, "INR   L",         c_Addr_Implicit   },
    /* 0055 */ {  5, "DCR   L",         c_Addr_Implicit   },
    /* 0056 */ {  7, "MVI   L,%02XH",   c_Addr_Immediate1 },
    /* 0057 */ {  4, "CMA",             c_Addr_Implicit   },

    /* 0060 */ {  0, "???",             c_Addr_Implicit   },
    /* 0061 */ { 10, "LXI   SP,%04XH",  c_Addr_Immediate2 },
    /* 0062 */ { 13, "STA   %04XH",     c_Addr_Direct2    },
    /* 0063 */ {  5, "INX   SP",        c_Addr_Implicit   },
    /* 0064 */ { 10, "INR   M",         c_Addr_Implicit   },
    /* 0065 */ { 10, "DCR   M",         c_Addr_Implicit   },
    /* 0066 */ { 10, "MVI   M,%02XH",   c_Addr_Immediate1 },
    /* 0067 */ {  4, "STC",             c_Addr_Implicit   },

    /* 0070 */ {  0, "???",             c_Addr_Implicit   },
    /* 0071 */ { 10, "DAD   SP",        c_Addr_Implicit   },
    /* 0072 */ { 13, "LDA   %04XH",     c_Addr_Direct2    },
    /* 0073 */ {  5, "DCX   SP",        c_Addr_Implicit   },
    /* 0074 */ {  5, "INR   A",         c_Addr_Implicit   },
    /* 0075 */ {  5, "DCR   A",         c_Addr_Implicit   },
    /* 0076 */ {  7, "MVI   A,%02XH",   c_Addr_Immediate1 },
    /* 0077 */ {  4, "CMC",             c_Addr_Implicit   },

    /* 0100 */ {  5, "MOV   B,B",       c_Addr_Implicit   },
    /* 0101 */ {  5, "MOV   B,C",       c_Addr_Implicit   },
    /* 0102 */ {  5, "MOV   B,D",       c_Addr_Implicit   },
    /* 0103 */ {  5, "MOV   B,E",       c_Addr_Implicit   },
    /* 0104 */ {  5, "MOV   B,H",       c_Addr_Implicit   },
    /* 0105 */ {  5, "MOV   B,L",       c_Addr_Implicit   },
    /* 0106 */ {  7, "MOV   B,M",       c_Addr_Implicit   },
    /* 0107 */ {  5, "MOV   B,A",       c_Addr_Implicit   },

    /* 0110 */ {  5, "MOV   C,B",       c_Addr_Implicit   },
    /* 0111 */ {  5, "MOV   C,C",       c_Addr_Implicit   },
    /* 0112 */ {  5, "MOV   C,D",       c_Addr_Implicit   },
    /* 0113 */ {  5, "MOV   C,E",       c_Addr_Implicit   },
    /* 0114 */ {  5, "MOV   C,H",       c_Addr_Implicit   },
    /* 0115 */ {  5, "MOV   C,L",       c_Addr_Implicit   },
    /* 0116 */ {  7, "MOV   C,M",       c_Addr_Implicit   },
    /* 0117 */ {  5, "MOV   C,A",       c_Addr_Implicit   },

    /* 0120 */ {  5, "MOV   D,B",       c_Addr_Implicit   },
    /* 0121 */ {  5, "MOV   D,C",       c_Addr_Implicit   },
    /* 0122 */ {  5, "MOV   D,D",       c_Addr_Implicit   },
    /* 0123 */ {  5, "MOV   D,E",       c_Addr_Implicit   },
    /* 0124 */ {  5, "MOV   D,H",       c_Addr_Implicit   },
    /* 0125 */ {  5, "MOV   D,L",       c_Addr_Implicit   },
    /* 0126 */ {  7, "MOV   D,M",       c_Addr_Implicit   },
    /* 0127 */ {  5, "MOV   D,A",       c_Addr_Implicit   },

    /* 0130 */ {  5, "MOV   E,B",       c_Addr_Implicit   },
    /* 0131 */ {  5, "MOV   E,C",       c_Addr_Implicit   },
    /* 0132 */ {  5, "MOV   E,D",       c_Addr_Implicit   },
    /* 0133 */ {  5, "MOV   E,E",       c_Addr_Implicit   },
    /* 0134 */ {  5, "MOV   E,H",       c_Addr_Implicit   },
    /* 0135 */ {  5, "MOV   E,L",       c_Addr_Implicit   },
    /* 0136 */ {  7, "MOV   E,M",       c_Addr_Implicit   },
    /* 0137 */ {  5, "MOV   E,A",       c_Addr_Implicit   },

    /* 0140 */ {  5, "MOV   H,B",       c_Addr_Implicit   },
    /* 0141 */ {  5, "MOV   H,C",       c_Addr_Implicit   },
    /* 0142 */ {  5, "MOV   H,D",       c_Addr_Implicit   },
    /* 0143 */ {  5, "MOV   H,E",       c_Addr_Implicit   },
    /* 0144 */ {  5, "MOV   H,H",       c_Addr_Implicit   },
    /* 0145 */ {  5, "MOV   H,L",       c_Addr_Implicit   },
    /* 0146 */ {  7, "MOV   H,M",       c_Addr_Implicit   },
    /* 0147 */ {  5, "MOV   H,A",       c_Addr_Implicit   },

    /* 0150 */ {  5, "MOV   L,B",       c_Addr_Implicit   },
    /* 0151 */ {  5, "MOV   L,C",       c_Addr_Implicit   },
    /* 0152 */ {  5, "MOV   L,D",       c_Addr_Implicit   },
    /* 0153 */ {  5, "MOV   L,E",       c_Addr_Implicit   },
    /* 0154 */ {  5, "MOV   L,H",       c_Addr_Implicit   },
    /* 0155 */ {  5, "MOV   L,L",       c_Addr_Implicit   },
    /* 0156 */ {  7, "MOV   L,M",       c_Addr_Implicit   },
    /* 0157 */ {  5, "MOV   L,A",       c_Addr_Implicit   },

    /* 0160 */ {  7, "MOV   M,B",       c_Addr_Implicit   },
    /* 0161 */ {  7, "MOV   M,C",       c_Addr_Implicit   },
    /* 0162 */ {  7, "MOV   M,D",       c_Addr_Implicit   },
    /* 0163 */ {  7, "MOV   M,E",       c_Addr_Implicit   },
    /* 0164 */ {  7, "MOV   M,H",       c_Addr_Implicit   },
    /* 0165 */ {  7, "MOV   M,L",       c_Addr_Implicit   },
    /* 0166 */ {  7, "HLT",             c_Addr_Implicit   },
    /* 0167 */ {  7, "MOV   M,A",       c_Addr_Implicit   },

    /* 0170 */ {  5, "MOV   A,B",       c_Addr_Implicit   },
    /* 0171 */ {  5, "MOV   A,C",       c_Addr_Implicit   },
    /* 0172 */ {  5, "MOV   A,D",       c_Addr_Implicit   },
    /* 0173 */ {  5, "MOV   A,E",       c_Addr_Implicit   },
    /* 0174 */ {  5, "MOV   A,H",       c_Addr_Implicit   },
    /* 0175 */ {  5, "MOV   A,L",       c_Addr_Implicit   },
    /* 0176 */ {  7, "MOV   A,M",       c_Addr_Implicit   },
    /* 0177 */ {  5, "MOV   A,A",       c_Addr_Implicit   },

    /* 0200 */ {  4, "ADD   B",         c_Addr_Implicit   },
    /* 0201 */ {  4, "ADD   C",         c_Addr_Implicit   },
    /* 0202 */ {  4, "ADD   D",         c_Addr_Implicit   },
    /* 0203 */ {  4, "ADD   E",         c_Addr_Implicit   },
    /* 0204 */ {  4, "ADD   H",         c_Addr_Implicit   },
    /* 0205 */ {  4, "ADD   L",         c_Addr_Implicit   },
    /* 0206 */ {  7, "ADD   M",         c_Addr_Implicit   },
    /* 0207 */ {  4, "ADD   A",         c_Addr_Implicit   },

    /* 0210 */ {  4, "ADC   B",         c_Addr_Implicit   },
    /* 0211 */ {  4, "ADC   C",         c_Addr_Implicit   },
    /* 0212 */ {  4, "ADC   D",         c_Addr_Implicit   },
    /* 0213 */ {  4, "ADC   E",         c_Addr_Implicit   },
    /* 0214 */ {  4, "ADC   H",         c_Addr_Implicit   },
    /* 0215 */ {  4, "ADC   L",         c_Addr_Implicit   },
    /* 0216 */ {  7, "ADC   M",         c_Addr_Implicit   },
    /* 0217 */ {  4, "ADC   A",         c_Addr_Implicit   },

    /* 0220 */ {  4, "SUB   B",         c_Addr_Implicit   },
    /* 0221 */ {  4, "SUB   C",         c_Addr_Implicit   },
    /* 0222 */ {  4, "SUB   D",         c_Addr_Implicit   },
    /* 0223 */ {  4, "SUB   E",         c_Addr_Implicit   },
    /* 0224 */ {  4, "SUB   H",         c_Addr_Implicit   },
    /* 0225 */ {  4, "SUB   L",         c_Addr_Implicit   },
    /* 0226 */ {  7, "SUB   M",         c_Addr_Implicit   },
    /* 0227 */ {  4, "SUB   A",         c_Addr_Implicit   },

    /* 0230 */ {  4, "SBB   B",         c_Addr_Implicit   },
    /* 0231 */ {  4, "SBB   C",         c_Addr_Implicit   },
    /* 0232 */ {  4, "SBB   D",         c_Addr_Implicit   },
    /* 0233 */ {  4, "SBB   E",         c_Addr_Implicit   },
    /* 0234 */ {  4, "SBB   H",         c_Addr_Implicit   },
    /* 0235 */ {  4, "SBB   L",         c_Addr_Implicit   },
    /* 0236 */ {  7, "SBB   M",         c_Addr_Implicit   },
    /* 0237 */ {  4, "SBB   A",         c_Addr_Implicit   },

    /* 0240 */ {  4, "ANA   B",         c_Addr_Implicit   },
    /* 0241 */ {  4, "ANA   C",         c_Addr_Implicit   },
    /* 0242 */ {  4, "ANA   D",         c_Addr_Implicit   },
    /* 0243 */ {  4, "ANA   E",         c_Addr_Implicit   },
    /* 0244 */ {  4, "ANA   H",         c_Addr_Implicit   },
    /* 0245 */ {  4, "ANA   L",         c_Addr_Implicit   },
    /* 0246 */ {  7, "ANA   M",         c_Addr_Implicit   },
    /* 0247 */ {  4, "ANA   A",         c_Addr_Implicit   },

    /* 0250 */ {  4, "XRA   B",         c_Addr_Implicit   },
    /* 0251 */ {  4, "XRA   C",         c_Addr_Implicit   },
    /* 0252 */ {  4, "XRA   D",         c_Addr_Implicit   },
    /* 0253 */ {  4, "XRA   E",         c_Addr_Implicit   },
    /* 0254 */ {  4, "XRA   H",         c_Addr_Implicit   },
    /* 0255 */ {  4, "XRA   L",         c_Addr_Implicit   },
    /* 0256 */ {  7, "XRA   M",         c_Addr_Implicit   },
    /* 0257 */ {  4, "XRA   A",         c_Addr_Implicit   },

    /* 0260 */ {  4, "ORA   B",         c_Addr_Implicit   },
    /* 0261 */ {  4, "ORA   C",         c_Addr_Implicit   },
    /* 0262 */ {  4, "ORA   D",         c_Addr_Implicit   },
    /* 0263 */ {  4, "ORA   E",         c_Addr_Implicit   },
    /* 0264 */ {  4, "ORA   H",         c_Addr_Implicit   },
    /* 0265 */ {  4, "ORA   L",         c_Addr_Implicit   },
    /* 0266 */ {  7, "ORA   M",         c_Addr_Implicit   },
    /* 0267 */ {  4, "ORA   A",         c_Addr_Implicit   },

    /* 0270 */ {  4, "CMP   B",         c_Addr_Implicit   },
    /* 0271 */ {  4, "CMP   C",         c_Addr_Implicit   },
    /* 0272 */ {  4, "CMP   D",         c_Addr_Implicit   },
    /* 0273 */ {  4, "CMP   E",         c_Addr_Implicit   },
    /* 0274 */ {  4, "CMP   H",         c_Addr_Implicit   },
    /* 0275 */ {  4, "CMP   L",         c_Addr_Implicit   },
    /* 0276 */ {  7, "CMP   M",         c_Addr_Implicit   },
    /* 0277 */ {  4, "CMP   A",         c_Addr_Implicit   },

    /* 0300 */ {  5, "RNZ",             c_Addr_Implicit   },
    /* 0301 */ { 10, "POP   B",         c_Addr_Implicit   },
    /* 0302 */ { 10, "JNZ   %04XH",     c_Addr_Direct2    },
    /* 0303 */ { 10, "JMP   %04XH",     c_Addr_Direct2    },
    /* 0304 */ { 10, "CNZ   %04XH",     c_Addr_Direct2    }, 
    /* 0305 */ { 11, "PUSH  B",         c_Addr_Implicit   },
    /* 0306 */ {  7, "ADI   %02XH",     c_Addr_Immediate1 },
    /* 0307 */ { 11, "RST   0",         c_Addr_Implicit   },

    /* 0310 */ {  5, "RZ",              c_Addr_Implicit   },
    /* 0311 */ { 10, "RET",             c_Addr_Implicit   },
    /* 0312 */ { 10, "JZ    %04XH",     c_Addr_Direct2    },
    /* 0313 */ {  0, "???",             c_Addr_Implicit   },  
    /* 0314 */ { 10, "CZ    %04XH",     c_Addr_Direct2    },
    /* 0315 */ { 17, "CALL  %04XH",     c_Addr_Direct2    },
    /* 0316 */ {  7, "ACI   %02XH",     c_Addr_Immediate1 },
    /* 0317 */ { 11, "RST   1",         c_Addr_Implicit   },

    /* 0320 */ {  5, "RNC",             c_Addr_Implicit   },
    /* 0321 */ { 10, "POP   D",         c_Addr_Implicit   },
    /* 0322 */ { 10, "JNC   %04XH",     c_Addr_Direct2    },
    /* 0323 */ { 10, "OUT   %02XH",     c_Addr_Direct1    },
    /* 0324 */ { 10, "CNC   %04XH",     c_Addr_Direct2    },
    /* 0325 */ { 11, "PUSH  D",         c_Addr_Implicit   },
    /* 0326 */ {  7, "SUI   %02XH",     c_Addr_Immediate1 },
    /* 0327 */ { 11, "RST   2",         c_Addr_Implicit   },

    /* 0330 */ {  5, "RC",              c_Addr_Implicit   },
    /* 0331 */ {  0, "???",             c_Addr_Implicit   },
    /* 0332 */ { 10, "JC    %04XH",     c_Addr_Direct2    },
    /* 0333 */ { 10, "IN    %02XH",     c_Addr_Direct1    },
    /* 0334 */ { 10, "CC    %04XH",     c_Addr_Direct2    }, 
    /* 0335 */ {  0, "???",             c_Addr_Implicit   },
    /* 0336 */ {  7, "SBI   %02XH",     c_Addr_Immediate1 },
    /* 0337 */ { 11, "RST   3",         c_Addr_Implicit   },

    /* 0340 */ {  5, "RPO",             c_Addr_Implicit   },
    /* 0341 */ { 10, "POP   H",         c_Addr_Implicit   },
    /* 0342 */ { 10, "JPO   %04XH",     c_Addr_Direct2    },
    /* 0343 */ { 18, "XTHL",            c_Addr_Implicit   },
    /* 0344 */ { 10, "CPO   %04XH",     c_Addr_Direct2    },
    /* 0345 */ { 11, "PUSH  H",         c_Addr_Implicit   },
    /* 0346 */ {  7, "ANI   %02XH",     c_Addr_Immediate1 },
    /* 0347 */ { 11, "RST   4",         c_Addr_Implicit   },

    /* 0350 */ {  5, "RPE",             c_Addr_Implicit   },
    /* 0351 */ {  5, "PCHL",            c_Addr_Implicit   },
    /* 0352 */ { 10, "JPE   %04XH",     c_Addr_Direct2    },
    /* 0353 */ {  4, "XCHG",            c_Addr_Implicit   },
    /* 0354 */ { 10, "CPE   %04XH",     c_Addr_Direct2    },
    /* 0355 */ {  0, "???",             c_Addr_Implicit   },
    /* 0356 */ {  7, "XRI   %02XH",     c_Addr_Immediate1 },
    /* 0357 */ { 11, "RST   5",         c_Addr_Implicit   },

    /* 0360 */ {  5, "RP",              c_Addr_Implicit   },
    /* 0361 */ { 10, "POP   PSW",       c_Addr_Implicit   },
    /* 0362 */ { 10, "JP    %04XH",     c_Addr_Direct2    },
    /* 0363 */ {  4, "DI",              c_Addr_Implicit   },
    /* 0364 */ { 10, "CP    %04XH",     c_Addr_Direct2    },
    /* 0365 */ { 11, "PUSH  PSW",       c_Addr_Implicit   },
    /* 0366 */ {  7, "ORI   %02XH",     c_Addr_Immediate1 },
    /* 0367 */ { 11, "RST   6",         c_Addr_Implicit   },

    /* 0370 */ {  5, "RM",              c_Addr_Implicit   },
    /* 0371 */ {  5, "SPHL",            c_Addr_Implicit   },
    /* 0372 */ { 10, "JM    %04XH",     c_Addr_Direct2    },
    /* 0373 */ {  4, "EI",              c_Addr_Implicit   },
    /* 0374 */ { 10, "CM    %04XH",     c_Addr_Direct2    },
    /* 0375 */ {  0, "???",             c_Addr_Implicit   },
    /* 0376 */ {  7, "CPI   %02XH",     c_Addr_Immediate1 },
    /* 0377 */ { 11, "RST   7",         c_Addr_Implicit   }
    };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45
