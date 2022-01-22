/***************************************************************************************************
*
* $BeginLicense$
* 
* $EndLicense$
*
***************************************************************************************************/

%{
// QtCore and QByteArray needed before bison.hpp
#include <QtCore>
#include <QByteArray>
#include "cb_asm_8080.bison.hpp"

extern int Strict;

char* SanitizeLabel(char* Label)
    {
    /* Uppercase, remove leading blanks and trailing ':' or blanks. */
    char *Tgt = Label;
    char *Src = Label;
    while (*Src)
        if (isspace(*Src)) Src++;
        else if (*Src == ':') Src++;
        else *Tgt++ = toupper(*Src++);
    *Tgt = 0;
    return Label;
    }
%}      

/**************************************************************************************************/

%option noyywrap
%option nounput
%option caseless

%s strict loose 

Comment             ;[^\n]*
Whitespace          [ \t]

    /* See ref manual page 2.3-2.4 */
AlphaNumeric        [0-9a-zA-Z?]                   
StrictLabel         ^{Whitespace}*({AlphaNumeric}|@){AlphaNumeric}{0,5}:
StrictName          ^{Whitespace}*({AlphaNumeric}|@){AlphaNumeric}{0,5}{Whitespace}
LooseLabel          ^({AlphaNumeric}|@){AlphaNumeric}*:?

    /* See ref manual page 2.5-2.6 */
HexInt              [0-9][0-9A-Fa-f]+H
DecInt              [0-9]+D?
OctInt              [0-7]+(O|Q)
BinInt              [0-1]+B
QuotedString        '([^']|''){2,}'
QuotedChar          '([^']|'')'

    /* See ref manual page 2.9 */
StrictIdentifier    ({AlphaNumeric}|@){AlphaNumeric}{0,5}
LooseIdentifier     ({AlphaNumeric}|@){AlphaNumeric}*

OpCode              NOP|RLC|RRC|RAL|RAR|DAA|CMA|STC|CMC|RNZ|RZ|RET|RNC|RC|RPO|XTHL|RPE|PCHL|XCHG|RP|DI|RM|SPHL|EI|HLT
OpCode_Reg          STAX|INX|INR|DCR|DAD|LDAX|DCX|ADD|ADC|SUB|SBB|ANA|XRA|ORA|CMP|POP|PUSH
OpCode_Reg_Exp_16   LXI
OpCode_Reg_Exp_8    MVI
OpCode_Reg_Reg      MOV
OpCode_Exp_16       SHLD|LHLD|STA|LDA|JNZ|JMP|CNZ|JZ|CZ|CALL|JNC|CNC|JC|CC|JPO|CPO|JPE|CPE|CP|JP|JM|CM
OpCode_Exp_8        ADI|ACI|OUT|SUI|IN|SBI|ANI|XRI|ORI|CPI|RST


/**************************************************************************************************/

%%
    if (Strict)
        BEGIN(strict);
    else
        BEGIN(loose);

{QuotedString}      {
                    /* Remove end quotes and replace '' (escaped quote) by ' */
                    char* Tgt = yytext;
                    char* Src = yytext+1;
                    while( *Src != '\'' or *(Src+1) == '\'')
                        {
                        *Tgt++ = *Src++;
                        if (*Src == '\'' and *(Src-1) == '\'') Src++;
                        }
                    *Tgt = 0;
                    yylval.String = strdup(yytext);
                    return TOKEN_QUOTED_STRING;
                    }

{QuotedChar}        {
                    yylval.String = (char*) malloc(2);
                    yylval.String[0] = yytext[1];
                    yylval.String[1] = 0;
                    return TOKEN_QUOTED_CHAR;
                    }

{Comment}           {
                    return TOKEN_COMMENT;
                    }

{HexInt}            {
                    yylval.Integer = strtol(yytext, NULL, 16);
                    return TOKEN_INTEGER;
                    }

{DecInt}            {
                    yylval.Integer = strtol(yytext, NULL, 10);
                    return TOKEN_INTEGER;
                    }

{OctInt}            {
                    yylval.Integer = strtol(yytext, NULL, 8);
                    return TOKEN_INTEGER;
                    }

{BinInt}            {
                    yylval.Integer = strtol(yytext, NULL, 2);
                    return TOKEN_INTEGER;
                    }

<strict>{StrictLabel} {
                    yylval.String = SanitizeLabel(strdup(yytext));
                    return TOKEN_LABEL;
                    }

