/***************************************************************************************************
*
* $BeginLicense$
*
* $EndLicense$
*
***************************************************************************************************/

%{
#include <QtCore>
#include <QByteArray>

#include "cb_Unit.h"
#include "cb_Line.h"

extern int yylex();
extern int yyerror(const char* msg);

extern cb_Unit Unit;
%}

/**************************************************************************************************/

%union
    {
    char*             String;
    qint32            Integer;
    struct cb_exp_node* IntExpression;
    struct 
        {
        struct cb_exp_node* Expression;
        int Count;
        }             ByteWordExpression;
    }

%token TOKEN_FLEX_ERROR
%token TOKEN_EOL
%token TOKEN_COMMENT
%token TOKEN_COMMA
%token TOKEN_DOLLAR

%token TOKEN_LEFT_PARENTHESIS
%token TOKEN_RIGHT_PARENTHESIS

%token TOKEN_PLUS
%token TOKEN_MINUS
%token TOKEN_MULT
%token TOKEN_DIV
%token TOKEN_MOD
%token TOKEN_SHR
%token TOKEN_SHL
%token TOKEN_NOT
%token TOKEN_AND
%token TOKEN_OR
%token TOKEN_XOR
%token TOKEN_EQ
%token TOKEN_NE
%token TOKEN_LT
%token TOKEN_LE
%token TOKEN_GT
%token TOKEN_GE
%token TOKEN_HIGH
%token TOKEN_LOW

%token TOKEN_EQU
%token TOKEN_SET
%token TOKEN_ORG
%token TOKEN_DB
%token TOKEN_DW
%token TOKEN_DS
%token TOKEN_END

%token <String>  TOKEN_OPCODE
%token <String>  TOKEN_OPCODE_REG
%token <String>  TOKEN_OPCODE_REG_REG
%token <String>  TOKEN_OPCODE_REG_EXP_8
%token <String>  TOKEN_OPCODE_REG_EXP_16
%token <String>  TOKEN_OPCODE_EXP_8
%token <String>  TOKEN_OPCODE_EXP_16

%token <String>  TOKEN_LABEL
%token <String>  TOKEN_NAME 
%token <String>  TOKEN_IDENTIFIER
%token <String>  TOKEN_QUOTED_STRING
%token <String>  TOKEN_QUOTED_CHAR

%token <Integer> TOKEN_INTEGER

%type <IntExpression>      expression_int
%type <ByteWordExpression> expression_bytes
%type <ByteWordExpression> expression_words

/* Lowest precedence to highest */
%left TOKEN_COMMA
%left TOKEN_OR TOKEN_XOR
%left TOKEN_AND
%left TOKEN_NOT
%left TOKEN_EQ TOKEN_NE TOKEN_LT TOKEN_LE TOKEN_GT TOKEN_GE
%left TOKEN_PLUS TOKEN_MINUS 
%left TOKEN_MULT TOKEN_DIV TOKEN_MOD TOKEN_SHR TOKEN_SHL
%left TOKEN_HIGH TOKEN_LOW

/**************************************************************************************************/

%%

line            : commentline
                | statementline

commentline     : TOKEN_COMMENT TOKEN_EOL
                | TOKEN_EOL

statementline   : statement TOKEN_EOL
                | statement TOKEN_COMMENT TOKEN_EOL

statement       : TOKEN_NAME TOKEN_EQU expression_int       /* StrictLabel = 1 */
                    { 
                    Unit.m_Line->m_Type = TOKEN_EQU; 
                    Unit.m_Line->m_Label = $1;
                    Unit.m_Line->m_Expression = $3;
                    }
                | TOKEN_LABEL TOKEN_EQU expression_int      /* StrictLabel = 0 */
                    { 
                    Unit.m_Line->m_Type = TOKEN_EQU; 
                    Unit.m_Line->m_Label = $1;
                    Unit.m_Line->m_Expression = $3;
                    }
                | TOKEN_NAME TOKEN_SET expression_int       /* StrictLabel = 1 */
                    { 
                    Unit.m_Line->m_Type = TOKEN_SET; 
                    Unit.m_Line->m_Label = $1;
                    Unit.m_Line->m_Expression = $3;
                    }
                | TOKEN_LABEL TOKEN_SET expression_int      /* StrictLabel = 0 */
                    { 
                    Unit.m_Line->m_Type = TOKEN_SET; 
                    Unit.m_Line->m_Label = $1;
                    Unit.m_Line->m_Expression = $3;
                    }
                | TOKEN_LABEL unlabeled
                    {
                    Unit.m_Line->m_Label = $1;
                    }
                | unlabeled
                | TOKEN_LABEL
                    {
                    Unit.m_Line->m_Label = $1;
                    }

