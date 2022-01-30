//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <QtCore>
#include <QtGui>

#include "cbDefines.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// IO module for a VT terminal.
//
// Programming interface. (I/O space).
//
// 0 : Write the byte to console.
// 1 : Returns 0xFF if key is available, 0x00 otherwise.
// 2 : Returns the key. Code TBD, starting ASCII.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define ATTR_DEFAULT       0X0098
#define ATTR_ERASE_MASK    0X00FF /* Resets all but color */
#define ATTR_BIT_BOLD      0X0100
#define ATTR_BIT_BLINK     0X0200
#define ATTR_BIT_ITALIC    0X0400 // Note : not supported on DEC based fonts.
#define ATTR_BIT_UNDERLINE 0X0800
#define ATTR_BIT_DIM       0X1000
#define ATTR_BIT_INVISIBLE 0X2000
#define ATTR_BIT_NEGATIVE  0X4000
#define ATTR_BIT_DECSCA    0X8000

struct cbScreenCell 
    {
    uint16_t Char;   // Encoded as (Set<<8)|Byte
    // XNiDUIbB BBBBFFFF 
    // (Negative,invisible,Dim,Underline,Italic,blink,Bold,Background,Foreground).
    // Colors : 0 = black, 1 = red, 2 = green, 3 = yellow, 4 = blue,
    //          5 = magenta, 6 = cyan, 7 = white. 
    //          8 = console default FG.
    //          9 = console default BG.
    // X : DECSCA_Eraseable bit.
    uint16_t Attr; 
    };

// Per line : double width/double hight.
struct cbDWDH 
    {
    bool DW; // Double Width
    bool DH; // Double Height
    bool Bt; // Bottom : Only relevant if DH
    };

// In a screen character. 
const int c_RowsPerChar         = 20;
const int c_ColsPerChar         = 10;
// In a glyph description.
const int c_RowsPerGlyph_Host   = 20;
const int c_ColsPerGlyph_Host   = 10;
const int c_RowsPerGlyph_Vt220  = 10;
const int c_ColsPerGlyph_Vt220  = 8;
// Character sets.
const int c_NrHardCharSets      = 16;
const int c_NrSoftCharSets      = 4;
const int c_NrCharsPerSet       = 96;
// Font modes.
const int c_FontMode_Vt220      = 0;
const int c_FontMode_Host       = 1;
// Font styles.
const int c_NrFontStyles     = 3;
const int c_FontStyle_Normal = 0;
const int c_FontStyle_Bold   = 1;
const int c_FontStyle_Italic = 2;

// And a charset description.
struct cbCharSet 
    {
    // For each set, the unicode charachter.
    uint16_t Uni16Map[c_NrCharsPerSet];
    // (uint16_t for compatibility with native wider fonts)
    uint16_t Vt220Glyphs[c_NrCharsPerSet][c_RowsPerGlyph_Vt220];
    // Host glyphs for each set. 
    uint16_t HostGlyphs[c_NrFontStyles][c_NrCharsPerSet][c_RowsPerGlyph_Host];
    };