<strict>{StrictName} {
                    yylval.String = SanitizeLabel(strdup(yytext));
                    return TOKEN_NAME;
                    }


<loose>{LooseLabel} {
                    yylval.String = SanitizeLabel(strdup(yytext));
                    return TOKEN_LABEL;
                    }

[+]                 { return TOKEN_PLUS;                }
[-]                 { return TOKEN_MINUS;               }
[*]                 { return TOKEN_MULT;                }
[/]                 { return TOKEN_DIV;                 }
[(]                 { return TOKEN_LEFT_PARENTHESIS;    }
[)]                 { return TOKEN_RIGHT_PARENTHESIS;   }
[,]                 { return TOKEN_COMMA;               }
[$]                 { return TOKEN_DOLLAR;              }
EQU                 { return TOKEN_EQU;                 }
SET                 { return TOKEN_SET;                 }
ORG                 { return TOKEN_ORG;                 }
DB                  { return TOKEN_DB;                  }
DW                  { return TOKEN_DW;                  }
DS                  { return TOKEN_DS;                  }
MOD                 { return TOKEN_MOD;                 }
SHR                 { return TOKEN_SHR;                 }
SHL                 { return TOKEN_SHL;                 }
NOT                 { return TOKEN_NOT;                 }
AND                 { return TOKEN_AND;                 }
OR                  { return TOKEN_OR;                  }
XOR                 { return TOKEN_XOR;                 }
EQ                  { return TOKEN_EQ;                  }
NE                  { return TOKEN_NE;                  }
LT                  { return TOKEN_LT;                  }
LE                  { return TOKEN_LE;                  }
GT                  { return TOKEN_GT;                  }
GE                  { return TOKEN_GE;                  }
HIGH                { return TOKEN_HIGH;                }
LOW                 { return TOKEN_LOW;                 }
END                 { return TOKEN_END;                 }

{OpCode}            { 
                    for (char* Ptr = yytext; *Ptr; Ptr++) *Ptr = toupper(*Ptr);
                    yylval.String = strdup(yytext);
                    return TOKEN_OPCODE; 
                    }

{OpCode_Reg}        { 
                    for (char* Ptr = yytext; *Ptr; Ptr++) *Ptr = toupper(*Ptr);
                    yylval.String = strdup(yytext);
                    return TOKEN_OPCODE_REG; 
                    }

{OpCode_Reg_Exp_8}  { 
                    for (char* Ptr = yytext; *Ptr; Ptr++) *Ptr = toupper(*Ptr);
                    yylval.String = strdup(yytext);
                    return TOKEN_OPCODE_REG_EXP_8; 
                    }

{OpCode_Reg_Exp_16} { 
                    for (char* Ptr = yytext; *Ptr; Ptr++) *Ptr = toupper(*Ptr);
                    yylval.String = strdup(yytext);
                    return TOKEN_OPCODE_REG_EXP_16; 
                    }

{OpCode_Reg_Reg}    { 
                    for (char* Ptr = yytext; *Ptr; Ptr++) *Ptr = toupper(*Ptr);
                    yylval.String = strdup(yytext);
                    return TOKEN_OPCODE_REG_REG; 
                    }

{OpCode_Exp_8}      { 
                    for (char* Ptr = yytext; *Ptr; Ptr++) *Ptr = toupper(*Ptr);
                    yylval.String = strdup(yytext);
                    return TOKEN_OPCODE_EXP_8; 
                    }

{OpCode_Exp_16}     { 
                    for (char* Ptr = yytext; *Ptr; Ptr++) *Ptr = toupper(*Ptr);
                    yylval.String = strdup(yytext);
                    return TOKEN_OPCODE_EXP_16; 
                    }

<strict>{StrictIdentifier} {
                    for (char* Ptr = yytext; *Ptr; Ptr++) *Ptr = toupper(*Ptr);
                    yylval.String = strdup(yytext);
                    return TOKEN_IDENTIFIER;
                    }

<loose>{LooseIdentifier} {
                    for (char* Ptr = yytext; *Ptr; Ptr++) *Ptr = toupper(*Ptr);
                    yylval.String = strdup(yytext);
                    return TOKEN_IDENTIFIER;
                    }

{Whitespace}*       { /* Whitespace skipping */ }

\n                  {
                    return TOKEN_EOL;
                    }

.                   {
                    return TOKEN_FLEX_ERROR;
                    }
%%

/**************************************************************************************************/

/* 
 * vim: syntax=lex ts=4 sw=4 sts=4 sr et columns=100 lines=45 
 */