unlabeled       : TOKEN_ORG expression_int
                    { 
                    Unit.m_Line->m_Type = TOKEN_ORG; 
                    Unit.m_Line->m_Expression = $2;
                    }
                | TOKEN_DS expression_int
                    { 
                    Unit.m_Line->m_Type = TOKEN_DS; 
                    Unit.m_Line->m_Expression = $2;
                    }
                | TOKEN_DB expression_bytes
                    {
                    Unit.m_Line->m_Type = TOKEN_DB;
                    Unit.m_Line->m_Expression = $2.Expression;
                    Unit.m_Line->m_Bytes.resize($2.Count);
                    }
                | TOKEN_DW expression_words
                    {
                    Unit.m_Line->m_Type = TOKEN_DW;
                    Unit.m_Line->m_Expression = $2.Expression;
                    Unit.m_Line->m_Bytes.resize($2.Count);
                    }
                | TOKEN_OPCODE
                    { 
                    Unit.m_Line->m_Type = TOKEN_OPCODE; 
                    Unit.m_Line->SetOpCode($1);
                    }
                | TOKEN_OPCODE_REG TOKEN_IDENTIFIER
                    { 
                    Unit.m_Line->m_Type = TOKEN_OPCODE_REG; 
                    Unit.m_Line->SetOpCode($1);
                    Unit.m_Line->m_Register = $2;
                    }
                | TOKEN_OPCODE_REG_REG TOKEN_IDENTIFIER TOKEN_COMMA TOKEN_IDENTIFIER
                    { 
                    Unit.m_Line->m_Type = TOKEN_OPCODE_REG_REG; 
                    Unit.m_Line->SetOpCode($1);
                    Unit.m_Line->m_Register = $2;
                    Unit.m_Line->m_Register2 = $4;
                    }
                | TOKEN_OPCODE_REG_EXP_8 TOKEN_IDENTIFIER TOKEN_COMMA expression_int
                    { 
                    Unit.m_Line->m_Type = TOKEN_OPCODE_REG_EXP_8; 
                    Unit.m_Line->SetOpCode($1);
                    Unit.m_Line->m_Register = $2;
                    Unit.m_Line->m_Expression = $4;
                    }
                | TOKEN_OPCODE_REG_EXP_16 TOKEN_IDENTIFIER TOKEN_COMMA expression_int
                    { 
                    Unit.m_Line->m_Type = TOKEN_OPCODE_REG_EXP_16; 
                    Unit.m_Line->SetOpCode($1);
                    Unit.m_Line->m_Register = $2;
                    Unit.m_Line->m_Expression = $4;
                    }
                | TOKEN_OPCODE_EXP_8 expression_int
                    { 
                    Unit.m_Line->m_Type = TOKEN_OPCODE_EXP_8; 
                    Unit.m_Line->SetOpCode($1);
                    Unit.m_Line->m_Expression = $2;
                    }
                | TOKEN_OPCODE_EXP_16 expression_int
                    { 
                    Unit.m_Line->m_Type = TOKEN_OPCODE_EXP_16; 
                    Unit.m_Line->SetOpCode($1);
                    Unit.m_Line->m_Expression = $2;
                    }
                | TOKEN_END

expression_int  : TOKEN_INTEGER
                    {
                    $$ = new cb_exp_node(cb_exp_node::Type_Integer, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Value = (int) $1;
                    }
                | TOKEN_DOLLAR
                    {
                    $$ = new cb_exp_node(cb_exp_node::Type_Integer, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Value = (int) Unit.m_Line->m_Org;
                    }
                | TOKEN_IDENTIFIER
                    {
                    $$ = new cb_exp_node(cb_exp_node::Type_Identifier, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Value = (QByteArray) $1;
                    }
                | TOKEN_QUOTED_CHAR
                    {
                    $$ = new cb_exp_node(cb_exp_node::Type_String, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Value = (QByteArray) $1;
                    }
                | expression_int TOKEN_PLUS expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_Plus, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | expression_int TOKEN_MINUS expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_Minus, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | expression_int TOKEN_MULT expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_Mult, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | expression_int TOKEN_DIV expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_Div, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | expression_int TOKEN_MOD expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_Mod, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | expression_int TOKEN_SHR expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_SHR, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | expression_int TOKEN_SHL expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_SHL, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | TOKEN_NOT expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_NOT, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = NULL;
                    $$->m_Right = $2;
                    }
                | expression_int TOKEN_AND expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_AND, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | expression_int TOKEN_OR expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_OR, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | expression_int TOKEN_XOR expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_XOR, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | expression_int TOKEN_EQ expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_EQ, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | expression_int TOKEN_NE expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_NE, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | expression_int TOKEN_LT expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_LT, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | expression_int TOKEN_LE expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_LE, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | expression_int TOKEN_GT expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_GT, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | expression_int TOKEN_GE expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_GE, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = $1;
                    $$->m_Right = $3;
                    }
                | TOKEN_LEFT_PARENTHESIS expression_int TOKEN_RIGHT_PARENTHESIS
                    { 
                    $$ = $2; 
                    }
                | TOKEN_MINUS expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_UnaryMinus, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = NULL;
                    $$->m_Right = $2;
                    }
                | TOKEN_HIGH expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_HIGH, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = NULL;
                    $$->m_Right = $2;
                    }
                | TOKEN_LOW  expression_int
                    { 
                    $$ = new cb_exp_node(cb_exp_node::Type_Op_LOW, 
                                       cb_exp_node::Target_Integer);
                    $$->m_Left  = NULL;
                    $$->m_Right = $2;
                    }

