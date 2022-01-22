//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <stdlib.h>
#include <string.h>

#include "cb_8080.h"
#include "cb_exp_node.h"
#include "cb_Unit.h"

extern cb_Unit Unit;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

QByteArray cb_exp_node::EvaluateInt(qint32& Value, bool ReportError)
    {
    QByteArray RV;
    switch (m_Type)
        {
        case Type_Integer :
            Value = m_Value.toInt(); 
            return RV;

        case Type_Identifier :
            return Unit.FindIdentifier(m_Value.toByteArray(), Value);

        case Type_Op_Plus :
        case Type_Op_Minus :
        case Type_Op_Mult :
        case Type_Op_Div :
        case Type_Op_Mod   :
        case Type_Op_SHR   :
        case Type_Op_SHL   :
        case Type_Op_AND   :
        case Type_Op_OR    :
        case Type_Op_XOR   :
        case Type_Op_EQ    :
        case Type_Op_NE    :
        case Type_Op_LT    :
        case Type_Op_LE    :
        case Type_Op_GT    :
        case Type_Op_GE    :
            {
            qint32 LValue;
            qint32 RValue;
            QByteArray LError = m_Left->EvaluateInt(LValue, ReportError);
            QByteArray RError = m_Right->EvaluateInt(RValue, ReportError);
            switch(m_Type)
                {
                case Type_Op_Plus  : Value = LValue + RValue; break;
                case Type_Op_Minus : Value = LValue - RValue; break;
                case Type_Op_Mult  : Value = LValue * RValue; break;
                case Type_Op_Div   : Value = LValue / RValue; break;
                case Type_Op_Mod   : Value = LValue % RValue; break;
                case Type_Op_SHR   : Value = LValue >> RValue; break;
                case Type_Op_SHL   : Value = LValue << RValue; break;
                case Type_Op_AND   : Value = ( LValue & RValue ) & 1; break;
                case Type_Op_OR    : Value = ( LValue | RValue ) & 1; break;
                case Type_Op_XOR   : Value = ( LValue ^ RValue ) & 1; break;
                // Logical operators should return all one's (-1) if true.
                case Type_Op_EQ    : Value = (LValue == RValue) ? -1 : 0; break;
                case Type_Op_NE    : Value = (LValue != RValue) ? -1 : 0;  break;
                case Type_Op_LT    : Value = ((unsigned)LValue< (unsigned)RValue) ? -1 : 0; break;
                case Type_Op_LE    : Value = ((unsigned)LValue<=(unsigned)RValue) ? -1 : 0; break;
                case Type_Op_GT    : Value = ((unsigned)LValue> (unsigned)RValue) ? -1 : 0; break;
                case Type_Op_GE    : Value = ((unsigned)LValue>=(unsigned)RValue) ? -1 : 0; break;
                }
            if (LError.size() or RError.size())
                {
                return LError + " " + RError;
                }
            return RV;
            }

        case Type_Op_UnaryMinus :
        case Type_Op_NOT   :
        case Type_Op_HIGH  :
        case Type_Op_LOW   :
            {
            qint32 RValue;
            QByteArray RError = m_Right->EvaluateInt(RValue, ReportError);
            if (RError.size())
                {
                return RError;
                }
            switch(m_Type)
                {
                case Type_Op_UnaryMinus : Value = -RValue; break;
                case Type_Op_NOT : Value = (~RValue) & 1; break;
                case Type_Op_HIGH : Value = (RValue >> 8) & 0xFF; break;
                case Type_Op_LOW : Value = RValue & 0xFF; break;
                }
            return RV;
            }

        case Type_String :
            Value = m_Value.toByteArray().at(0);
            if (m_Value.toByteArray().size() != 1)
                {
                return QByteArray("Internal error. Type Q unexpected length.");
                }
            return RV;

        default :
            return QByteArray("Internal error. Unknown type " + m_Type);
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

QByteArray cb_exp_node::EvaluateByteWord(bool        Word, 
                                       QByteArray& Bytes,
                                       cb_Line*&    Line,
                                       bool        ReportError)
    {
    QByteArray RV;
    switch(m_Type)
        {
        // a part of our byte expression that actually is int expression.
        case Type_Expression :
            {
            qint32 Value;
            QByteArray LError = m_Left->EvaluateInt(Value, ReportError);
            if (LError.size())
                {
                return LError;
                }
            Bytes.resize(Word ? 2 : 1);
            Bytes[0] = Value & 0xFF;
            if (Word)
                {
                Bytes[1] = (Value>>8) & 0xFF;
                }
            return RV;
            }
            
        // Concatenate bytes.
        case Type_Op_Concat :
            {
            QByteArray LBytes;
            QByteArray RBytes;
            QByteArray LError = m_Left->EvaluateByteWord(Word, LBytes, Line, ReportError);
            QByteArray RError = m_Right->EvaluateByteWord(Word, RBytes, Line, ReportError);
            if (LError.size() or RError.size())
                {
                return LError + " " + RError;
                }
            Bytes = LBytes + RBytes;
            return RV;
            }

        // String.
        case Type_String :
            {
            Bytes = m_Value.toByteArray();
            return RV;
            }

        // This is the tricky corner case of DB (ADD C) and alike. Try assemble.
        case Type_Instruction :
            {
            char Assembly[32];
            if (Line->m_Register2.size())
                {
                snprintf(Assembly, 32, 
                         "%-6s%s,%s", 
                         Line->m_OpCode.data(), 
                         Line->m_Register.data(),
                         Line->m_Register2.data());
                }
            else if (Line->m_Register.size())
                {
                snprintf(Assembly, 32, 
                         "%-6s%s", 
                         Line->m_OpCode.data(), 
                         Line->m_Register.data());

                }
            else
                {
                snprintf(Assembly, 32, "%s", Line->m_OpCode.data());
                }
            // Per definition only one byte (the leftmost, see 2.7 of ref manual).
            Bytes.resize(1);
            int i;
            for (i=0; i<0x100; i++)
                {   
                if (!strcmp(OpCodes[i].m_Mnemonic, Assembly))
                    {
                    Line->m_Bytes[0] = i;
                    break;
                    }
                }
            if (i == 0x100)
                {
                // This ensures that second pass will generate the error.
                Bytes.clear();
                return QByteArray( "Could not get object code for '" +
                                   QByteArray(Assembly) +
                                   "'");
                }
            return RV;
            }

        // Fallthrough that should not happen !
        default :
            return QByteArray("Internal error. Unknown type " + m_Type);
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const char* cb_exp_node::NameFromType(int Type)
    { 
    switch(Type)
        {  
        case Type_Integer       : return("Integer");
        case Type_Identifier    : return("Identifier");
        case Type_String        : return("String");
        case Type_Instruction   : return("Instruction"); 
        case Type_Expression    : return("Expression"); 
        case Type_Op_Plus       : return("Op_Plus");
        case Type_Op_Minus      : return("Op_Minus");
        case Type_Op_Mult       : return("Op_Mult");
        case Type_Op_Div        : return("Op_Div");
        case Type_Op_Mod        : return("Op_Mod");
        case Type_Op_Concat     : return("Op_Concat");
        case Type_Op_SHR        : return("Op_SHR");
        case Type_Op_SHL        : return("Op_SHL");
        case Type_Op_UnaryMinus : return("Op_UnaryMinus");
        case Type_Op_NOT        : return("Op_NOT");
        case Type_Op_AND        : return("Op_AND");
        case Type_Op_OR         : return("Op_OR");
        case Type_Op_XOR        : return("Op_XOR");
        case Type_Op_EQ         : return("Op_EQ");
        case Type_Op_NE         : return("Op_NE");
        case Type_Op_LT         : return("Op_LT");
        case Type_Op_LE         : return("Op_LE");
        case Type_Op_GT         : return("Op_GT");
        case Type_Op_GE         : return("Op_GE");
        case Type_Op_HIGH       : return("Op_HIGH");
        case Type_Op_LOW        : return("Op_LOW");
        default                 : return("?????");
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const char* cb_exp_node::NameFromTarget(int Target)
    {
    switch(Target)
        {
        case Target_Integer : return("Integer");
        case Target_Bytes   : return("Bytes");
        case Target_Words   : return("Words");
        default             : return("?????");
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_exp_node::Print(FILE* File, int Depth )
    {
    for (int i=0; i<Depth; i++) fprintf(File, "\t");
    fprintf(File, "Target : %s\n", NameFromTarget(m_Target));
    for (int i=0; i<Depth; i++) fprintf(File, "\t");
    if (m_Type == cb_exp_node::Type_Identifier)
        {
        fprintf(File, "Identifier : '%s'\n", m_Value.toByteArray().data());
        }
    else if (m_Type == cb_exp_node::Type_Integer)
        {
        fprintf(File, "Value  : %04X\n", m_Value.toInt());
        }
    else 
        {
        fprintf(File, "Type   : %s\n", NameFromType(m_Type));
        }
    if (m_Left)
        {
        for (int i=0; i<Depth; i++) fprintf(File, "\t");
        fprintf(File, "Left   :\n");
        m_Left->Print(File, Depth+1);
        }
    if (m_Right)
        {
        for (int i=0; i<Depth; i++) fprintf(File, "\t");
        fprintf(File, "Right   :\n");
        m_Right->Print(File, Depth+1);
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 
