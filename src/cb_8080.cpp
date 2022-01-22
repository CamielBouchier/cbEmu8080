//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "cb_8080.h"

#include "cb_Defines.h"
#include "cb_Model.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cb_8080::cb_8080(cb_Model* Model)
    {
    m_Model     = Model;
    m_TraceFile = NULL;
    Reset();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_8080::SetDoTrace(const bool DoTrace) 
    {
    if (m_TraceFile) 
        {
        fclose(m_TraceFile);
        }

    if (DoTrace) 
        {
        m_TraceFile = fopen(C_STRING(m_TraceFileName), "wt");
        if (not m_TraceFile) 
            {
            qWarning("Could not open logfile : '%s'", C_STRING(m_TraceFileName));
            return;
            }
        }

    m_DoTrace = DoTrace;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_8080::FlushTrace()
    {
    if (m_TraceFile)
        {
        fflush(m_TraceFile);
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_8080::Reset() 
    {
    m_Halted           = false;
    m_InterruptEnabled = false;
    RegPC              = 0;
    m_ProcessorState   = c_StateFetch;

    // Shouldn't in reality (done for reproduceability).
    RegA         = 0;
    m_BC.W       = 0;
    m_DE.W       = 0;
    m_HL.W       = 0;
    m_SP.W       = 0;
    m_FlagZ      = 0;
    m_FlagC      = 0;
    m_FlagA      = 0;
    m_FlagP      = 0;
    m_FlagS      = 0;
    m_IR         = 0;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cb_8080::~cb_8080() 
    {
    qDebug("Start destructor");
    qDebug("End   destructor");
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// XXX CB : Be aware that the spurious Read() might jeopardize
// memory mapped IO devices where a read has a side effect !
// Allthough that would be a very degenerate case , the disassembler
// reads in the code stream, not the data stream.

QString cb_8080::Disassemble(const uint16_t Address) 
    {
    QString Disassembled;
    uint8_t DataL;
    uint8_t DataH;
    uint8_t IR = m_Model->Read(Address);
    switch (OpCodes[IR].AddrMode) 
        {
        case c_Addr_Immediate1 :
        case c_Addr_Direct1    :
            DataL = m_Model->Read(Address+1);
            Disassembled = QString::asprintf(OpCodes[IR].Mnemonic, DataL);
            break;
        case c_Addr_Immediate2 :
        case c_Addr_Direct2    :
            DataL = m_Model->Read(Address+1);
            DataH = m_Model->Read(Address+2);
            Disassembled = QString::asprintf(OpCodes[IR].Mnemonic, (DataH<<8)|DataL);
            break;
        default :
            Disassembled = QString::asprintf(OpCodes[IR].Mnemonic);
        }
    return QString("%1: ").arg(Address,4,16,QChar('0')).toUpper() + Disassembled;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_8080::ClockTick() 
    {
    if (m_ProcessorState == c_StateFetch) 
        {
        if (m_Halted) return;

        m_IR = m_Model->Read(RegPC);

        if (m_DoTrace)
            {
            m_Disassembled = Disassemble(RegPC);
            }

        RegPC++; 
        }
  
    else if (m_ProcessorState == c_StateDecode) 
        {
        // Call the opcode function.
        (*this.*OpCodes[m_IR].OpFunction)();
        m_IStates = OpCodes[m_IR].NrStates;

        if (m_DoTrace)
            {
            // Working with C-files does speed up nearly with 60%.
            // This is due to bypassing UTF etc, which is not needed anyway here.
            fprintf(m_TraceFile, 
                    "%s\n\tA:%02X\tZ:%d C:%d E:%d M:%d A:%d\n\tBC:%04X DE:%04X HL:%04X SP:%04X\n",
                    C_STRING(m_Disassembled),
                    RegA,m_FlagZ,m_FlagC,m_FlagP,m_FlagS,m_FlagA,m_BC.W,m_DE.W,m_HL.W,m_SP.W);
            }
        }

    // Just consume cycles until matching the clock cycle count 
    // of the instruction.
    m_ProcessorState ++;
    if (m_ProcessorState == m_IStates) 
        {
        m_ProcessorState = c_StateFetch;
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Parity and HalfCarry tables.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const bool    cb_8080::HalfCarry[] = { 0, 0, 1, 0, 1, 0, 1, 1 };
const bool cb_8080::SubHalfCarry[] = { 0, 1, 1, 1, 0, 0, 0, 1 };

const bool cb_8080::EvenParity[] = 
    {
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// ConditionMet(). CCC is always coded on the same place in the assembly.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool cb_8080::ConditionMet() 
    {
    switch(m_IR & 070) 
        {
        case 000 : return not m_FlagZ;
        case 010 : return     m_FlagZ;
        case 020 : return not m_FlagC;
        case 030 : return     m_FlagC;
        case 040 : return not m_FlagP;
        case 050 : return     m_FlagP;
        case 060 : return not m_FlagS;
        case 070 : return     m_FlagS;
        }
    return false; // To keep gcc happy.
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Routines for the different OpCodes.
// Common part put in macro's.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define POP(Reg)                                \
    {                                           \
    cb_RegPair Tmp;                             \
    Tmp.L = m_Model->Read(RegSP++);             \
    Tmp.H = m_Model->Read(RegSP++);             \
    Reg   = Tmp.W;                              \
    }

#define PUSH(Reg)                               \
    {                                           \
    cb_RegPair Tmp;                             \
    Tmp.W = Reg;                                \
    m_Model->Write(--RegSP,Tmp.H);              \
    m_Model->Write(--RegSP,Tmp.L);              \
    }

#define ADC(Data)                               \
    {                                           \
    uint8_t  SData = Data; /*Avoid side eff.*/  \
    uint16_t Sum = RegA + SData + m_FlagC;      \
    uint8_t  Index = 0x07 &                     \
       ( ((RegA  & 0x88) >> 1) |                \
         ((SData & 0x88) >> 2) |                \
         ((Sum   & 0x88) >> 3) );               \
    m_FlagA = HalfCarry[Index];                 \
    RegA    = Sum & 0XFF;                       \
    m_FlagC = Sum & 0x100;                      \
    m_FlagS = RegA & 0x80;                      \
    m_FlagZ = (RegA==0);                        \
    m_FlagP = EvenParity[RegA];                 \
    }

#define ADD(Data)                               \
    {                                           \
    uint8_t  SData = Data; /*Avoid side eff.*/  \
    uint16_t Sum = RegA + SData;                \
    uint8_t  Index = 0x07 &                     \
      ( ((RegA  & 0x88) >> 1) |                 \
        ((SData & 0x88) >> 2) |                 \
        ((Sum   & 0x88) >> 3) );                \
    m_FlagA = HalfCarry[Index];                 \
    RegA    = Sum & 0XFF;                       \
    m_FlagC = Sum & 0x100;                      \
    m_FlagS = RegA & 0x80;                      \
    m_FlagZ = (RegA==0);                        \
    m_FlagP = EvenParity[RegA];                 \
    }

#define ANA(Data)                               \
    {                                           \
    uint8_t  SData = Data; /*Avoid side eff.*/  \
    m_FlagA = (RegA | SData) & 0x08;            \
    RegA   &= SData;                            \
    m_FlagC = false;                            \
    m_FlagS = RegA & 0x80;                      \
    m_FlagZ = (RegA==0);                        \
    m_FlagP = EvenParity[RegA];                 \
    }

#define CMP(Data)                               \
    {                                           \
    uint8_t  SData = Data; /*Avoid side eff.*/  \
    uint16_t Sum = RegA - SData;                \
    uint8_t  Index = 0x07 &                     \
      ( ((RegA  & 0x88) >> 1) |                 \
        ((SData & 0x88) >> 2) |                 \
        ((Sum   & 0x88) >> 3) );                \
    m_FlagA = not SubHalfCarry[Index];          \
    uint8_t A = Sum & 0XFF;                     \
    m_FlagC = Sum & 0x100;                      \
    m_FlagS = A & 0x80;                         \
    m_FlagZ = (A==0);                           \
    m_FlagP = EvenParity[A];                    \
    }

#define DAD(Data)                               \
    {                                           \
    uint32_t Sum = RegHL + Data;                \
    RegHL   = Sum & 0X0FFFF;                    \
    m_FlagC = Sum & 0X10000;                    \
    }

#define DCR(Reg)                                \
    {                                           \
    uint8_t SReg = --Reg; /*Avoid side eff.*/   \
    m_FlagA = ((SReg & 0x0f) != 0x0f);          \
    m_FlagS = SReg & 0x80;                      \
    m_FlagZ = (SReg==0);                        \
    m_FlagP = EvenParity[SReg];                 \
    }

#define INR(Reg)                                \
    {                                           \
    uint8_t SReg = ++Reg; /*Avoid side eff.*/   \
    m_FlagA = not (SReg & 0x0f);                \
    m_FlagS = SReg & 0x80;                      \
    m_FlagZ = (SReg==0);                        \
    m_FlagP = EvenParity[SReg];                 \
    }

#define ORA(Data)                               \
    {                                           \
    RegA   |= Data;                             \
    m_FlagA = false;                            \
    m_FlagC = false;                            \
    m_FlagS = RegA & 0x80;                      \
    m_FlagZ = (RegA==0);                        \
    m_FlagP = EvenParity[RegA];                 \
    }

#define SBB(Data)                               \
    {                                           \
    uint8_t  SData = Data; /*Avoid side eff.*/  \
    uint16_t Sum = RegA - SData - m_FlagC;      \
    uint8_t  Index = 0x07 &                     \
      ( ((RegA  & 0x88) >> 1) |                 \
        ((SData & 0x88) >> 2) |                 \
        ((Sum   & 0x88) >> 3) );                \
    m_FlagA = not SubHalfCarry[Index];          \
    RegA    = Sum & 0XFF;                       \
    m_FlagC = Sum & 0x100;                      \
    m_FlagS = RegA & 0x80;                      \
    m_FlagZ = (RegA==0);                        \
    m_FlagP = EvenParity[RegA];                 \
    }

#define SUB(Data)                               \
    {                                           \
    uint8_t  SData = Data; /*Avoid side eff.*/  \
    uint16_t Sum = RegA - SData;                \
    uint8_t  Index = 0x07 &                     \
      ( ((RegA  & 0x88) >> 1) |                 \
        ((SData & 0x88) >> 2) |                 \
        ((Sum   & 0x88) >> 3) );                \
    m_FlagA = not SubHalfCarry[Index];          \
    RegA    = Sum & 0XFF;                       \
    m_FlagC = Sum & 0x100;                      \
    m_FlagS = RegA & 0x80;                      \
    m_FlagZ = (RegA==0);                        \
    m_FlagP = EvenParity[RegA];                 \
    }

#define XRA(Data)                               \
    {                                           \
    RegA   ^= Data;                             \
    m_FlagA = false;                            \
    m_FlagC = false;                            \
    m_FlagS = RegA & 0x80;                      \
    m_FlagZ = (RegA==0);                        \
    m_FlagP = EvenParity[RegA];                 \
    }

void cb_8080::OpIllegal() 
    {
    m_Halted = true;
    qWarning("Illegal opcode PC=%04XH:%03oQ. Halted.",RegPC,m_IR);
    }

void cb_8080::OpACI() 
    {
    ADC(m_Model->Read(RegPC++));
    }

void cb_8080::OpADC_A() 
    {
    ADC(RegA);
    }

void cb_8080::OpADC_B() 
    {
    ADC(RegB);
    }

void cb_8080::OpADC_C() 
    {
    ADC(RegC);
    }

void cb_8080::OpADC_D() 
    {
    ADC(RegD);
    }

void cb_8080::OpADC_E() 
    {
    ADC(RegE);
    }

void cb_8080::OpADC_H() 
    {
    ADC(RegH);
    }

void cb_8080::OpADC_L() 
    {
    ADC(RegL);
    }

void cb_8080::OpADC_M() 
    {
    ADC(m_Model->Read(RegHL));
    }

void cb_8080::OpADD_A() 
    {
    ADD(RegA);
    }

void cb_8080::OpADD_B() 
    {
    ADD(RegB);
    }

void cb_8080::OpADD_C() 
    {
    ADD(RegC);
    }

void cb_8080::OpADD_D() 
    {
    ADD(RegD);
    }

void cb_8080::OpADD_E() 
    {
    ADD(RegE);
    }

void cb_8080::OpADD_H() 
    {
    ADD(RegH);
    }

void cb_8080::OpADD_L() 
    {
    ADD(RegL);
    }

void cb_8080::OpADD_M() 
    {
    ADD(m_Model->Read(RegHL));
    }

void cb_8080::OpADI() 
    {
    ADD(m_Model->Read(RegPC++));
    }

void cb_8080::OpANA_A() 
    {
    ANA(RegA);
    }

void cb_8080::OpANA_B() 
    {
    ANA(RegB);
    }

void cb_8080::OpANA_C() 
    {
    ANA(RegC);
    }

void cb_8080::OpANA_D() 
    {
    ANA(RegD);
    }

void cb_8080::OpANA_E() 
    {
    ANA(RegE);
    }

void cb_8080::OpANA_H() 
    {
    ANA(RegH);
    }

void cb_8080::OpANA_L() 
    {
    ANA(RegL);
    }

void cb_8080::OpANA_M() 
    {
    ANA(m_Model->Read(RegHL));
    }

void cb_8080::OpANI() 
    {
    ANA(m_Model->Read(RegPC++));
    }

void cb_8080::OpCALL() 
    {
    cb_RegPair Tmp;
    Tmp.L = m_Model->Read(RegPC++);
    Tmp.H = m_Model->Read(RegPC++);
    PUSH(RegPC); 
    RegPC = Tmp.W;
    }

void cb_8080::OpCCond() 
    {
    cb_RegPair Tmp;
    Tmp.L = m_Model->Read(RegPC++);
    Tmp.H = m_Model->Read(RegPC++);
    if (ConditionMet()) 
        {
        PUSH(RegPC); 
        RegPC = Tmp.W;
        } 
    }

void cb_8080::OpCMA() 
    {
    RegA ^= 0XFF;
    }

void cb_8080::OpCMC() 
    {
    m_FlagC = not m_FlagC;
    }

void cb_8080::OpCMP_A() 
    {
    CMP(RegA);
    }

void cb_8080::OpCMP_B() 
    {
    CMP(RegB);
    }

void cb_8080::OpCMP_C() 
    {
    CMP(RegC);
    }

void cb_8080::OpCMP_D() 
    {
    CMP(RegD);
    }

void cb_8080::OpCMP_E() 
    {
    CMP(RegE);
    }

void cb_8080::OpCMP_H() 
    {
    CMP(RegH);
    }

void cb_8080::OpCMP_L() 
    {
    CMP(RegL);
    }

void cb_8080::OpCMP_M() 
    {
    CMP(m_Model->Read(RegHL));
    }

void cb_8080::OpCPI() 
    {
    CMP(m_Model->Read(RegPC++));
    }

void cb_8080::OpDAA() 
    {
    uint8_t Adjust = 0x00;
    bool    Carry  = m_FlagC;
    if (m_FlagA or (RegA & 0x0f) > 9) 
        {
        Adjust = 0x06;
        }
    if (m_FlagC or (RegA >> 4) > 9 or ((RegA >> 4) >= 9 and (RegA & 0x0f) > 9)) 
        {
        Adjust |= 0x60;
        Carry = 1;
        }
    ADD(Adjust);
    m_FlagC = Carry;
    m_FlagP = EvenParity[RegA];
    }

void cb_8080::OpDAD_B() 
    {
    DAD(RegBC);
    }

void cb_8080::OpDAD_D() 
    {
    DAD(RegDE);
    }

void cb_8080::OpDAD_H() 
    {
    DAD(RegHL);
    }

void cb_8080::OpDAD_SP() 
    {
    DAD(RegSP);
    }

void cb_8080::OpDCR_A() 
    {
    DCR(RegA);
    }

void cb_8080::OpDCR_B() 
    {
    DCR(RegB);
    }

void cb_8080::OpDCR_C() 
    {
    DCR(RegC);
    }

void cb_8080::OpDCR_D() 
    {
    DCR(RegD);
    }

void cb_8080::OpDCR_E() 
    {
    DCR(RegE);
    }

void cb_8080::OpDCR_H() 
    {
    DCR(RegH);
    }

void cb_8080::OpDCR_L() 
    {
    DCR(RegL);
    }

void cb_8080::OpDCR_M() 
    {
    uint8_t Tmp = m_Model->Read(RegHL);
    DCR(Tmp);
    m_Model->Write(RegHL,Tmp);
    }

void cb_8080::OpDCX_B() 
    {
    RegBC--;
    }

void cb_8080::OpDCX_D() 
    {
    RegDE--;
    }

void cb_8080::OpDCX_H() 
    {
    RegHL--;
    }

void cb_8080::OpDCX_SP() 
    {
    RegSP--;
    }

void cb_8080::OpDI() 
    {
    m_InterruptEnabled = false;
    }

void cb_8080::OpEI() 
    {
    m_InterruptEnabled = true;
    }

void cb_8080::OpHLT() 
    {
    m_Halted = true;
    qWarning("PC=%04X. Halted.",RegPC);
    }

void cb_8080::OpIN() 
    {
    RegA = m_Model->ReadIO(m_Model->Read(RegPC++));
    }

void cb_8080::OpINR_A() 
    {
    INR(RegA);
    }

void cb_8080::OpINR_B() 
    {
    INR(RegB);
    }

void cb_8080::OpINR_C() 
    {
    INR(RegC);
    }

void cb_8080::OpINR_D() 
    {
    INR(RegD);
    }

void cb_8080::OpINR_E() 
    {
    INR(RegE);
    }

void cb_8080::OpINR_H() 
    {
    INR(RegH);
    }

void cb_8080::OpINR_L() 
    {
    INR(RegL);
    }

void cb_8080::OpINR_M() 
    {
    uint8_t Tmp = m_Model->Read(RegHL);
    INR(Tmp);
    m_Model->Write(RegHL,Tmp);
    }

void cb_8080::OpINX_B() 
    {
    RegBC++;
    }

void cb_8080::OpINX_D() 
    {
    RegDE++;
    }

void cb_8080::OpINX_H() 
    {
    RegHL++;
    }

void cb_8080::OpINX_SP() 
    {
    RegSP++;
    }

void cb_8080::OpJCond() 
    {
    cb_RegPair Tmp;
    Tmp.L = m_Model->Read(RegPC++);
    Tmp.H = m_Model->Read(RegPC++);
    if (ConditionMet()) 
        {
        RegPC = Tmp.W;
        }
    }

void cb_8080::OpJMP() 
    {
    cb_RegPair Tmp;
    Tmp.L = m_Model->Read(RegPC++);
    Tmp.H = m_Model->Read(RegPC++);
    RegPC = Tmp.W;
    }

void cb_8080::OpLDA() 
    {
    cb_RegPair Tmp;
    Tmp.L = m_Model->Read(RegPC++);
    Tmp.H = m_Model->Read(RegPC++);
    RegA = m_Model->Read(Tmp.W);
    }

void cb_8080::OpLDAX_B() 
    {
    RegA = m_Model->Read(RegBC);
    }

void cb_8080::OpLDAX_D() 
    {
    RegA = m_Model->Read(RegDE);
    }

void cb_8080::OpLHLD() 
    {
    cb_RegPair Tmp;
    Tmp.L = m_Model->Read(RegPC++);
    Tmp.H = m_Model->Read(RegPC++);
    RegL = m_Model->Read(Tmp.W);
    RegH = m_Model->Read(Tmp.W+1);
    }

void cb_8080::OpLXI_B() 
    {
    RegC = m_Model->Read(RegPC++);
    RegB = m_Model->Read(RegPC++);
    }

void cb_8080::OpLXI_D() 
    {
    RegE = m_Model->Read(RegPC++);
    RegD = m_Model->Read(RegPC++);
    }

void cb_8080::OpLXI_H() 
    {
    RegL = m_Model->Read(RegPC++);
    RegH = m_Model->Read(RegPC++);
    }

void cb_8080::OpLXI_SP() 
    {
    m_SP.L = m_Model->Read(RegPC++);
    m_SP.H = m_Model->Read(RegPC++);
    }

void cb_8080::OpMOV_A_A() 
    {
    RegA = RegA;
    }

void cb_8080::OpMOV_A_B() 
    {
    RegA = RegB;
    }

void cb_8080::OpMOV_A_C() 
    {
    RegA = RegC;
    }

void cb_8080::OpMOV_A_D() 
    {
    RegA = RegD;
    }

void cb_8080::OpMOV_A_E() 
    {
    RegA = RegE;
    }

void cb_8080::OpMOV_A_H() 
    {
    RegA = RegH;
    }

void cb_8080::OpMOV_A_L() 
    {
    RegA = RegL;
    }

void cb_8080::OpMOV_A_M() 
    {
    RegA = m_Model->Read(RegHL);
    }

void cb_8080::OpMOV_B_A() 
    {
    RegB = RegA;
    }

void cb_8080::OpMOV_B_B() 
    {
    RegB = RegB;
    }

void cb_8080::OpMOV_B_C() 
    {
    RegB = RegC;
    }

void cb_8080::OpMOV_B_D() 
    {
    RegB = RegD;
    }

void cb_8080::OpMOV_B_E() 
    {
    RegB = RegE;
    }

void cb_8080::OpMOV_B_H() 
    {
    RegB = RegH;
    }

void cb_8080::OpMOV_B_L() 
    {
    RegB = RegL;
    }

void cb_8080::OpMOV_B_M() 
    {
    RegB = m_Model->Read(RegHL);
    }

void cb_8080::OpMOV_C_A() 
    {
    RegC = RegA;
    }

void cb_8080::OpMOV_C_B() 
    {
    RegC = RegB;
    }

void cb_8080::OpMOV_C_C() 
    {
    RegC = RegC;
    }

void cb_8080::OpMOV_C_D() 
    {
    RegC = RegD;
    }

void cb_8080::OpMOV_C_E() 
    {
    RegC = RegE;
    }

void cb_8080::OpMOV_C_H() 
    {
    RegC = RegH;
    }

void cb_8080::OpMOV_C_L() 
    {
    RegC = RegL;
    }

void cb_8080::OpMOV_C_M() 
    {
    RegC = m_Model->Read(RegHL);
    }

void cb_8080::OpMOV_D_A() 
    {
    RegD = RegA;
    }

void cb_8080::OpMOV_D_B() 
    {
    RegD = RegB;
    }

void cb_8080::OpMOV_D_C() 
    {
    RegD = RegC;
    }

void cb_8080::OpMOV_D_D() 
    {
    RegD = RegD;
    }

void cb_8080::OpMOV_D_E() 
    {
    RegD = RegE;
    }

void cb_8080::OpMOV_D_H() 
    {
    RegD = RegH;
    }

void cb_8080::OpMOV_D_L() 
    {
    RegD = RegL;
    }

void cb_8080::OpMOV_D_M() 
    {
    RegD = m_Model->Read(RegHL);
    }

void cb_8080::OpMOV_E_A() 
    {
    RegE = RegA;
    }

void cb_8080::OpMOV_E_B() 
    {
    RegE = RegB;
    }

void cb_8080::OpMOV_E_C() 
    {
    RegE = RegC;
    }

void cb_8080::OpMOV_E_D() 
    {
    RegE = RegD;
    }

void cb_8080::OpMOV_E_E() 
    {
    RegE = RegE;
    }

void cb_8080::OpMOV_E_H() 
    {
    RegE = RegH;
    }

void cb_8080::OpMOV_E_L() 
    {
    RegE = RegL;
    }

void cb_8080::OpMOV_E_M() 
    {
    RegE = m_Model->Read(RegHL);
    }

void cb_8080::OpMOV_H_A() 
    {
    RegH = RegA;
    }

void cb_8080::OpMOV_H_B() 
    {
    RegH = RegB;
    }

void cb_8080::OpMOV_H_C() 
    {
    RegH = RegC;
    }

void cb_8080::OpMOV_H_D() 
    {
    RegH = RegD;
    }

void cb_8080::OpMOV_H_E() 
    {
    RegH = RegE;
    }

void cb_8080::OpMOV_H_H() 
    {
    RegH = RegH;
    }

void cb_8080::OpMOV_H_L() 
    {
    RegH = RegL;
    }

void cb_8080::OpMOV_H_M() 
    {
    RegH = m_Model->Read(RegHL);
    }

void cb_8080::OpMOV_L_A() 
    {
    RegL = RegA;
    }

void cb_8080::OpMOV_L_B() 
    {
    RegL = RegB;
    }

void cb_8080::OpMOV_L_C() 
    {
    RegL = RegC;
    }

void cb_8080::OpMOV_L_D() 
    {
    RegL = RegD;
    }

void cb_8080::OpMOV_L_E() 
    {
    RegL = RegE;
    }

void cb_8080::OpMOV_L_H() 
    {
    RegL = RegH;
    }

void cb_8080::OpMOV_L_L() 
    {
    RegL = RegL;
    }

void cb_8080::OpMOV_L_M() 
    {
    RegL = m_Model->Read(RegHL);
    }

void cb_8080::OpMOV_M_A() 
    {
    m_Model->Write(RegHL,RegA);
    }

void cb_8080::OpMOV_M_B() 
    {
    m_Model->Write(RegHL,RegB);
    }

void cb_8080::OpMOV_M_C() 
    {
    m_Model->Write(RegHL,RegC);
    }

void cb_8080::OpMOV_M_D() 
    {
    m_Model->Write(RegHL,RegD);
    }

void cb_8080::OpMOV_M_E() 
    {
    m_Model->Write(RegHL,RegE);
    }

void cb_8080::OpMOV_M_H() 
    {
    m_Model->Write(RegHL,RegH);
    }

void cb_8080::OpMOV_M_L() 
    {
    m_Model->Write(RegHL,RegL);
    }

void cb_8080::OpMVI_A() 
    {
    RegA = m_Model->Read(RegPC++);
    }

void cb_8080::OpMVI_B() 
    {
    RegB = m_Model->Read(RegPC++);
    }

void cb_8080::OpMVI_C() 
    {
    RegC = m_Model->Read(RegPC++);
    }

void cb_8080::OpMVI_D() 
    {
    RegD = m_Model->Read(RegPC++);
    }

void cb_8080::OpMVI_E() 
    {
    RegE = m_Model->Read(RegPC++);
    }

void cb_8080::OpMVI_H() 
    {
    RegH = m_Model->Read(RegPC++);
    }

void cb_8080::OpMVI_L() 
    {
    RegL = m_Model->Read(RegPC++);
    }

void cb_8080::OpMVI_M() 
    {
    m_Model->Write(RegHL,m_Model->Read(RegPC++));
    }

void cb_8080::OpNOP() 
    {
    }

void cb_8080::OpORA_A() 
    {
    ORA(RegA);
    }

void cb_8080::OpORA_B() 
    {
    ORA(RegB);
    }

void cb_8080::OpORA_C() 
    {
    ORA(RegC);
    }

void cb_8080::OpORA_D() 
    {
    ORA(RegD);
    }

void cb_8080::OpORA_E() 
    {
    ORA(RegE);
    }

void cb_8080::OpORA_H() 
    {
    ORA(RegH);
    }

void cb_8080::OpORA_L() 
    {
    ORA(RegL);
    }

void cb_8080::OpORA_M() 
    {
    ORA(m_Model->Read(RegHL));
    }

void cb_8080::OpORI() 
    {
    ORA(m_Model->Read(RegPC++));
    }

void cb_8080::OpOUT() 
    {
    m_Model->WriteIO(m_Model->Read(RegPC++),RegA);
    }

void cb_8080::OpPCHL() 
    {
    RegPC = RegHL;
    }

void cb_8080::OpPOP_BC() 
    {
    POP(RegBC);
    }

void cb_8080::OpPOP_DE() 
    {
    POP(RegDE);
    }

void cb_8080::OpPOP_HL() 
    {
    POP(RegHL);
    }

void cb_8080::OpPOP_PSW() 
    {
    cb_RegPair PSW;
    POP(PSW.W);
    RegA    = PSW.H;
    m_FlagC = PSW.L & 0x01;
    m_FlagP = PSW.L & 0x04;
    m_FlagA = PSW.L & 0x10;
    m_FlagZ = PSW.L & 0x40;
    m_FlagS = PSW.L & 0x80;
    }

void cb_8080::OpPUSH_BC() 
    {
    PUSH(RegBC);
    }

void cb_8080::OpPUSH_DE() 
    {
    PUSH(RegDE);
    }

void cb_8080::OpPUSH_HL() 
    {
    PUSH(RegHL);
    }

void cb_8080::OpPUSH_PSW() 
    {
    cb_RegPair PSW;
    PSW.H  = RegA;
    PSW.L  = 0X02;
    PSW.L |= m_FlagS << 7;
    PSW.L |= m_FlagZ << 6;
    PSW.L |= m_FlagA << 4;
    PSW.L |= m_FlagP << 2;
    PSW.L |= m_FlagC;
    PUSH(PSW.W);
    }

void cb_8080::OpRAL() 
    {
    uint16_t Data = (RegA << 1) | m_FlagC;
    RegA    = Data & 0xff;
    m_FlagC = Data & 0x100;
    }

void cb_8080::OpRAR() 
    {
    uint16_t Data = RegA | (m_FlagC ? 0x100 : 0);
    RegA    = Data >> 1;
    m_FlagC = Data & 1;
    }

void cb_8080::OpRCond() 
    {
    if (ConditionMet()) 
        {
        POP(RegPC);
        m_IStates += c_ExtraStates;
        }
    }

void cb_8080::OpRET() 
    {
    POP(RegPC);
    }

void cb_8080::OpRLC() 
    {
    RegA    = (RegA << 1) | (RegA >> 7);
    m_FlagC = RegA & 0x01;
    }

void cb_8080::OpRRC() 
    {
    RegA = (RegA >> 1) | (RegA << 7);
    m_FlagC = RegA & 0x80;
    }

void cb_8080::OpRST() 
    {
    PUSH(RegPC);
    RegPC = m_IR & 070;
    }

void cb_8080::OpSBB_A() 
    {
    SBB(RegA);
    }

void cb_8080::OpSBB_B() 
    {
    SBB(RegB);
    }

void cb_8080::OpSBB_C() 
    {
    SBB(RegC);
    }

void cb_8080::OpSBB_D() 
    {
    SBB(RegD);
    }

void cb_8080::OpSBB_E() 
    {
    SBB(RegE);
    }

void cb_8080::OpSBB_H() 
    {
    SBB(RegH);
    }

void cb_8080::OpSBB_L() 
    {
    SBB(RegL);
    }

void cb_8080::OpSBB_M() 
    {
    SBB(m_Model->Read(RegHL));
    }

void cb_8080::OpSBI() 
    {
    SBB(m_Model->Read(RegPC++));
    }

void cb_8080::OpSHLD() 
    {
    cb_RegPair Tmp;
    Tmp.L = m_Model->Read(RegPC++);
    Tmp.H = m_Model->Read(RegPC++);
    m_Model->Write(Tmp.W  ,RegL);
    m_Model->Write(Tmp.W+1,RegH);
    }

void cb_8080::OpSPHL() 
    {
    RegSP = RegHL;
    }

void cb_8080::OpSTA() 
    {
    cb_RegPair Tmp;
    Tmp.L = m_Model->Read(RegPC++);
    Tmp.H = m_Model->Read(RegPC++);
    m_Model->Write(Tmp.W,RegA);
    }

void cb_8080::OpSTAX_B() 
    {
    m_Model->Write(RegBC,RegA);
    }

void cb_8080::OpSTAX_D() 
    {
    m_Model->Write(RegDE,RegA);
    }

void cb_8080::OpSTC() 
    {
    m_FlagC = true;
    }

void cb_8080::OpSUB_A() 
    {
    SUB(RegA);
    }

void cb_8080::OpSUB_B() 
    {
    SUB(RegB);
    }

void cb_8080::OpSUB_C() 
    {
    SUB(RegC);
    }

void cb_8080::OpSUB_D() 
    {
    SUB(RegD);
    }

void cb_8080::OpSUB_E() 
    {
    SUB(RegE);
    }

void cb_8080::OpSUB_H() 
    {
    SUB(RegH);
    }

void cb_8080::OpSUB_L() 
    {
    SUB(RegL);
    }

void cb_8080::OpSUB_M() 
    {
    SUB(m_Model->Read(RegHL));
    }

void cb_8080::OpSUI() 
    {
    SUB(m_Model->Read(RegPC++));
    }

void cb_8080::OpXCHG() 
    {
    uint16_t Tmp = RegDE;
    RegDE = RegHL;
    RegHL = Tmp;
    }

void cb_8080::OpXRA_A() 
    {
    XRA(RegA);
    }

void cb_8080::OpXRA_B() 
    {
    XRA(RegB);
    }

void cb_8080::OpXRA_C() 
    {
    XRA(RegC);
    }

void cb_8080::OpXRA_D() 
    {
    XRA(RegD);
    }

void cb_8080::OpXRA_E() 
    {
    XRA(RegE);
    }

void cb_8080::OpXRA_H() 
    {
    XRA(RegH);
    }

void cb_8080::OpXRA_L() 
    {
    XRA(RegL);
    }

void cb_8080::OpXRA_M() 
    {
    XRA(m_Model->Read(RegHL));
    }

void cb_8080::OpXRI() 
    {
    XRA(m_Model->Read(RegPC++));
    }

void cb_8080::OpXTHL() 
    {
    cb_RegPair Tmp;
    Tmp.L = m_Model->Read(RegSP++);
    Tmp.H = m_Model->Read(RegSP++);
    m_Model->Write(--RegSP,RegH);
    m_Model->Write(--RegSP,RegL);
    RegHL = Tmp.W;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// OpCode table.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const cb_8080::OpCode cb_8080::OpCodes[0x100] = 
    {
    /* 0000 */ {  4, &cb_8080::OpNOP,     "NOP",             c_Addr_Implicit   },
    /* 0001 */ { 10, &cb_8080::OpLXI_B,   "LXI   B,%04XH",   c_Addr_Immediate2 },
    /* 0002 */ {  7, &cb_8080::OpSTAX_B,  "STAX  B",         c_Addr_Implicit   },
    /* 0003 */ {  5, &cb_8080::OpINX_B,   "INX   B",         c_Addr_Implicit   },
    /* 0004 */ {  5, &cb_8080::OpINR_B,   "INR   B",         c_Addr_Implicit   },
    /* 0005 */ {  5, &cb_8080::OpDCR_B,   "DCR   B",         c_Addr_Implicit   },
    /* 0006 */ {  7, &cb_8080::OpMVI_B,   "MVI   B,%02XH",   c_Addr_Immediate1 },
    /* 0007 */ {  4, &cb_8080::OpRLC,     "RLC",             c_Addr_Implicit   },


    /* 0010 */ {  0, &cb_8080::OpIllegal, "???",             c_Addr_Implicit   },
    /* 0011 */ { 10, &cb_8080::OpDAD_B,   "DAD   B",         c_Addr_Implicit   },
    /* 0012 */ {  7, &cb_8080::OpLDAX_B,  "LDAX  B",         c_Addr_Implicit   },
    /* 0013 */ {  5, &cb_8080::OpDCX_B,   "DCX   B",         c_Addr_Implicit   },
    /* 0014 */ {  5, &cb_8080::OpINR_C,   "INR   C",         c_Addr_Implicit   },
    /* 0015 */ {  5, &cb_8080::OpDCR_C,   "DCR   C",         c_Addr_Implicit   },
    /* 0016 */ {  7, &cb_8080::OpMVI_C,   "MVI   C,%02XH",   c_Addr_Immediate1 },
    /* 0017 */ {  4, &cb_8080::OpRRC,     "RRC",             c_Addr_Implicit   },

    /* 0020 */ {  0, &cb_8080::OpIllegal, "???",             c_Addr_Implicit   },  
    /* 0021 */ { 10, &cb_8080::OpLXI_D,   "LXI   D,%04XH",   c_Addr_Immediate2 },
    /* 0022 */ {  7, &cb_8080::OpSTAX_D,  "STAX  D",         c_Addr_Implicit   },
    /* 0023 */ {  5, &cb_8080::OpINX_D,   "INX   D",         c_Addr_Implicit   },
    /* 0024 */ {  5, &cb_8080::OpINR_D,   "INR   D",         c_Addr_Implicit   },
    /* 0025 */ {  5, &cb_8080::OpDCR_D,   "DCR   D",         c_Addr_Implicit   },
    /* 0026 */ {  7, &cb_8080::OpMVI_D,   "MVI   D,%02XH",   c_Addr_Immediate1 },
    /* 0027 */ {  4, &cb_8080::OpRAL,     "RAL",             c_Addr_Implicit   },

    /* 0030 */ {  0, &cb_8080::OpIllegal, "???",             c_Addr_Implicit   },
    /* 0031 */ { 10, &cb_8080::OpDAD_D,   "DAD   D",         c_Addr_Implicit   },
    /* 0032 */ {  7, &cb_8080::OpLDAX_D,  "LDAX  D",         c_Addr_Implicit   },
    /* 0033 */ {  5, &cb_8080::OpDCX_D,   "DCX   D",         c_Addr_Implicit   },
    /* 0034 */ {  5, &cb_8080::OpINR_E,   "INR   E",         c_Addr_Implicit   },
    /* 0035 */ {  5, &cb_8080::OpDCR_E,   "DCR   E",         c_Addr_Implicit   },
    /* 0036 */ {  7, &cb_8080::OpMVI_E,   "MVI   E,%02XH",   c_Addr_Immediate1 },
    /* 0037 */ {  4, &cb_8080::OpRAR,     "RAR",             c_Addr_Implicit   },

    /* 0040 */ {  0, &cb_8080::OpIllegal, "???",             c_Addr_Implicit   },

    /* 0041 */ { 10, &cb_8080::OpLXI_H,   "LXI   H,%04XH",   c_Addr_Immediate2 },
    /* 0042 */ { 16, &cb_8080::OpSHLD,    "SHLD  %04XH",     c_Addr_Direct2    },
    /* 0043 */ {  5, &cb_8080::OpINX_H,   "INX   H",         c_Addr_Implicit   },
    /* 0044 */ {  5, &cb_8080::OpINR_H,   "INR   H",         c_Addr_Implicit   },
    /* 0045 */ {  5, &cb_8080::OpDCR_H,   "DCR   H",         c_Addr_Implicit   },
    /* 0046 */ {  7, &cb_8080::OpMVI_H,   "MVI   H,%02XH",   c_Addr_Immediate1 },
    /* 0047 */ {  4, &cb_8080::OpDAA,     "DAA",             c_Addr_Implicit   },

    /* 0050 */ {  0, &cb_8080::OpIllegal, "???",             c_Addr_Implicit   },
    /* 0051 */ { 10, &cb_8080::OpDAD_H,   "DAD   H",         c_Addr_Implicit   },
    /* 0052 */ { 16, &cb_8080::OpLHLD,    "LHLD  %04XH",     c_Addr_Direct2    },
    /* 0053 */ {  5, &cb_8080::OpDCX_H,   "DCX   H",         c_Addr_Implicit   },
    /* 0054 */ {  5, &cb_8080::OpINR_L,   "INR   L",         c_Addr_Implicit   },
    /* 0055 */ {  5, &cb_8080::OpDCR_L,   "DCR   L",         c_Addr_Implicit   },
    /* 0056 */ {  7, &cb_8080::OpMVI_L,   "MVI   L,%02XH",   c_Addr_Immediate1 },
    /* 0057 */ {  4, &cb_8080::OpCMA,     "CMA",             c_Addr_Implicit   },

    /* 0060 */ {  0, &cb_8080::OpIllegal, "???",             c_Addr_Implicit   },

    /* 0061 */ { 10, &cb_8080::OpLXI_SP,  "LXI   SP,%04XH",  c_Addr_Immediate2 },
    /* 0062 */ { 13, &cb_8080::OpSTA,     "STA   %04XH",     c_Addr_Direct2    },
    /* 0063 */ {  5, &cb_8080::OpINX_SP,  "INX   SP",        c_Addr_Implicit   },
    /* 0064 */ { 10, &cb_8080::OpINR_M,   "INR   M",         c_Addr_Implicit   },
    /* 0065 */ { 10, &cb_8080::OpDCR_M,   "DCR   M",         c_Addr_Implicit   },
    /* 0066 */ { 10, &cb_8080::OpMVI_M,   "MVI   M,%02XH",   c_Addr_Immediate1 },
    /* 0067 */ {  4, &cb_8080::OpSTC,     "STC",             c_Addr_Implicit   },

    /* 0070 */ {  0, &cb_8080::OpIllegal, "???",             c_Addr_Implicit   },
    /* 0071 */ { 10, &cb_8080::OpDAD_SP,  "DAD   SP",        c_Addr_Implicit   },
    /* 0072 */ { 13, &cb_8080::OpLDA,     "LDA   %04XH",     c_Addr_Direct2    },
    /* 0073 */ {  5, &cb_8080::OpDCX_SP,  "DCX   SP",        c_Addr_Implicit   },
    /* 0074 */ {  5, &cb_8080::OpINR_A,   "INR   A",         c_Addr_Implicit   },
    /* 0075 */ {  5, &cb_8080::OpDCR_A,   "DCR   A",         c_Addr_Implicit   },
    /* 0076 */ {  7, &cb_8080::OpMVI_A,   "MVI   A,%02XH",   c_Addr_Immediate1 },
    /* 0077 */ {  4, &cb_8080::OpCMC,     "CMC",             c_Addr_Implicit   },

    /* 0100 */ {  5, &cb_8080::OpMOV_B_B, "MOV   B,B",       c_Addr_Implicit   },
    /* 0101 */ {  5, &cb_8080::OpMOV_B_C, "MOV   B,C",       c_Addr_Implicit   },
    /* 0102 */ {  5, &cb_8080::OpMOV_B_D, "MOV   B,D",       c_Addr_Implicit   },
    /* 0103 */ {  5, &cb_8080::OpMOV_B_E, "MOV   B,E",       c_Addr_Implicit   },
    /* 0104 */ {  5, &cb_8080::OpMOV_B_H, "MOV   B,H",       c_Addr_Implicit   },
    /* 0105 */ {  5, &cb_8080::OpMOV_B_L, "MOV   B,L",       c_Addr_Implicit   },
    /* 0106 */ {  7, &cb_8080::OpMOV_B_M, "MOV   B,M",       c_Addr_Implicit   },
    /* 0107 */ {  5, &cb_8080::OpMOV_B_A, "MOV   B,A",       c_Addr_Implicit   },

    /* 0110 */ {  5, &cb_8080::OpMOV_C_B, "MOV   C,B",       c_Addr_Implicit   },
    /* 0111 */ {  5, &cb_8080::OpMOV_C_C, "MOV   C,C",       c_Addr_Implicit   },
    /* 0112 */ {  5, &cb_8080::OpMOV_C_D, "MOV   C,D",       c_Addr_Implicit   },
    /* 0113 */ {  5, &cb_8080::OpMOV_C_E, "MOV   C,E",       c_Addr_Implicit   },
    /* 0114 */ {  5, &cb_8080::OpMOV_C_H, "MOV   C,H",       c_Addr_Implicit   },
    /* 0115 */ {  5, &cb_8080::OpMOV_C_L, "MOV   C,L",       c_Addr_Implicit   },
    /* 0116 */ {  7, &cb_8080::OpMOV_C_M, "MOV   C,M",       c_Addr_Implicit   },
    /* 0117 */ {  5, &cb_8080::OpMOV_C_A, "MOV   C,A",       c_Addr_Implicit   },

    /* 0120 */ {  5, &cb_8080::OpMOV_D_B, "MOV   D,B",       c_Addr_Implicit   },
    /* 0121 */ {  5, &cb_8080::OpMOV_D_C, "MOV   D,C",       c_Addr_Implicit   },
    /* 0122 */ {  5, &cb_8080::OpMOV_D_D, "MOV   D,D",       c_Addr_Implicit   },
    /* 0123 */ {  5, &cb_8080::OpMOV_D_E, "MOV   D,E",       c_Addr_Implicit   },
    /* 0124 */ {  5, &cb_8080::OpMOV_D_H, "MOV   D,H",       c_Addr_Implicit   },
    /* 0125 */ {  5, &cb_8080::OpMOV_D_L, "MOV   D,L",       c_Addr_Implicit   },
    /* 0126 */ {  7, &cb_8080::OpMOV_D_M, "MOV   D,M",       c_Addr_Implicit   },
    /* 0127 */ {  5, &cb_8080::OpMOV_D_A, "MOV   D,A",       c_Addr_Implicit   },

    /* 0130 */ {  5, &cb_8080::OpMOV_E_B, "MOV   E,B",       c_Addr_Implicit   },
    /* 0131 */ {  5, &cb_8080::OpMOV_E_C, "MOV   E,C",       c_Addr_Implicit   },
    /* 0132 */ {  5, &cb_8080::OpMOV_E_D, "MOV   E,D",       c_Addr_Implicit   },
    /* 0133 */ {  5, &cb_8080::OpMOV_E_E, "MOV   E,E",       c_Addr_Implicit   },
    /* 0134 */ {  5, &cb_8080::OpMOV_E_H, "MOV   E,H",       c_Addr_Implicit   },
    /* 0135 */ {  5, &cb_8080::OpMOV_E_L, "MOV   E,L",       c_Addr_Implicit   },
    /* 0136 */ {  7, &cb_8080::OpMOV_E_M, "MOV   E,M",       c_Addr_Implicit   },
    /* 0137 */ {  5, &cb_8080::OpMOV_E_A, "MOV   E,A",       c_Addr_Implicit   },

    /* 0140 */ {  5, &cb_8080::OpMOV_H_B, "MOV   H,B",       c_Addr_Implicit   },
    /* 0141 */ {  5, &cb_8080::OpMOV_H_C, "MOV   H,C",       c_Addr_Implicit   },
    /* 0142 */ {  5, &cb_8080::OpMOV_H_D, "MOV   H,D",       c_Addr_Implicit   },
    /* 0143 */ {  5, &cb_8080::OpMOV_H_E, "MOV   H,E",       c_Addr_Implicit   },
    /* 0144 */ {  5, &cb_8080::OpMOV_H_H, "MOV   H,H",       c_Addr_Implicit   },
    /* 0145 */ {  5, &cb_8080::OpMOV_H_L, "MOV   H,L",       c_Addr_Implicit   },
    /* 0146 */ {  7, &cb_8080::OpMOV_H_M, "MOV   H,M",       c_Addr_Implicit   },
    /* 0147 */ {  5, &cb_8080::OpMOV_H_A, "MOV   H,A",       c_Addr_Implicit   },

    /* 0150 */ {  5, &cb_8080::OpMOV_L_B, "MOV   L,B",       c_Addr_Implicit   },
    /* 0151 */ {  5, &cb_8080::OpMOV_L_C, "MOV   L,C",       c_Addr_Implicit   },
    /* 0152 */ {  5, &cb_8080::OpMOV_L_D, "MOV   L,D",       c_Addr_Implicit   },
    /* 0153 */ {  5, &cb_8080::OpMOV_L_E, "MOV   L,E",       c_Addr_Implicit   },
    /* 0154 */ {  5, &cb_8080::OpMOV_L_H, "MOV   L,H",       c_Addr_Implicit   },
    /* 0155 */ {  5, &cb_8080::OpMOV_L_L, "MOV   L,L",       c_Addr_Implicit   },
    /* 0156 */ {  7, &cb_8080::OpMOV_L_M, "MOV   L,M",       c_Addr_Implicit   },
    /* 0157 */ {  5, &cb_8080::OpMOV_L_A, "MOV   L,A",       c_Addr_Implicit   },

    /* 0160 */ {  7, &cb_8080::OpMOV_M_B, "MOV   M,B",       c_Addr_Implicit   },
    /* 0161 */ {  7, &cb_8080::OpMOV_M_C, "MOV   M,C",       c_Addr_Implicit   },
    /* 0162 */ {  7, &cb_8080::OpMOV_M_D, "MOV   M,D",       c_Addr_Implicit   },
    /* 0163 */ {  7, &cb_8080::OpMOV_M_E, "MOV   M,E",       c_Addr_Implicit   },
    /* 0164 */ {  7, &cb_8080::OpMOV_M_H, "MOV   M,H",       c_Addr_Implicit   },
    /* 0165 */ {  7, &cb_8080::OpMOV_M_L, "MOV   M,L",       c_Addr_Implicit   },
    /* 0166 */ {  7, &cb_8080::OpHLT,     "HLT",             c_Addr_Implicit   },
    /* 0167 */ {  7, &cb_8080::OpMOV_M_A, "MOV   M,A",       c_Addr_Implicit   },

    /* 0170 */ {  5, &cb_8080::OpMOV_A_B, "MOV   A,B",       c_Addr_Implicit   },
    /* 0171 */ {  5, &cb_8080::OpMOV_A_C, "MOV   A,C",       c_Addr_Implicit   },
    /* 0172 */ {  5, &cb_8080::OpMOV_A_D, "MOV   A,D",       c_Addr_Implicit   },
    /* 0173 */ {  5, &cb_8080::OpMOV_A_E, "MOV   A,E",       c_Addr_Implicit   },
    /* 0174 */ {  5, &cb_8080::OpMOV_A_H, "MOV   A,H",       c_Addr_Implicit   },
    /* 0175 */ {  5, &cb_8080::OpMOV_A_L, "MOV   A,L",       c_Addr_Implicit   },
    /* 0176 */ {  7, &cb_8080::OpMOV_A_M, "MOV   A,M",       c_Addr_Implicit   },
    /* 0177 */ {  5, &cb_8080::OpMOV_A_A, "MOV   A,A",       c_Addr_Implicit   },

    /* 0200 */ {  4, &cb_8080::OpADD_B,   "ADD   B",         c_Addr_Implicit   },
    /* 0201 */ {  4, &cb_8080::OpADD_C,   "ADD   C",         c_Addr_Implicit   },
    /* 0202 */ {  4, &cb_8080::OpADD_D,   "ADD   D",         c_Addr_Implicit   },
    /* 0203 */ {  4, &cb_8080::OpADD_E,   "ADD   E",         c_Addr_Implicit   },
    /* 0204 */ {  4, &cb_8080::OpADD_H,   "ADD   H",         c_Addr_Implicit   },
    /* 0205 */ {  4, &cb_8080::OpADD_L,   "ADD   L",         c_Addr_Implicit   },
    /* 0206 */ {  7, &cb_8080::OpADD_M,   "ADD   M",         c_Addr_Implicit   },
    /* 0207 */ {  4, &cb_8080::OpADD_A,   "ADD   A",         c_Addr_Implicit   },

    /* 0210 */ {  4, &cb_8080::OpADC_B,   "ADC   B",         c_Addr_Implicit   },
    /* 0211 */ {  4, &cb_8080::OpADC_C,   "ADC   C",         c_Addr_Implicit   },
    /* 0212 */ {  4, &cb_8080::OpADC_D,   "ADC   D",         c_Addr_Implicit   },
    /* 0213 */ {  4, &cb_8080::OpADC_E,   "ADC   E",         c_Addr_Implicit   },
    /* 0214 */ {  4, &cb_8080::OpADC_H,   "ADC   H",         c_Addr_Implicit   },
    /* 0215 */ {  4, &cb_8080::OpADC_L,   "ADC   L",         c_Addr_Implicit   },
    /* 0216 */ {  7, &cb_8080::OpADC_M,   "ADC   M",         c_Addr_Implicit   },
    /* 0217 */ {  4, &cb_8080::OpADC_A,   "ADC   A",         c_Addr_Implicit   },

    /* 0220 */ {  4, &cb_8080::OpSUB_B,   "SUB   B",         c_Addr_Implicit   },
    /* 0221 */ {  4, &cb_8080::OpSUB_C,   "SUB   C",         c_Addr_Implicit   },
    /* 0222 */ {  4, &cb_8080::OpSUB_D,   "SUB   D",         c_Addr_Implicit   },
    /* 0223 */ {  4, &cb_8080::OpSUB_E,   "SUB   E",         c_Addr_Implicit   },
    /* 0224 */ {  4, &cb_8080::OpSUB_H,   "SUB   H",         c_Addr_Implicit   },
    /* 0225 */ {  4, &cb_8080::OpSUB_L,   "SUB   L",         c_Addr_Implicit   },
    /* 0226 */ {  7, &cb_8080::OpSUB_M,   "SUB   M",         c_Addr_Implicit   },
    /* 0227 */ {  4, &cb_8080::OpSUB_A,   "SUB   A",         c_Addr_Implicit   },

    /* 0230 */ {  4, &cb_8080::OpSBB_B,   "SBB   B",         c_Addr_Implicit   },
    /* 0231 */ {  4, &cb_8080::OpSBB_C,   "SBB   C",         c_Addr_Implicit   },
    /* 0232 */ {  4, &cb_8080::OpSBB_D,   "SBB   D",         c_Addr_Implicit   },
    /* 0233 */ {  4, &cb_8080::OpSBB_E,   "SBB   E",         c_Addr_Implicit   },
    /* 0234 */ {  4, &cb_8080::OpSBB_H,   "SBB   H",         c_Addr_Implicit   },
    /* 0235 */ {  4, &cb_8080::OpSBB_L,   "SBB   L",         c_Addr_Implicit   },
    /* 0236 */ {  7, &cb_8080::OpSBB_M,   "SBB   M",         c_Addr_Implicit   },
    /* 0237 */ {  4, &cb_8080::OpSBB_A,   "SBB   A",         c_Addr_Implicit   },

    /* 0240 */ {  4, &cb_8080::OpANA_B,   "ANA   B",         c_Addr_Implicit   },
    /* 0241 */ {  4, &cb_8080::OpANA_C,   "ANA   C",         c_Addr_Implicit   },
    /* 0242 */ {  4, &cb_8080::OpANA_D,   "ANA   D",         c_Addr_Implicit   },
    /* 0243 */ {  4, &cb_8080::OpANA_E,   "ANA   E",         c_Addr_Implicit   },
    /* 0244 */ {  4, &cb_8080::OpANA_H,   "ANA   H",         c_Addr_Implicit   },
    /* 0245 */ {  4, &cb_8080::OpANA_L,   "ANA   L",         c_Addr_Implicit   },
    /* 0246 */ {  7, &cb_8080::OpANA_M,   "ANA   M",         c_Addr_Implicit   },
    /* 0247 */ {  4, &cb_8080::OpANA_A,   "ANA   A",         c_Addr_Implicit   },

    /* 0250 */ {  4, &cb_8080::OpXRA_B,   "XRA   B",         c_Addr_Implicit   },
    /* 0251 */ {  4, &cb_8080::OpXRA_C,   "XRA   C",         c_Addr_Implicit   },
    /* 0252 */ {  4, &cb_8080::OpXRA_D,   "XRA   D",         c_Addr_Implicit   },
    /* 0253 */ {  4, &cb_8080::OpXRA_E,   "XRA   E",         c_Addr_Implicit   },
    /* 0254 */ {  4, &cb_8080::OpXRA_H,   "XRA   H",         c_Addr_Implicit   },
    /* 0255 */ {  4, &cb_8080::OpXRA_L,   "XRA   L",         c_Addr_Implicit   },
    /* 0256 */ {  7, &cb_8080::OpXRA_M,   "XRA   M",         c_Addr_Implicit   },
    /* 0257 */ {  4, &cb_8080::OpXRA_A,   "XRA   A",         c_Addr_Implicit   },

    /* 0260 */ {  4, &cb_8080::OpORA_B,   "ORA   B",         c_Addr_Implicit   },
    /* 0261 */ {  4, &cb_8080::OpORA_C,   "ORA   C",         c_Addr_Implicit   },
    /* 0262 */ {  4, &cb_8080::OpORA_D,   "ORA   D",         c_Addr_Implicit   },
    /* 0263 */ {  4, &cb_8080::OpORA_E,   "ORA   E",         c_Addr_Implicit   },
    /* 0264 */ {  4, &cb_8080::OpORA_H,   "ORA   H",         c_Addr_Implicit   },
    /* 0265 */ {  4, &cb_8080::OpORA_L,   "ORA   L",         c_Addr_Implicit   },
    /* 0266 */ {  7, &cb_8080::OpORA_M,   "ORA   M",         c_Addr_Implicit   },
    /* 0267 */ {  4, &cb_8080::OpORA_A,   "ORA   A",         c_Addr_Implicit   },

    /* 0270 */ {  4, &cb_8080::OpCMP_B,   "CMP   B",         c_Addr_Implicit   },
    /* 0271 */ {  4, &cb_8080::OpCMP_C,   "CMP   C",         c_Addr_Implicit   },
    /* 0272 */ {  4, &cb_8080::OpCMP_D,   "CMP   D",         c_Addr_Implicit   },
    /* 0273 */ {  4, &cb_8080::OpCMP_E,   "CMP   E",         c_Addr_Implicit   },
    /* 0274 */ {  4, &cb_8080::OpCMP_H,   "CMP   H",         c_Addr_Implicit   },
    /* 0275 */ {  4, &cb_8080::OpCMP_L,   "CMP   L",         c_Addr_Implicit   },
    /* 0276 */ {  7, &cb_8080::OpCMP_M,   "CMP   M",         c_Addr_Implicit   },
    /* 0277 */ {  4, &cb_8080::OpCMP_A,   "CMP   A",         c_Addr_Implicit   },

    /* 0300 */ {  5, &cb_8080::OpRCond,   "RNZ",             c_Addr_Implicit   },
    /* 0301 */ { 10, &cb_8080::OpPOP_BC,  "POP   B",         c_Addr_Implicit   },
    /* 0302 */ { 10, &cb_8080::OpJCond,   "JNZ   %04XH",     c_Addr_Direct2    },
    /* 0303 */ { 10, &cb_8080::OpJMP,     "JMP   %04XH",     c_Addr_Direct2    },
    /* 0304 */ { 10, &cb_8080::OpCCond,   "CNZ   %04XH",     c_Addr_Direct2    }, 
    /* 0305 */ { 11, &cb_8080::OpPUSH_BC, "PUSH  B",         c_Addr_Implicit   },
    /* 0306 */ {  7, &cb_8080::OpADI,     "ADI   %02XH",     c_Addr_Immediate1 },
    /* 0307 */ { 11, &cb_8080::OpRST,     "RST   0",         c_Addr_Implicit   },

    /* 0310 */ {  5, &cb_8080::OpRCond,   "RZ",              c_Addr_Implicit   },
    /* 0311 */ { 10, &cb_8080::OpRET,     "RET",             c_Addr_Implicit   },
    /* 0312 */ { 10, &cb_8080::OpJCond,   "JZ    %04XH",     c_Addr_Direct2    },
    /* 0313 */ {  0, &cb_8080::OpIllegal, "???",             c_Addr_Implicit   },  
    /* 0314 */ { 10, &cb_8080::OpCCond,   "CZ    %04XH",     c_Addr_Direct2    },
    /* 0315 */ { 17, &cb_8080::OpCALL,    "CALL  %04XH",     c_Addr_Direct2    },
    /* 0316 */ {  7, &cb_8080::OpACI,     "ACI   %02XH",     c_Addr_Immediate1 },
    /* 0317 */ { 11, &cb_8080::OpRST,     "RST   1",         c_Addr_Implicit   },

    /* 0320 */ {  5, &cb_8080::OpRCond,   "RNC",             c_Addr_Implicit   },
    /* 0321 */ { 10, &cb_8080::OpPOP_DE,  "POP   D",         c_Addr_Implicit   },
    /* 0322 */ { 10, &cb_8080::OpJCond,   "JNC   %04XH",     c_Addr_Direct2    },
    /* 0323 */ { 10, &cb_8080::OpOUT,     "OUT   %02XH",     c_Addr_Direct1    },
    /* 0324 */ { 10, &cb_8080::OpCCond,   "CNC   %04XH",     c_Addr_Direct2    },
    /* 0325 */ { 11, &cb_8080::OpPUSH_DE, "PUSH  D",         c_Addr_Implicit   },
    /* 0326 */ {  7, &cb_8080::OpSUI,     "SUI   %02XH",     c_Addr_Immediate1 },
    /* 0327 */ { 11, &cb_8080::OpRST,     "RST   2",         c_Addr_Implicit   },

    /* 0330 */ {  5, &cb_8080::OpRCond,   "RC",              c_Addr_Implicit   },
    /* 0331 */ {  0, &cb_8080::OpIllegal, "???",             c_Addr_Implicit   },
    /* 0332 */ { 10, &cb_8080::OpJCond,   "JC    %04XH",     c_Addr_Direct2    },
    /* 0333 */ { 10, &cb_8080::OpIN,      "IN    %02XH",     c_Addr_Direct1    },
    /* 0334 */ { 10, &cb_8080::OpCCond,   "CC    %04XH",     c_Addr_Direct2    }, 
    /* 0335 */ {  0, &cb_8080::OpIllegal, "???",             c_Addr_Implicit   },
    /* 0336 */ {  7, &cb_8080::OpSBI,     "SBI   %02XH",     c_Addr_Immediate1 },
    /* 0337 */ { 11, &cb_8080::OpRST,     "RST   3",         c_Addr_Implicit   },

    /* 0340 */ {  5, &cb_8080::OpRCond,   "RPO",             c_Addr_Implicit   },
    /* 0341 */ { 10, &cb_8080::OpPOP_HL,  "POP   H",         c_Addr_Implicit   },
    /* 0342 */ { 10, &cb_8080::OpJCond,   "JPO   %04XH",     c_Addr_Direct2    },
    /* 0343 */ { 18, &cb_8080::OpXTHL,    "XTHL",            c_Addr_Implicit   },
    /* 0344 */ { 10, &cb_8080::OpCCond,   "CPO   %04XH",     c_Addr_Direct2    },
    /* 0345 */ { 11, &cb_8080::OpPUSH_HL, "PUSH  H",         c_Addr_Implicit   },
    /* 0346 */ {  7, &cb_8080::OpANI,     "ANI   %02XH",     c_Addr_Immediate1 },
    /* 0347 */ { 11, &cb_8080::OpRST,     "RST   4",         c_Addr_Implicit   },

    /* 0350 */ {  5, &cb_8080::OpRCond,   "RPE",             c_Addr_Implicit   },
    /* 0351 */ {  5, &cb_8080::OpPCHL,    "PCHL",            c_Addr_Implicit   },
    /* 0352 */ { 10, &cb_8080::OpJCond,   "JPE   %04XH",     c_Addr_Direct2    },
    /* 0353 */ {  4, &cb_8080::OpXCHG,    "XCHG",            c_Addr_Implicit   },
    /* 0354 */ { 10, &cb_8080::OpCCond,   "CPE   %04XH",     c_Addr_Direct2    },
    /* 0355 */ {  0, &cb_8080::OpIllegal, "???",             c_Addr_Implicit   },
    /* 0356 */ {  7, &cb_8080::OpXRI,     "XRI   %04XH",     c_Addr_Immediate1 },
    /* 0357 */ { 11, &cb_8080::OpRST,     "RST   5",         c_Addr_Implicit   },

    /* 0360 */ {  5, &cb_8080::OpRCond,   "RP",              c_Addr_Implicit   },
    /* 0361 */ { 10, &cb_8080::OpPOP_PSW, "POP   PSW",       c_Addr_Implicit   },
    /* 0362 */ { 10, &cb_8080::OpJCond,   "JP    %04XH",     c_Addr_Direct2    },
    /* 0363 */ {  4, &cb_8080::OpDI,      "DI",              c_Addr_Implicit   },
    /* 0364 */ { 10, &cb_8080::OpCCond,   "CP    %04XH",     c_Addr_Direct2    },
    /* 0365 */ { 11, &cb_8080::OpPUSH_PSW,"PUSH  PSW",       c_Addr_Implicit   },
    /* 0366 */ {  7, &cb_8080::OpORI,     "ORI   %02XH",     c_Addr_Immediate1 },
    /* 0367 */ { 11, &cb_8080::OpRST,     "RST   6",         c_Addr_Implicit   },

    /* 0370 */ {  5, &cb_8080::OpRCond,   "RM",              c_Addr_Implicit   },
    /* 0371 */ {  5, &cb_8080::OpSPHL,    "SPHL",            c_Addr_Implicit   },
    /* 0372 */ { 10, &cb_8080::OpJCond,   "JM    %04XH",     c_Addr_Direct2    },
    /* 0373 */ {  4, &cb_8080::OpEI,      "EI",              c_Addr_Implicit   },
    /* 0374 */ { 10, &cb_8080::OpCCond,   "CM    %04XH",     c_Addr_Direct2    },
    /* 0375 */ {  0, &cb_8080::OpIllegal, "???",             c_Addr_Implicit   },
    /* 0376 */ {  7, &cb_8080::OpCPI,     "CPI   %02XH",     c_Addr_Immediate1 },
    /* 0377 */ { 11, &cb_8080::OpRST,     "RST   7",         c_Addr_Implicit   }
    };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
