//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// QtCore and QByteArray needed before bison.hpp
#include <QtCore>
#include <QByteArray>
#include "cb_asm_8080.bison.hpp"

#include "cb_8080.h"
#include "cb_Line.h"
#include "cb_exp_node.h"
#include "cb_Unit.h"

extern cb_Unit Unit;
extern bool   GlobalError;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const char* cb_Line::NameFromType(int Type)
    {
    switch(Type)
        {
        case LINETYPE_SYNTAXERROR    : return("(Syntax error)");
        case LINETYPE_EMPTY          : return("(Empty line)");
        case TOKEN_EQU               : return("EQU");
        case TOKEN_ORG               : return("ORG");
        case TOKEN_DB                : return("DB");
        case TOKEN_DW                : return("DW");
        case TOKEN_DS                : return("DS");
        case TOKEN_OPCODE            : return("OPCODE");
        case TOKEN_OPCODE_REG        : return("OPCODE_REG");
        case TOKEN_OPCODE_REG_REG    : return("OPCODE_REG_REG");
        case TOKEN_OPCODE_REG_EXP_8  : return("OPCODE_REG_EXP_8");
        case TOKEN_OPCODE_REG_EXP_16 : return("OPCODE_REG_EXP_16");
        case TOKEN_OPCODE_EXP_8      : return("OPCODE_EXP_8");
        case TOKEN_OPCODE_EXP_16     : return("OPCODE_EXP_16");
        default                      : return("?????");
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_Line::Print(FILE* File)
    {
    fprintf(File, "Line %d\n", m_LineNumber);
    if (m_Label.size()) fprintf(File, "\tLabel      : '%s'\n", m_Label.data());
    fprintf(File, "\tType       : %s\n", NameFromType(m_Type));
    if (m_OpCode.size()) 
        fprintf(File, "\tOpCode     : %s\n", m_OpCode.data());
    fprintf(File, "\tOrg        : %04X\n", m_Org);
    if (m_Register.size()) 
        fprintf(File, "\tRegister   : %s\n", m_Register.data());
    if (m_Register2.size()) 
        fprintf(File, "\tRegister2  : %s\n", m_Register2.data());
    fprintf(File, "\tValue      : %04X\n", m_Value);
    fprintf(File, "\tHaveValue  : %d\n", m_HaveValue);
    fprintf(File, "\tByteCount  : %d\n", m_Bytes.size());
    if (m_Bytes.size())
        {
        fprintf(File, "\tBytes      : ");
        for (int i=0; i<m_Bytes.size(); i++)
            {   
            fprintf(File, "%02X ", (unsigned char)m_Bytes.at(i));
            }
        fprintf(File, "\n");
        }
    if (m_Expression) 
        {
        fprintf(File, "\tExpression :\n");
        m_Expression->Print(File, 2);
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool MnemonicCompare(const char* FromTable, const char* FromCode)
    {
    int i;
    for (i=0; *(FromTable+i) and *(FromCode+i); i++)
        {
        if (*(FromTable+i) != *(FromCode+i)) return false;
        }
    if ( !*(FromCode+i) and !*(FromTable+i) ) return true;
    if ( !*(FromCode+i) and isspace(*(FromTable+i)) ) return true;
    return false;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_Line::SetOpCode(QByteArray OpCode)
    {
    m_OpCode = OpCode;
    for (int i=0; i<0x100; i++)
        {   
        if (MnemonicCompare(OpCodes[i].m_Mnemonic, OpCode))
            {
            switch(OpCodes[i].m_AddrMode)
                {
                case c_Addr_Implicit :
                    Unit.m_CurrentOrg += 1;
                    break;
                case c_Addr_Immediate1 :
                case c_Addr_Direct1 :
                    Unit.m_CurrentOrg += 2;
                    break;
                case c_Addr_Immediate2 :
                case c_Addr_Direct2 :
                    Unit.m_CurrentOrg += 3;
                    break;
                }
            return;
            }
        }
    // Internal error that never should happen. Lexer does guarantee opcodes.
    GlobalError = true;
    fprintf(stderr, 
            "Error on line %d : %s\n", 
            m_LineNumber, 
            "No such mnemonic (internal error)");
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 
