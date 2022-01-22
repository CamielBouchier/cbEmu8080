//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <cstdint>
#include <QtCore>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// A union for 8b/16b register combination.
//
// XXX CB BE AWARE : this is relying on endianness ! I *assume* that
// L/H inversion on endianness that is different from x86 should be fine.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef union 
    {
    struct 
        {
        uint8_t L;
        uint8_t H;
        };
    uint16_t W;
    } cb_RegPair;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// The big chunk of the class definition.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class cb_8080 
    {
    public:

	    // The registers and flags mostly as pairs.
	    cb_RegPair  m_BC;
	    cb_RegPair  m_DE;
	    cb_RegPair  m_HL;
	    cb_RegPair  m_SP;
	    cb_RegPair  m_PC;
	    uint8_t     m_A;
	    uint8_t     m_IR; // Instruction register.
	
	    bool        m_FlagZ; // Zero
	    bool        m_FlagC; // Carry
	    bool        m_FlagA; // AuxCarry
	    bool        m_FlagP; // Parity
	    bool        m_FlagS; // Sign
	
	    bool        m_InterruptEnabled;
	
		// Macro's to access the registers in convenient ways.
		#define     RegBC  m_BC.W
		#define     RegDE  m_DE.W
		#define     RegHL  m_HL.W
		#define     RegSP  m_SP.W
		#define     RegPC  m_PC.W
		#define     RegA   m_A
		#define     RegB   m_BC.H
		#define     RegC   m_BC.L
		#define     RegD   m_DE.H
		#define     RegE   m_DE.L
		#define     RegH   m_HL.H
		#define     RegL   m_HL.L
		#define     RegSPH m_SP.H
		#define     RegSPL m_SP.H
		#define     RegPCH m_PC.H
		#define     RegPCL m_PC.L
		
		bool        m_Halted;
		int         m_ProcessorState;
		int         m_IStates;	// Number of states in an instruction
	
		QString     m_Disassembled;
		
		// Constructor-Destructor.
		cb_8080(class cb_Model *Model);
		~cb_8080();
		
        //
        void clock_tick();

		// Reset
		void Reset();

        //
        QString m_TraceFileName;
        bool    m_DoTrace;
        void SetDoTrace(const bool DoTrace);
        void FlushTrace();
		
		// Addressing modes.
		static const int c_Addr_Implicit   = 0;
		static const int c_Addr_Immediate1 = 1;
		static const int c_Addr_Immediate2 = 2;
		static const int c_Addr_Direct1    = 3;
		static const int c_Addr_Direct2    = 4;
		
		// Additional states in conditional CALL/RET
		static const int c_ExtraStates = 6;
		
		// Processor states
		static const int c_StateFetch  = 0;
		static const int c_StateDecode = 1;

	    // Disassemble 
	    QString Disassemble(const uint16_t Address);
		
	private:

	    class cb_Model* m_Model;
	
	    FILE*       m_TraceFile;
	    QTextStream m_TraceStream;
	
	    // Evaluator of conditional stuff.
	    bool ConditionMet();
	
		// The routines per opcode (group).
		void OpIllegal(); // An illegal catcher.
		void OpACI();
		void OpADC_A();
		void OpADC_B();
		void OpADC_C();
		void OpADC_D();
		void OpADC_E();
		void OpADC_H();
		void OpADC_L();
		void OpADC_M();
		void OpADD_A();
		void OpADD_B();
		void OpADD_C();
		void OpADD_D();
		void OpADD_E();
		void OpADD_H();
		void OpADD_L();
		void OpADD_M();
		void OpADI();
		void OpANA_A();
		void OpANA_B();
		void OpANA_C();
		void OpANA_D();
		void OpANA_E();
		void OpANA_H();
		void OpANA_L();
		void OpANA_M();
		void OpANI();
		void OpCALL();
		void OpCCond(); // Conditional call group.
		void OpCMA();
		void OpCMC();
		void OpCMP_A();
		void OpCMP_B();
		void OpCMP_C();
		void OpCMP_D();
		void OpCMP_E();
		void OpCMP_H();
		void OpCMP_L();
		void OpCMP_M();
		void OpCPI();
		void OpDAA();
		void OpDAD_B();
		void OpDAD_D();
		void OpDAD_H();
		void OpDAD_SP();
		void OpDCR_A();
		void OpDCR_B();
		void OpDCR_C();
		void OpDCR_D();
		void OpDCR_E();
		void OpDCR_H();
		void OpDCR_L();
		void OpDCR_M();
		void OpDCX_B();
		void OpDCX_D();
		void OpDCX_H();
		void OpDCX_SP();
		void OpDI();
		void OpEI();
		void OpHLT();
		void OpIN();
		void OpINR_A();
		void OpINR_B();
		void OpINR_C();
		void OpINR_D();
		void OpINR_E();
		void OpINR_H();
		void OpINR_L();
		void OpINR_M();
		void OpINX_B();
		void OpINX_D();
		void OpINX_H();
		void OpINX_SP();
		void OpJCond(); // Conditional jump group.
		void OpJMP();
		void OpLDA();
		void OpLDAX_B();
		void OpLDAX_D();
		void OpLHLD();
		void OpLXI_B();
		void OpLXI_D();
		void OpLXI_H();
		void OpLXI_SP();
		void OpMOV_A_A();
		void OpMOV_A_B();
		void OpMOV_A_C();
		void OpMOV_A_D();
		void OpMOV_A_E();
		void OpMOV_A_H();
		void OpMOV_A_L();
		void OpMOV_A_M();
		void OpMOV_B_A();
		void OpMOV_B_B();
		void OpMOV_B_C();
		void OpMOV_B_D();
		void OpMOV_B_E();
		void OpMOV_B_H();
		void OpMOV_B_L();
		void OpMOV_B_M();
		void OpMOV_C_A();
		void OpMOV_C_B();
		void OpMOV_C_C();
		void OpMOV_C_D();
		void OpMOV_C_E();
		void OpMOV_C_H();
		void OpMOV_C_L();
		void OpMOV_C_M();
		void OpMOV_D_A();
		void OpMOV_D_B();
		void OpMOV_D_C();
		void OpMOV_D_D();
		void OpMOV_D_E();
		void OpMOV_D_H();
		void OpMOV_D_L();
		void OpMOV_D_M();
		void OpMOV_E_A();
		void OpMOV_E_B();
		void OpMOV_E_C();
		void OpMOV_E_D();
		void OpMOV_E_E();
		void OpMOV_E_H();
		void OpMOV_E_L();
		void OpMOV_E_M();
		void OpMOV_H_A();
		void OpMOV_H_B();
		void OpMOV_H_C();
		void OpMOV_H_D();
		void OpMOV_H_E();
		void OpMOV_H_H();
		void OpMOV_H_L();
		void OpMOV_H_M();
		void OpMOV_L_A();
		void OpMOV_L_B();
		void OpMOV_L_C();
		void OpMOV_L_D();
		void OpMOV_L_E();
		void OpMOV_L_H();
		void OpMOV_L_L();
		void OpMOV_L_M();
		void OpMOV_M_A();
		void OpMOV_M_B();
		void OpMOV_M_C();
		void OpMOV_M_D();
		void OpMOV_M_E();
		void OpMOV_M_H();
		void OpMOV_M_L();
		void OpMVI_A();
		void OpMVI_B();
		void OpMVI_C();
		void OpMVI_D();
		void OpMVI_E();
		void OpMVI_H();
		void OpMVI_L();
		void OpMVI_M();
		void OpNOP();
		void OpORA_A();
		void OpORA_B();
		void OpORA_C();
		void OpORA_D();
		void OpORA_E();
		void OpORA_H();
		void OpORA_L();
		void OpORA_M();
		void OpORI();
		void OpOUT();
		void OpPCHL();
		void OpPOP_BC();
		void OpPOP_DE();
		void OpPOP_HL();
		void OpPOP_PSW();
		void OpPUSH_BC();
		void OpPUSH_DE();
		void OpPUSH_HL();
		void OpPUSH_PSW();
		void OpRAL();
		void OpRAR();
		void OpRCond();
		void OpRET();
		void OpRLC();
		void OpRRC();
		void OpRST();
		void OpSBB_A();
		void OpSBB_B();
		void OpSBB_C();
		void OpSBB_D();
		void OpSBB_E();
		void OpSBB_H();
		void OpSBB_L();
		void OpSBB_M();
		void OpSBI();
		void OpSHLD();
		void OpSPHL();
		void OpSTA();
		void OpSTAX_B();
		void OpSTAX_D();
		void OpSTC();
		void OpSUB_A();
		void OpSUB_B();
		void OpSUB_C();
		void OpSUB_D();
		void OpSUB_E();
		void OpSUB_H();
		void OpSUB_L();
		void OpSUB_M();
		void OpSUI();
		void OpXCHG();
		void OpXRA_A();
		void OpXRA_B();
		void OpXRA_C();
		void OpXRA_D();
		void OpXRA_E();
		void OpXRA_H();
		void OpXRA_L();
		void OpXRA_M();
		void OpXRI();
		void OpXTHL();
	
	    // A structure definition and associated array to 
	    // describe the OpCodes.
	
	    struct OpCode 
            {
	        short NrStates;
	        void  (cb_8080::*OpFunction)();
	        const char* Mnemonic;
	        short AddrMode;
	        };
	    static const OpCode OpCodes[0x100];
	
	    // Half carry and parity stuff.
	    static const bool HalfCarry[];
	    static const bool SubHalfCarry[];
	    static const bool EvenParity[];

    };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