expression_bytes: expression_int
                    {
                    $$.Expression = new cb_exp_node(cb_exp_node::Type_Expression, 
                                                  cb_exp_node::Target_Bytes);
                    $$.Expression->m_Left = $1;
                    $$.Expression->m_Right = NULL;
                    $$.Count = 1;
                    }
                | TOKEN_QUOTED_STRING
                    {
                    $$.Expression = new cb_exp_node(cb_exp_node::Type_String, 
                                                  cb_exp_node::Target_Bytes);
                    $$.Expression->m_Value = (QByteArray) $1;
                    $$.Count = strlen($1);
                    }
                | expression_bytes TOKEN_COMMA expression_bytes
                    { 
                    $$.Expression = new cb_exp_node(cb_exp_node::Type_Op_Concat, 
                                                  cb_exp_node::Target_Bytes);
                    $$.Expression->m_Left  = $1.Expression;
                    $$.Expression->m_Right = $3.Expression;
                    $$.Count = $1.Count + $3.Count;
                    }
                | TOKEN_LEFT_PARENTHESIS expression_bytes TOKEN_RIGHT_PARENTHESIS
                    { 
                    $$ = $2; 
                    }
                | TOKEN_LEFT_PARENTHESIS unlabeled TOKEN_RIGHT_PARENTHESIS
                    { 
                    $$.Expression = new cb_exp_node(cb_exp_node::Type_Instruction, 
                                                  cb_exp_node::Target_Bytes);
                    $$.Count = 1;
                    // The parsing of opcodes with registers did change m_Org.
                    // That happened in the SetOpCode function, as we parsed opcodes indeed.
                    // Here we correct for that by resetting to the original one.
                    Unit.m_CurrentOrg = Unit.m_Line->m_OriginalOrg;
                    }

expression_words: expression_int
                    {
                    $$.Expression = new cb_exp_node(cb_exp_node::Type_Expression, 
                                                  cb_exp_node::Target_Words);
                    $$.Expression->m_Left = $1;
                    $$.Expression->m_Right = NULL;
                    $$.Count = 2;
                    }
                | TOKEN_QUOTED_STRING
                    {
                    $$.Expression = new cb_exp_node(cb_exp_node::Type_String, 
                                                  cb_exp_node::Target_Words);
                    // Give same interpretation as asm8080. Not sure if it make sense
                    // certainly not for strings more than 2 chars long.
                    int BL = strlen($1);
                    QByteArray V = (QByteArray) $1;
                    V.resize(BL);
                    for (int i=0; i<BL/2; i++)
                        {
                        char T = V.at(2*i);
                        V[2*i] = V.at(2*i+1);
                        V[2*i+1] = T;
                        }
                    if (BL%2) 
                        {
                        BL++;
                        V.resize(BL);
                        V[BL-1]=0;
                        }
                    $$.Expression->m_Value = V;
                    $$.Count = BL;
                    }
                | expression_words TOKEN_COMMA expression_words
                    { 
                    $$.Expression = new cb_exp_node(cb_exp_node::Type_Op_Concat, 
                                                  cb_exp_node::Target_Words);
                    $$.Expression->m_Left  = $1.Expression;
                    $$.Expression->m_Right = $3.Expression;
                    $$.Count = $1.Count + $3.Count;
                    }
                | TOKEN_LEFT_PARENTHESIS expression_words TOKEN_RIGHT_PARENTHESIS
                    { 
                    $$ = $2; 
                    }

%%

/**************************************************************************************************/

/* 
 * vim: syntax=yacc ts=4 sw=4 sts=4 sr et columns=100 lines=45 
 */
