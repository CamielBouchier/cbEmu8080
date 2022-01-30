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

#include "cbExpNode.h"

#define LINETYPE_EMPTY       -1
#define LINETYPE_SYNTAXERROR -2

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Concerning qint32 :
//
// Ref manual page 53 describes all internal operations use signed 16 bit values.
// To use comfortably >> without having to worry about implementation defined shifting in 1,
// I choose for a 32bit signed which has always the same results, but not above problem for
// shifts less than 16.

struct cbLine
    {
    int        m_LineNumber;
    qint32     m_Org;      
    qint32     m_OriginalOrg;
    QByteArray m_OpCode;
    QByteArray m_Label;
    int        m_Type;
    QByteArray m_Register;
    QByteArray m_Register2;
    QByteArray m_Bytes;
    cbExpNode* m_Expression;
    qint32     m_Value; 
    bool       m_HaveValue;

    cbLine(int LineNumber, int Org)
        {
        m_Type          = LINETYPE_EMPTY;
        m_LineNumber    = LineNumber;
        m_Org           = Org;
        m_OriginalOrg   = Org;
        m_Expression    = NULL;
        m_Value         = 0;
        m_HaveValue     = false;
        }

    void SetOpCode(QByteArray OpCode);

    void Print(FILE* File);
    const char* NameFromType(int Type);
    };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 
