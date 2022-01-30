//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <stdarg.h>

#include <QtCore>
#include <QByteArray>
#include <QList>

#include "cbLine.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct Identifier
    {
    QByteArray  m_Name;
    qint32      m_Value;
    int         m_LineNumber;
    QByteArray  m_Type;
    };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct SourceLine
    {
    QByteArray  m_Text;
    int         m_LineNumber;
    SourceLine()
        {
        m_LineNumber = 0;
        }
    SourceLine(SourceLine* S)
        {
        m_Text       = S->m_Text;
        m_LineNumber = S->m_LineNumber;
        }
    };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct cbUnit
    {
    int     m_LineNumber;
    qint32  m_CurrentOrg;
    cbLine* m_Line;

    QList <SourceLine*>  m_Source;
    QList <cbLine*>      m_Lines;
    QList <Identifier*>  m_Identifiers;

    cbUnit()
        {
        m_LineNumber = 1;
        m_CurrentOrg = 0;
        m_Line       = new cbLine(m_LineNumber, m_CurrentOrg);
        }

    void Process(FILE* File);

    void CommitLine();
    void Assemble();

    void ResolveExpressions(const bool ReportError);

    QByteArray AddIdentifier(Identifier* Identifier);
    QByteArray FindIdentifier(QByteArray Id, int&  Value);

    void DumpHex(FILE* File);
    void DumpBin(FILE* File);
    void DumpList(FILE* File);
    void DumpIdentifiers(FILE* File);

    void DumpAST(FILE* File)
        {
        for (auto Line : m_Lines)
            {
            Line->Print(File);
            }
        }

    void CheckRange(cbLine* Line, bool Word);
    void ErrorMessage(cbLine* Line, const char* Format, ...);
    };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 
