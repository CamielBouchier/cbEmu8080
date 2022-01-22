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
#include "cb_asm_8080.bison.hpp"
#include "cb_Unit.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// This is quite some hocus-pocus to get yy_scan_string correctly in.
typedef struct yy_buffer_state* YY_BUFFER_STATE;
extern  int yyparse();
extern  YY_BUFFER_STATE yy_scan_string(const char * str);
extern  void yy_delete_buffer(YY_BUFFER_STATE buffer);

extern char* yytext;

extern  bool  GlobalError;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool IsRegister(QByteArray Reg)
    {
    if (Reg == "A") return true;
    if (Reg == "B") return true;
    if (Reg == "C") return true;
    if (Reg == "D") return true;
    if (Reg == "E") return true;
    if (Reg == "F") return true;
    if (Reg == "M") return true;
    if (Reg == "H") return true;
    if (Reg == "L") return true;
    if (Reg == "SP") return true;
    if (Reg == "PSW") return true;
    return false;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_Unit::CommitLine()
    {
    // fprintf(stderr, "Committing line %d\n", m_Line->m_LineNumber);
    m_Lines.push_back(m_Line);

    // Should we have m_Register ? Is it valid ?
    if (m_Line->m_Type == TOKEN_OPCODE_REG or 
        m_Line->m_Type == TOKEN_OPCODE_REG_REG)
        {
        if (not m_Line->m_Register.size() or not IsRegister(m_Line->m_Register))
            {
            ErrorMessage(m_Line, 
                         "Invalid register specification : '%s'", 
                         m_Line->m_Register.data());
            }
        }
  
    // Should we have m_Register2 ? Is it valid ?
    if (m_Line->m_Type == TOKEN_OPCODE_REG_REG)
        {
        if (not m_Line->m_Register2.size() or not IsRegister(m_Line->m_Register2))
            {
            ErrorMessage(m_Line, 
                         "Invalid register specification : '%s'", 
                         m_Line->m_Register2.data());
            }
        }

    // Expressions need to be resolved upon ORG, SET or DS 
    // On ORG and DS , the m_CurrentOrg must be set accordingly. 
    if (m_Line->m_Type == TOKEN_ORG or 
        m_Line->m_Type == TOKEN_SET or 
        m_Line->m_Type == TOKEN_DS)
        {
        ResolveExpressions(false);
        if (not m_Line->m_HaveValue)
            {
            ErrorMessage(m_Line, "Expression undefined while evaluating ORG/SET/DS.");
            }
        if (m_Line->m_Type == TOKEN_ORG)
            {
            m_CurrentOrg = m_Line->m_Value;
            }
        else if (m_Line->m_Type == TOKEN_DS)
            {
            m_CurrentOrg += m_Line->m_Value;
            }
        }

    // If a label ( and that is not part of EQU/SET ) then the labels m_Org is an identifier.
    if (m_Line->m_Label.size() and 
        m_Line->m_Type != TOKEN_EQU and 
        m_Line->m_Type != TOKEN_SET)
        {
        Identifier* Id   = new Identifier;
        Id->m_LineNumber = m_Line->m_LineNumber;
        Id->m_Name       = m_Line->m_Label;
        Id->m_Value      = m_Line->m_Org;
        Id->m_Type       = "Label";
        QByteArray ErrMsg = AddIdentifier(Id);
        if (ErrMsg.size())
            {
            ErrorMessage(m_Line, ErrMsg.data());
            }
        }

    // Bytes.
    if (m_Line->m_Type == TOKEN_DB or m_Line->m_Type == TOKEN_DW)
        {
        m_CurrentOrg += m_Line->m_Bytes.size();
        }

    // Next translation line.
    m_Line = new cb_Line(++m_LineNumber, m_CurrentOrg);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_Unit::ResolveExpressions(const bool ReportError)
    {
    for (auto Line : m_Lines)
        {
        if (not Line->m_HaveValue and 
            Line->m_Expression and 
            Line->m_Expression->m_Target == cb_exp_node::Target_Integer)
            {
            int Value;
            QByteArray ErrMsg = Line->m_Expression->EvaluateInt(Value, ReportError);
            // If unresolved, that will be flagged by non empty ErrMsg. Empty=>Resolved.
            if (not ErrMsg.size())
                {
                Line->m_Value = Value;
                Line->m_HaveValue = true;
                if (Line->m_Type == TOKEN_EQU or Line->m_Type == TOKEN_SET)
                    {
                    Identifier* Id = new Identifier;
                    Id->m_LineNumber = Line->m_LineNumber;
                    Id->m_Name       = Line->m_Label;
                    Id->m_Value      = Value;
                    Id->m_Type       = Line->m_Type == TOKEN_EQU ? "EQU" : "SET";
                    QByteArray ErrMsg = AddIdentifier(Id);
                    if (ErrMsg.size())
                        {
                        ErrorMessage(Line, ErrMsg.data());
                        }
                    }
                }
            else 
                {
                if (ReportError)
                    {
                    ErrorMessage(Line, ErrMsg.data());
                    }
                }
            }

        if (not Line->m_HaveValue and Line->m_Expression and 
            ( Line->m_Expression->m_Target == cb_exp_node::Target_Bytes or 
              Line->m_Expression->m_Target == cb_exp_node::Target_Words) )
            {
            QByteArray ErrMsg = Line->m_Expression->
                EvaluateByteWord(Line->m_Expression->m_Target == cb_exp_node::Target_Words,
                                 Line->m_Bytes, 
                                 Line, /* Corner case DB (ADD C) uses it */
                                 ReportError);
            if (not ErrMsg.size())
                {
                Line->m_HaveValue = true;
                }
            else
                {
                if (ReportError)
                    {
                    ErrorMessage(Line, ErrMsg.data());
                    }
                }
            }
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

QByteArray cb_Unit::AddIdentifier(Identifier* Id)
    {
    QByteArray RV;
    for (auto X : m_Identifiers)
        {
        if (X->m_Name == Id->m_Name)
            {
            // We have it already. That is only OK when it is of type SET.
            if (X->m_Type == "SET")
                {
                X->m_Value = Id->m_Value;
                return RV;
                }
            // If not, it is an error.
            return QByteArray("Identifier'" + 
                              X->m_Name + 
                              "' already declared at line " + 
                              QByteArray::number(X-> m_LineNumber) + 
                              ".");
            }
        }
    // If we get here, it is a new identifier. Add it.
    m_Identifiers.push_back(Id);
    return NULL;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

QByteArray cb_Unit::FindIdentifier(QByteArray Id, int&  Value) 
    {
    QByteArray RV;
    for (auto X : m_Identifiers)
        {
        if (X->m_Name == Id)
            {
            Value = X->m_Value;
            return RV;
            }
        }
    return QByteArray("Identifier '" + Id + "' undefined.");
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_Unit::DumpIdentifiers(FILE* File)
    {
    for (auto X : m_Identifiers)
        {
        fprintf(File,
                "%s\n\tValue : %04X\n\tType  : %s\n\tLine  : %d\n",
                X->m_Name.data(),
                X->m_Value,
                X->m_Type.data(),
                X->m_LineNumber);
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_Unit::Process(FILE* File)
    {
    // getline crashes on completely empty file.
    fseek(File, 0L, SEEK_END);
    size_t Size = ftell(File);
    rewind(File);
    if (not Size) return; 

    // Slurp in the complete source.
    const int buf_size = 1024;
    char   Line[buf_size];
    int    LineNr = 1;
    while(fgets(Line, buf_size, File) )
        {
        SourceLine* X = new SourceLine();
        X->m_Text = strdup(Line);
        X->m_LineNumber = LineNr++;
        m_Source.push_back(X);
        }

    // And start parsing/processing it.
    for (int i=0; i<m_Source.size(); i++)
        {
        YY_BUFFER_STATE ScanBuffer = yy_scan_string(m_Source[i]->m_Text.data());
        int RV = yyparse();
        if (RV) 
            {
            m_Line->m_Type = LINETYPE_SYNTAXERROR;
            ErrorMessage(m_Line, "Syntax error near '%s'", yytext);
            }
        CommitLine();
        yy_delete_buffer(ScanBuffer);
        }

    // The two pass, were second time identifiers must be resolved.
    ResolveExpressions(false);
    ResolveExpressions(true);
    Assemble();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_Unit::CheckRange(cb_Line* Line, bool Word)
    {
    bool Fit = true;
    int Mask = Word ? 0xFFFF : 0xFF;
    if (Line->m_Value > 0 and Line->m_Value != (Line->m_Value & Mask)) Fit = false;
    if (Line->m_Value < 0 and -Line->m_Value != (-Line->m_Value & Mask)) Fit = false;
    if (not Fit)
        {
        ErrorMessage(Line, 
                     (Word ? "Value '%XH' unsigned does not fit in WORD." 
                           : "Value '%XH' does not fit in BYTE." ),
                     Line->m_Value);
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_Unit::Assemble()
    {
    for (auto Line : m_Lines)
        {
        char Assembly[32];
        bool HaveAssembly = true;
        switch (Line->m_Type)
            {
            case TOKEN_OPCODE :
                Line->m_Bytes.resize(1);
                snprintf(Assembly, 32, "%s", Line->m_OpCode.data());
                break;

            case TOKEN_OPCODE_REG :
                Line->m_Bytes.resize(1);
                snprintf(Assembly, 32, 
                         "%-6s%s", 
                         Line->m_OpCode.data(), 
                         Line->m_Register.data());
                break;

            case TOKEN_OPCODE_REG_REG :
                Line->m_Bytes.resize(1);
                snprintf(Assembly, 32, 
                         "%-6s%s,%s", 
                         Line->m_OpCode.data(), 
                         Line->m_Register.data(), 
                         Line->m_Register2.data());
                break;

            case TOKEN_OPCODE_REG_EXP_8 :
                Line->m_Bytes.resize(2);
                snprintf(Assembly, 32, 
                         "%-6s%s,%%02XH", 
                         Line->m_OpCode.data(), 
                         Line->m_Register.data());
                CheckRange(Line, false);
                Line->m_Bytes[1] = Line->m_Value;
                break;

            case TOKEN_OPCODE_REG_EXP_16 :
                Line->m_Bytes.resize(3);
                snprintf(Assembly, 32, 
                         "%-6s%s,%%04XH", 
                         Line->m_OpCode.data(), 
                         Line->m_Register.data());
                CheckRange(Line, true);
                Line->m_Bytes[1] = Line->m_Value & 0xFF;
                Line->m_Bytes[2] = (Line->m_Value >> 8) & 0xFF;
                break;

            case TOKEN_OPCODE_EXP_8 :
                if (Line->m_OpCode == "RST")
                    {
                    Line->m_Bytes.resize(1);
                    snprintf(Assembly, 32, 
                             "%-6s%d", 
                             Line->m_OpCode.data(),
                             Line->m_Value);
                    }
                else
                    {
                    Line->m_Bytes.resize(2);
                    snprintf(Assembly, 32, 
                             "%-6s%%02XH", 
                             Line->m_OpCode.data());
                    CheckRange(Line, false);
                    Line->m_Bytes[1] = Line->m_Value;
                    }
                break;

            case TOKEN_OPCODE_EXP_16 :
                Line->m_Bytes.resize(3);
                snprintf(Assembly, 32, 
                         "%-6s%%04XH", 
                         Line->m_OpCode.data());
                CheckRange(Line, true);
                Line->m_Bytes[1] = Line->m_Value & 0xFF;
                Line->m_Bytes[2] = (Line->m_Value >> 8) & 0xFF;
                break;

            default :
                HaveAssembly = false;
            }

        if (HaveAssembly)
            {
            int i;
            for (i=0; i<0x100; i++)
                {   
                if (!strcmp(OpCodes[i].m_Mnemonic, Assembly))
                    {
                    Line->m_Bytes[0] = i;
                    Line->m_OpCode = Assembly;
                    break;
                    }
                }
    
            if (i == 0x100)
                {
                ErrorMessage(Line, "Invalid assembly : '%s'", Assembly);
                }
            }
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void DumpHexBuffer(FILE* File, int& Address, unsigned char* Buffer, int& WriteBufPtr)
    {
    unsigned char CheckSum = 0;
    int ReadBufPtr;
    for (ReadBufPtr=0; ReadBufPtr<WriteBufPtr; ReadBufPtr++)
        {
        if (ReadBufPtr % 16 == 0)
            {
            CheckSum += Address >> 8;
            CheckSum += Address & 0xFF;
            int NrBytes = std::min(16,WriteBufPtr-ReadBufPtr);
            fprintf(File, ":%02X%04X00", NrBytes, Address);
            Address += 16;
            }
        CheckSum += Buffer[ReadBufPtr];
        fprintf(File, "%02X", Buffer[ReadBufPtr]);
        if (ReadBufPtr % 16 == 15)
            {
            CheckSum += 16;
            CheckSum = ~CheckSum + 1;
            fprintf(File, "%02X\n", CheckSum);
            CheckSum = 0;
            }
        }
    if (ReadBufPtr % 16 != 0)
        {
        CheckSum += ReadBufPtr % 16 ;
        CheckSum = ~CheckSum + 1;
        fprintf(File, "%02X\n", CheckSum);
        }
    WriteBufPtr = 0;
    CheckSum = 0;
    }

void cb_Unit::DumpHex(FILE* File)
    {
    unsigned char Buffer[0x10000];
             int  WriteBufPtr = 0;
             int  Address;
             int  ExpectedAddress;
             bool Running = false;

    for (auto Line : m_Lines)
        {
        bool LastIt = (Line == m_Lines.back());
        if (Running and Line->m_Org != ExpectedAddress)
            {
            DumpHexBuffer(File, Address, Buffer, WriteBufPtr);
            }
        Running = true;
        for (int i=0; i < Line->m_Bytes.size(); ) 
            {
            if (WriteBufPtr == 0) Address = Line->m_Org;
            // The part after : is for an internal error case not to crash. Should not happen.
            Buffer[WriteBufPtr++] = Line->m_Bytes.size() ? Line->m_Bytes.at(i++) : i++;
            }
        ExpectedAddress = Line->m_Org + Line->m_Bytes.size();
        if (LastIt)
            {
            DumpHexBuffer(File, Address, Buffer, WriteBufPtr);
            }
        }
    fprintf(File, ":00000001FF\n");
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
void cb_Unit::DumpBin(FILE* File)
    {
    unsigned char Buffer[0x10000];
             int  Address;
             int  FirstAddress = -1;
    memset(Buffer, 0, 0x10000);
    for (auto Line : m_Lines)
        {
        for (int i=0; i < Line->m_Bytes.size(); ) 
            {
            if (i==0) 
                {
                Address = Line->m_Org;
                if (FirstAddress < 0)
                    {
                    FirstAddress = Address;
                    }
                }
            // The part after : is for an internal error case not to crash. Should not happen.
            Buffer[Address++] = Line->m_Bytes.size() ? Line->m_Bytes.at(i++) : i++;
            }
        }
    fwrite(Buffer+FirstAddress, Address-FirstAddress, 1, File);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_Unit::DumpList(FILE* File)
    {
    for (auto Line : m_Lines)
        {
        QByteArray SourceLine   = m_Source[Line->m_LineNumber-1]->m_Text;
        int   SourceLineNr = m_Source[Line->m_LineNumber-1]->m_LineNumber;
        switch (Line->m_Type)
            {
            case TOKEN_EQU :
            case TOKEN_SET :
            case TOKEN_ORG :
                fprintf(File, 
                        "%6d%6s%02X %02X%9s\t%s", 
                        SourceLineNr,          
                        "",
                        (Line->m_Value >> 8) & 0xFF,
                        Line->m_Value & 0xFF,
                        "",
                        SourceLine.data());
                break;

            case TOKEN_DB :
            case TOKEN_DW :
                fprintf(File, 
                        "%6d %04X %14s\t%s", 
                        SourceLineNr,          
                        Line->m_Org,
                        "",
                        SourceLine.data());
                fprintf(File, "%12s", "");
                for (int i=0; i < Line->m_Bytes.size(); i++)
                    {
                    fprintf(File, "%02X ", (unsigned char) Line->m_Bytes[i]);
                    if ( (i % 4 == 3) and (i != Line->m_Bytes.size()-1) )
                        {
                        fprintf(File, "\n%12s", "");
                        }
                    }
                fprintf(File, "\n");
                break;

            case TOKEN_OPCODE            :
            case TOKEN_OPCODE_REG        :
            case TOKEN_OPCODE_REG_REG    :
            case TOKEN_OPCODE_REG_EXP_8  :
            case TOKEN_OPCODE_REG_EXP_16 :
            case TOKEN_OPCODE_EXP_8      :
            case TOKEN_OPCODE_EXP_16     :
                switch (Line->m_Bytes.size())
                    {
                    case 1 :
                        fprintf(File, 
                                "%6d %04X %02X%12s\t%s", 
                                SourceLineNr,          
                                Line->m_Org,
                                (unsigned char)Line->m_Bytes[0],
                                "",
                                SourceLine.data());
                        break;
                    case 2 :
                        fprintf(File, 
                                "%6d %04X %02X %02X%9s\t%s", 
                                SourceLineNr,          
                                Line->m_Org,
                                (unsigned char)Line->m_Bytes[0],
                                (unsigned char)Line->m_Bytes[1],
                                "",
                                SourceLine.data());
                        break;
                    case 3 :
                        fprintf(File, 
                                "%6d %04X %02X %02X %02X%6s\t%s", 
                                SourceLineNr,          
                                Line->m_Org,
                                (unsigned char)Line->m_Bytes[0],
                                (unsigned char)Line->m_Bytes[1],
                                (unsigned char)Line->m_Bytes[2],
                                "",
                                SourceLine.data());
                        break;
                    }
                break;

            default :
                fprintf(File, 
                        "%6d%20s\t%s", 
                        SourceLineNr,
                        "",
                        SourceLine.data());
            }
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int yyerror(const char*) {return 0;};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_Unit::ErrorMessage(cb_Line* Line, const char* Format, ...)
    {
    GlobalError = true;
    fprintf(stderr, "Error at line %d : ", Line->m_LineNumber);
    va_list Args;
    va_start(Args, Format);
    vfprintf(stderr, Format, Args);
    fprintf(stderr,"\n");
    va_end(Args);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 
