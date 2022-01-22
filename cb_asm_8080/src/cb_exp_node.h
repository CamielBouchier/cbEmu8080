//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <stdio.h>

#include <QtCore>
#include <QByteArray>
#include <QVariant>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct cb_exp_node
    { 
    static const int Type_Integer       = 0;
    static const int Type_Identifier    = 1;
    static const int Type_String        = 2;
    static const int Type_Instruction   = 3; // see page 2.7 of ref manual : DB (ADD C)
    static const int Type_Expression    = 4; // I.e. int expression within bytes target.
    static const int Type_Op_Plus       = 5;
    static const int Type_Op_Minus      = 6;
    static const int Type_Op_Mult       = 7;
    static const int Type_Op_Div        = 8;
    static const int Type_Op_Mod        = 9;
    static const int Type_Op_Concat     = 10;
    static const int Type_Op_SHR        = 11;
    static const int Type_Op_SHL        = 12;
    static const int Type_Op_UnaryMinus = 13;
    static const int Type_Op_NOT        = 14;
    static const int Type_Op_AND        = 15;
    static const int Type_Op_OR         = 16;
    static const int Type_Op_XOR        = 17;
    static const int Type_Op_EQ         = 18;
    static const int Type_Op_NE         = 19;
    static const int Type_Op_LT         = 20;
    static const int Type_Op_LE         = 21;
    static const int Type_Op_GT         = 22;
    static const int Type_Op_GE         = 23;
    static const int Type_Op_HIGH       = 24;
    static const int Type_Op_LOW        = 25;

    static const int Target_Integer     = 0;
    static const int Target_Bytes       = 1;
    static const int Target_Words       = 2;

    int        m_Type; 
    int        m_Target;
    cb_exp_node* m_Left;
    cb_exp_node* m_Right;
    QVariant   m_Value;
    cb_exp_node(int Type, int Target)
        {
        m_Type   = Type;
        m_Target = Target;
        m_Left   = NULL;
        m_Right  = NULL;
        }
    QByteArray EvaluateInt(qint32& Value, bool ReportError);
    QByteArray EvaluateByteWord(bool        Word, 
                                QByteArray& Bytes,
                                /* Used for the corner case of DB (ADD C) */
                                struct      cb_Line*& Line,
                                bool        ReportError);

    void Print(FILE* File, int Depth);

    const char* NameFromType(int Type);
    const char* NameFromTarget(int Target);
    };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 