class cbConsole : public QObject 
    {
    Q_OBJECT
    
    public:
    
        cbConsole();
        ~cbConsole();
        
        void Reset(const bool Hard = true);
        
        inline bool IsLNM() { return m_LNM; };

        void    Write(const uint8_t Address, const uint8_t Value);
        uint8_t Read (const uint8_t Address);

        void    OnBlink();
        void    OnUpdateConsole();
        void    OnSizeChange(const int NrRows, const int NrCols);
        void    OnFontModeVt220Changed(const int Check);
        void    OnBlankInterlineChanged(const int Check);
        void    OnIntensityChanged(const int Value);   // 0..100
        void    OnUnderlineRowChanged(const int Check);
        void    OnColorSchemeChanged(const int Scheme);
        void    OnTest();
    
    signals : 
    
        void    SignalWrite(const uint8_t Address, const uint8_t Value);

    private :
    
        static const uint16_t c_Uni16Maps[c_NrHardCharSets][c_NrCharsPerSet];
        
        // Character set name to index.
        QHash <QByteArray,int> m_CharSetNameIdxMap;
        
        cbCharSet m_CharSets[c_NrHardCharSets + c_NrSoftCharSets];
        
        // Character set encodings.
        QHash <uint16_t,uint8_t> m_KeyMap; // Inverse of above for keyboard maps.
        
        QColor          m_Background;
        QColor          m_Foreground;
        int             m_ColorScheme;
        bool            m_CursorPhaseOn;
        //bool          m_TestingConsole;
        bool            m_BlankInterline; 
        int             m_Intensity;
        int             m_UnderlineRow;  // Within glyph : Should be 8 acc spec. 9 nicer.
        int             m_FontMode;
        
        QList <QColor>  m_Colors;           // Unaltered standards
        QList <QColor>  m_Colors_Intensity; // Modulated by m_Intensity.
        QList <QColor>  m_Colors_Dim;       // Dimmed version
        QList <QColor>  m_Colors_Bold;      // Bold (more intense) version
        
        class cbMainWindow*	m_MainWindow;
        QStringList     	m_ColorSchemes;
        QByteArray          m_KeyStrokes;
        QMutex              m_KeyStrokesMutex;
        
        // For catching the stuff on the QLineEdit associated with this.
        bool eventFilter(QObject *Object, QEvent *Event);
        
        // The array of cells and rows of TabStops and WidthHeight info.
        cbScreenCell* m_Cells; // #Columns*Row+Col.
        bool*         m_TabStops;
        cbDWDH*       m_DWDHs;
        
        // VtParser.
        
        static const int c_VtParserMaxIChars = 2;
        
        int     m_VtParserState;
        uint8_t m_VtParserIChars[c_VtParserMaxIChars+1];
        int     m_VtParserNrIChars;        // Intermediate chars in ESC, CSI ...
        bool    m_VtParserIgnoreFlagged;
        int     m_VtParserParms[16];
        int     m_VtParserNrParms;
        int     m_VtParserCallbackAction;  // Used for later error reporting.
        uint8_t m_VtParserCallbackChar;    // Used for later error reporting.
        bool    m_VtParserReparse;         // For re-entrance without re-entering.
        uint8_t m_VtParserReparseChar;     // Will reparse with this (ESC-..=>C1)
        uint8_t m_VtParserDCSFinalChar;
        
        QByteArray m_VtParserUDKDefinition;
        QByteArray m_VtParserDLDDefinition;
        
        void VtParserInit();
        void VtParse(const uint8_t Char);
        void VtParserCallback(const int State, const uint8_t Char);
        void VtParserDoAction(const int Action, const uint8_t Char);
        void VtParserDoStateChange(const uint8_t Change, const uint8_t Char);
        void VtParserShowResult(); // Mainly for error reporting.
        
        void VtPutChar(const uint8_t Char);       // Response to the PRINT action.
        void VtExecute(const uint8_t Char);       // C0/C1 controls.
        void VtCsiCommand(const uint8_t Char);
        void VtEscCommand(const uint8_t Char);
        void VtHookCommand(const uint8_t Char);   // Hook,Put,Unhook for DSC strings.
        void VtPutCommand(const uint8_t Char);
        void VtUnHookCommand(const uint8_t Char);
        
        // VT commands.
        // See also http://vt100.net/docs/vt510-rm/contents
        
        void VtANSICL(const uint8_t Char);// ANSI Conformance Level.
        void VtBS();                      // BackSpace.
        void VtCBT();                     // Cursor Backward Tabulation.
        void VtCR();                      // Carriage Return.
        void VtCHA();                     // Cursor Horizontal Absolute.
        void VtCHT();                     // Cursor Horizontal forward Tabulation.
        void VtCNL();                     // Cursor Next Line.
        void VtCPL();                     // Cursor Previous Line.
        //   VtCPR();                     // Cursor Position Report. Part of VtDSR()
        //   VtCRM();                     // Control Character Mode. Part of VtMode()
        void VtCUB();                     // Cursor Backward.
        void VtCUD();                     // Cursor Down.
        void VtCUF();                     // Cursor Forward.
        void VtCUP();                     // Cursor Position.
        void VtCUU();                     // Cusor Up.
        void VtDA1();                     // Device Attributes.
        void VtDA2();                     // Secondary Device Attributes.
        void VtDA3();                     // Tertiary Device Attributes.
        void VtDCH();                     // Delete Character.
        void VtDECALN();                  // Screen Alignment pattern.
        //   VtDECANM();                  // Ansi Mode. Part of VtMode().
        void VtDECANSI();                 // Enter ANSI.
        //   VtDECARM();                  // Auto Repeat Mode. Part of VtMode().
        //   VtDECARSM();                 // Auto Resize Mode. Part of VtMode().
        void VtDECAUPSS();                // Assigning User-Preferred Supplemental Sets.
        //   VtDECAWM();                  // Auto Wrap Mode. Part of VtMode().
        void VtDECBI();                   // Back Index.
        //   VtDECBKM();                  // Backarrow Key Mode. Part of VtMode().
        //   VtDECCANSM();                // Conceal Answerback Message Mode. VtMode().
        //   VtDECCAPSLK();               // Caps Lock Mode. VtMode().
        void VtDECCARA();                 // Change Attributes in Rectangular Area.
        // XXX CB TOT HIER Met de lijst exhaustief te maken.
        void VtDECDHL(const bool Bottom); // Double Height Line.
        void VtDECDLD();                  // DownLoadable charachter set.
        void VtDECDWL();                  // Double Width Line. (and Single Height)
        void VtDECKPAM();                 // Keypad Application Mode.
        void VtDECKPNM();                 // Keypad Numeric Mode.
        void VtDECLL();                   // Load Leds.
        void VtDECRC();                   // Restore Cursor.
        void VtDECREQTPARM();             // Request Terminal Parameters.
        void VtDECSC();                   // Save Cursor.
        void VtDECSCA();                  // Select Character Protection Attribute.
        void VtDECSCL();                  // Select Conformance Level.
        void VtDECSED();                  // Selective Erase in Display.
        void VtDECSEL();                  // Selective Erase in Line.
        void VtDECSLRM();                 // Select Left and Right Margin.
        void VtDECSTBM();                 // Set Top Bottom Margin.
        void VtDECSTR();                  // Soft Terminal Reset.
        void VtDECSWL();                  // Single Width Line. (and Single Height)
        void VtDECTST();                  // Invoke confidence Test.
        void VtDECUDK();                  // User Defined Keys.
        void VtDL();                      // Delete Line.
        void VtDSR();                     // Device Status Report.
        void VtECH();                     // Erase Character.
        void VtED();                      // Erase in Display.
        void VtEL();                      // Erase in Line.
        void VtHPA();                     // Horizontal Position Absolute.
        void VtHPR();                     // Horizontal Position Relative.
        void VtHT();                      // Horizontal Tab.
        void VtHTS();                     // Horizontal Tab Set.
        void VtHVP();                     // Horizontal and Vertical Positioning.
        void VtICH();                     // Insert Character.
        void VtIL();                      // Insert Line.
        void VtIND();                     // Index. (one line down)
        void VtLF();                      // Line Feed.
        void VtLS1R();                    // Lock Shift 1 Right.
        void VtLS2();                     // Lock Shift 2.
        void VtLS2R();                    // Lock Shift 2 Right.
        void VtLS3();                     // Lock Shift 3.
        void VtLS3R();                    // Lock Shift 3 Right.
        void VtMode(const bool Set);      // SM/RM Mode (re)setting.
        void VtNEL();                     // Next Line.
        void VtRI();                      // Reverse Index. (one line up)
        void VtRIS();                     // Reset to Initial State.
        void VtS7C1T();                   // Transmit as 7 bit.
        void VtS8C1T();                   // Transmit as 8 bit.
        void VtSCS(const int     Set, 
                   const uint8_t Char);   // Select Character Set (Set=0..3,Char='B'..)
        void VtSGR();                     // Select Graphic Rendition.
        void VtSI();                      // Shift In.
        void VtSO();                      // Shift Out.
        void VtSS2();                     // Single Shift 2.
        void VtSS3();                     // Single Shift 3.
        void VtTBC();                     // Tabulation clear.
        void VtVPA();                     // Vertical line Position Absolute. 
        void VtVPR();                     // Vertical Position Relative.
        
        // VT52 special (Non-ANSI)
        void Vt52CUD();                   // VT52 - Cursor Down.
        void Vt52CUL();                   // VT52 - Cursor Left.
        void Vt52CUR();                   // VT52 - Cursor Right.
        void Vt52CUU();                   // VT52 - Cursor Up.
        void Vt52DCA(const uint8_t Char); // VT52 - Direct Cursor Address.
        void Vt52ED();                    // VT52 - Erase to end of screen.
        void Vt52EL();                    // VT52 - Erase to end of line.
        void Vt52ESA();                   // VT52 - Graphics set off.
        void Vt52Home();                  // VT52 - Cursor home.
        void Vt52RI();                    // Reverse Index. (one line up)
        void Vt52SSA();                   // VT52 - Graphics set on.
        
        
        // VT meanings.
        int        m_NrRows;
        int        m_NrCols;
        int        m_CursorRow;
        int        m_CursorCol;
        uint16_t   m_CurrentAttr;
        int        m_ScrollTop;
        int        m_ScrollBot;
        bool       m_OriginMode;
        bool       m_InverseMode;
        bool       m_SmoothScroll;
        bool       m_AutoWrap;
        bool       m_LNM;	 // LNM Mode : <Ret>=>CR/LF , LF=>Cursor@Col0
        QByteArray m_G[4]; // G0..G3 'B' = US ASCII
        int        m_GL;   // Index in above.
        int        m_GR;   // Index in above.
        bool       m_DECNRCM;
        bool       m_InsertMode;
        int        m_TermLevel;
        bool       m_CursorAppSeq;
        bool       m_KeypadNumeric;
        bool       m_LocalEcho;
        bool       m_CursorVisible;
        bool       m_Transmit8Bit;
        bool       m_UDKLocked;
        int        m_KeyboardCode;
        bool       m_VT52_SSA;
        
        //
        int      m_OldGL;// Remembered value in single shifts.
        bool     m_SingleShifting;
        
        // Function keys definition.
        QByteArray m_FunctionKeys[4*20]; // Normal,Shift,Alt,Alt-Shift
        
        // This series is for saving/restoring cursor.
        int        m_SavedCursorRow;
        int        m_SavedCursorCol;
        uint16_t   m_SavedAttr;
        QByteArray m_SavedG[4];
        char       m_SavedGL;
        char       m_SavedGR;
        
        // Others.
        bool     m_Dirty;
        bool     m_Vt52EscYCollectingRow; // VT52 ESC-Y exception on ANSI parser.
        bool     m_Vt52EscYCollectingCol;
        
        
        int  GetNrCols(const int Row);
        void ResetDWDH(const int Row);
        void LineDown();
        void LineUp();
        void GenerateCsiReply(const char* Reply);
        void GenerateReply(const char* Reply);
        void ClampCursor(); // Help function to clamp cursor in scroll region.
        void EraseInDisplay(const bool Selective); // Common DECSED, ED
        void EraseInLine(const bool Selective); // Common DECSEL, EL
        
        //
        QImage*  m_ScreenImage;
        uint16_t ToCellChar(const uint8_t Byte);
        void CreateHardCharSets();
        void CreateVt220Glyphs();
        void CreateHostGlyphs();
        void PaintScreen(QWidget* TheWidget);
    
    };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

inline int cbConsole::GetNrCols(const int Row) 
    {
    return m_DWDHs[Row].DW ? m_NrCols/2 : m_NrCols;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

inline void cbConsole::ResetDWDH(const int Row) 
    {
    m_DWDHs[Row].DW = false;
    m_DWDHs[Row].DH = false;
    m_DWDHs[Row].Bt = false;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
