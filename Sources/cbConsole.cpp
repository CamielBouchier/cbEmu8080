
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "cbConsole.h"
#include "cbEmu8080.h"
#include "cbMainWindow.h"
#include "cbVt220CharRom.h"
#include "cbVtParserTables.h"

void StopVtTest(void); // In vttest.cpp, but don't want to pull in lots unneeded.
bool RunningVtTest(void);

#define MAX_INTERMEDIATE_CHARS 2
#define ACTION(state_change) (state_change & 0x0F)
#define STATE(state_change)  (state_change >> 4)

#define MAKECHARPAIR(x,y) (((x)<<8) | (y))

#define DEFAULT_1(N) \
  int N =(m_VtParserNrParms and m_VtParserParms[0] >  0)? m_VtParserParms[0] : 1;
#define DEFAULT_1_0(N) \
  int N = m_VtParserNrParms ? m_VtParserParms[0] : 1;
#define DEFAULT_0(N) \
  int N = m_VtParserNrParms ? m_VtParserParms[0] : 0;

#define SHORT_BUFFER_SIZE 64

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cbConsole::cbConsole()
    {
    if (QThread::currentThread() != qApp->thread())
        {
        this->moveToThread(qApp->thread());
        }

    // No point in moving to cbEmu. Source and Target are here.
    connect(this, &cbConsole::SignalWrite, this, &cbConsole::Write);
    // connect(this, &cbConsole::SignalRead,  this, &cbConsole::Read);

    m_Background     = Qt::black;
    m_Foreground     = Qt::green;
    m_CursorPhaseOn  = false;
    m_NrRows         = 24;
    m_NrCols         = 80;

    // Finally (reallocated) in a hard reset.
    // So it will work for size changes as well.
    m_Cells    = NULL;
    m_TabStops = NULL;
    m_DWDHs    = NULL;

    m_MainWindow = Emu8080->m_MainWindow;
    QSettings* UserSettings = Emu8080->m_UserSettings;

    // Standard colors.
    // Order as in terminal specs.
    m_Colors << Qt::black
             << Qt::red
             << Qt::green
             << Qt::yellow
             << Qt::blue
             << Qt::magenta
             << Qt::cyan
             << Qt::white
             // Default colors 8 and 9.
             << m_Foreground	
             << m_Background;

    // Screen Image.
    m_ScreenImage = new QImage(m_NrCols * c_ColsPerChar, 
                               // 2 for interline scan gap.
                               m_NrRows * c_RowsPerChar,
                               QImage::Format_RGB32);

    // Vt220 font settings.
    bool Vt220Fonts = UserSettings->value("Console/Vt220Fonts",1).toBool();
    OnFontModeVt220Changed(Vt220Fonts);

 
    // Blank interline settings.
    m_BlankInterline = UserSettings->value("Console/BlankInterline",1).toBool();
    OnBlankInterlineChanged(m_BlankInterline);

    // Underline row settings.
    m_UnderlineRow = ( UserSettings->value("Console/UnderlineRow9",false).toBool() ) ? 9 : 8;
    OnUnderlineRowChanged(m_UnderlineRow == 9);

    // Intensity settings.
    // With respects to settings, this must come befor Color Scheme settings !
    m_Intensity = UserSettings->value("Console/Intensity",100).toInt();
    OnIntensityChanged(m_Intensity);

    // Color scheme settings.
    m_ColorSchemes << "Green on black"
                   << "White on black"
                   << "Amber on black";
    m_MainWindow->QCB_ConsoleColorScheme->addItems(m_ColorSchemes);
    m_ColorScheme = UserSettings->value("Console/ColorScheme",0).toInt();
    OnColorSchemeChanged(m_ColorScheme);

    //
    Reset();

    // Needed to size the console to a minimum.
    OnSizeChange(m_NrRows,m_NrCols);

    // Parser
    VtParserInit();

    // Fonts, Glyphs etc. Give some time to show the status.
    // XXX CB TODO Emu8080->UpdateStatus(tr("Constructing characters"));
    qInfo("Constructing characters.");
    CreateHardCharSets();
    qInfo("Constructed characters.");
    // XXX CB TODO Emu8080->ResetStatus();
    //

    // This is where we will start painting ourselves and receive input.
    m_MainWindow->QF_Console->installEventFilter(this);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::CreateVt220Glyphs() {
  for (int Set = 0; Set < c_NrHardCharSets; Set++) {
    for (int Char = 0; Char < c_NrCharsPerSet; Char++) {
      uint16_t Unicode = m_CharSets[Set].Uni16Map[Char];
      int  Idx;
      bool Found = false;
      // Find the character position in the ROM
      for (Idx = 0; Idx < 288; Idx++) {
        if (Vt220CharRomUnicode[Idx] == Unicode) {
          Found = true;
          break;
        }
      }
      if (not Found) {
        qFatal("%s@%s:%d Did not found Unicode %04X for Char %d in Set %d",
               __PRETTY_FUNCTION__,CB_BASENAME(__FILE__),__LINE__,
               Unicode,Char,Set);
      }
      for (int Row=0; Row < c_RowsPerGlyph_Vt220; Row++) {
        m_CharSets[Set].Vt220Glyphs[Char][Row] = Vt220CharRom[Idx][Row];
      }
    }
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::CreateHostGlyphs() {

  QFont Font;

  Font.setFamily("Courier");
  Font.setKerning(false);

  int PixelSize;
  int FontWidth  = 0;
  int FontHeight = 0;

  for (PixelSize = 5 ; PixelSize < 25 ; PixelSize++) {
    Font.setPixelSize(PixelSize);
    QFontMetrics FontMetrics(Font);
    int FH = FontMetrics.height();
    int FW = FontMetrics.averageCharWidth();
    /*
    qDebug("%s@%s:%d PixelSize : %02d - FontHeight : %02d - FontWidth : %02d",
           __PRETTY_FUNCTION__,CB_BASENAME(__FILE__),__LINE__,
           PixelSize,FH,FW);
    */
    if (FH > c_RowsPerGlyph_Host or
        FW > c_ColsPerGlyph_Host) {
      PixelSize--;
      break;
    }
    FontWidth  = FW;
    FontHeight = FH;
  }
  qDebug("PixelSize : %02d was selected. Height : %d Width : %d",
         PixelSize,FontHeight,FontWidth);

  QImage Bitmap(c_ColsPerGlyph_Host,c_RowsPerGlyph_Host,QImage::Format_Mono);

  for (int Set = 0; Set < c_NrHardCharSets; Set++) {
    for (int Char = 0; Char < c_NrCharsPerSet; Char++) {
      uint16_t Unicode = m_CharSets[Set].Uni16Map[Char];
      for (int Style=c_FontStyle_Normal; Style <= c_FontStyle_Italic; Style++) {
        Bitmap.fill(0);
        QPainter Painter(&Bitmap);
        Painter.setBackground(Qt::color0);
        Painter.setPen(Qt::color0);
        Font.setPixelSize(PixelSize);
        switch (Style) {
          default :
            Font.setBold(false);
            Font.setItalic(false);
            break;
          case c_FontStyle_Bold :
            Font.setBold(true);
            Font.setItalic(false);
            break;
          case c_FontStyle_Italic :
            Font.setBold(false);
            Font.setItalic(true);
            break;
        }
        Painter.setFont(Font);
        Painter.drawText(Bitmap.rect(),QChar(Unicode));
        for (int Row = 0 ; Row < c_RowsPerGlyph_Host ; Row++) {
          uint16_t GlyphWord = 0;
          for (int Col = c_ColsPerGlyph_Host - 1; Col >= 0 ; Col--) {
            GlyphWord = (GlyphWord << 1) | ((Bitmap.pixel(Col,Row) & 0XFF)?1:0);
          }
          m_CharSets[Set].HostGlyphs[Style][Char][Row] = GlyphWord;
        }
      }
    }
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cbConsole::~cbConsole() 
    {
    qDebug("Start destructor");
    #ifdef CB_WITH_VTTEST
        if (RunningVtTest()) 
            {
            StopVtTest();
            }
    #endif
    CB_FREE(m_Cells);
    CB_FREE(m_TabStops);
    CB_FREE(m_DWDHs);
    delete m_ScreenImage;
    qDebug("End   destructor");
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::Reset(const bool Hard) 
    {
    m_KeyStrokesMutex.lock();
    m_KeyStrokes.clear();
    m_KeyStrokesMutex.unlock();
  
    if (Hard) 
        {
        m_CursorRow        = 0;
        m_CursorCol        = 0;
        }
  
    m_CurrentAttr      = ATTR_DEFAULT;
    m_Dirty            = true;
    m_OriginMode       = false;
    m_AutoWrap         = false;
    m_InverseMode      = false;
    m_SmoothScroll     = false;
    m_LNM              = false;
    m_InsertMode       = false;
    m_SingleShifting   = false;
    m_DECNRCM          = false; // Normal 8 bit mode.
    m_ScrollTop        = 0;
    m_ScrollBot        = m_NrRows-1;
    m_CursorAppSeq     = false;
    m_LocalEcho        = false;
    m_CursorVisible    = true;
    m_KeypadNumeric    = true;
    m_UDKLocked        = false;
    m_G[0]             = "B"; // US ASCII
    m_G[1]             = "B"; // US ASCII
    m_G[2]             = "<"; // DEC Supplemental.
    m_G[3]             = "<"; // DEC Supplemental.
    m_GL               = 0;
    m_GR               = 2;
    m_VT52_SSA         = false;
  
    // Remind we are pretending to be VT220 now !
    m_TermLevel        = 2;      // 0:VT52 1:VT1XX 2:VT2XX 3:VT3XX 4:VT4XX
    m_Transmit8Bit     = false;
    qDebug("Conformance level set to %d. Tx8bits : %d",
           m_TermLevel, m_Transmit8Bit);
  
    if (Hard) 
        {
        CB_FREE(m_Cells);
        CB_FREE(m_TabStops);
        CB_FREE(m_DWDHs);
  
        CB_CALLOC(m_Cells,cbScreenCell*,m_NrRows*m_NrCols,sizeof(cbScreenCell));
        CB_CALLOC(m_TabStops,bool*,m_NrCols,sizeof(bool));
        CB_CALLOC(m_DWDHs,cbDWDH*,m_NrRows,sizeof(cbDWDH));
  
        for (int Row = 0; Row < m_NrRows; Row++) 
            {
            int Offset = Row * m_NrCols;
            for (int Col = 0; Col < m_NrCols; Col++) 
                {
                m_Cells[Offset+Col].Char = 0X20;
                m_Cells[Offset+Col].Attr = ATTR_DEFAULT;
                }
            }
    
        for (int Col = 0; Col < m_NrCols; Col++) 
            {
            m_TabStops[Col] = ((Col % 8) == 0);
            }
        }
  
    // Preliminary keyboard detection.
    int Map = 0;
    m_KeyboardCode = 0;
    QLocale KeyboardLocale = QGuiApplication::inputMethod()->locale();
    QString KeyboardName   = KeyboardLocale.name();
    if (KeyboardName == "C") 
        {
        // Pretending US.
        m_KeyboardCode = 1;
        Map = 1; // Into CharMap.
        } 
    else if (KeyboardName == "en_US") 
        {
        m_KeyboardCode = 1;
        Map = 1; // Into CharMap.
        } 
    else if (KeyboardName == "nl_BE") 
        {
        m_KeyboardCode = 3;
        Map = 6; // Into CharMap.
        } 
    else if (KeyboardName == "nl_NL") 
        {
        m_KeyboardCode = 6;
        Map = 6; // Into CharMap.
        } 
    else 
        {
        // Pretending US.
        m_KeyboardCode = 1;
        Map = 1; // Into CharMap.
        qWarning("Please complete my keyboard report for locale '%s'",
                 C_STRING(KeyboardName));
        }
    /*
    qDebug("%s@%s:%d m_KeyboardCode set to %d",
           __PRETTY_FUNCTION__,
           CB_BASENAME(__FILE__),
           __LINE__,
           m_KeyboardCode);
    */
    // KeyMap construction. (kind of sparse inverse of c_Uni16Map)
    for (int Char = 0X20; Char < 0X80; Char++) 
        {
        if (Char != m_CharSets[Map].Uni16Map[Char-0X20]) 
            {
            m_KeyMap.insert(m_CharSets[Map].Uni16Map[Char-0X20],Char);
            /*
            qDebug("%04X => %04X (%c)",
                   c_Uni16Map[Map][Char-0X20],
                   Char,
                   c_Uni16Map[Map][Char-0X20]);
            */
            }
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::GenerateReply(const char* Reply) 
    {
    // TODO Check me. Now the keyboard buffer is *replaced* by the reply.
    // Maybe it's more correct to push the reply on front of it. 
    // Not sure if relevant for any practical purpose.
    m_KeyStrokesMutex.lock();
    m_KeyStrokes.clear();
    for (const char* Ptr = Reply; *Ptr; Ptr++) 
        {
        m_KeyStrokes.append(*Ptr);
        }
    m_KeyStrokesMutex.unlock();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::GenerateCsiReply(const char* Reply) {
  char Buffer[SHORT_BUFFER_SIZE];
  const char* FormatString = m_Transmit8Bit ?  "\x9B%s" : "\x1B[%s";
  snprintf(Buffer,SHORT_BUFFER_SIZE,FormatString,Reply);
  GenerateReply(Buffer);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Note read happens in the worker thread ( we can't return a value from a signal ).
// The only risky part is m_KeyStrokes.remove as it is a potential concurrent write.

uint8_t cbConsole::Read(const uint8_t Address) 
    {
    // Do not test against running in the worker thread. The VT test runs in GUI and that's fine.
    uint8_t Value = 0;

    if (Address == 1) 
        {
        m_KeyStrokesMutex.lock();
        int NrKeyStrokes = m_KeyStrokes.size();
        m_KeyStrokesMutex.unlock();
        Value = (NrKeyStrokes != 0)? 0xFF : 0x00;
        } 
    else if (Address == 2) 
        {
        m_KeyStrokesMutex.lock();
        Value = m_KeyStrokes.at(0);
        m_KeyStrokes.remove(0,1);
        m_KeyStrokesMutex.unlock();
        } 
    else 
        {
        qFatal("Address : %02X - Decoding violation.", Address);
        }
    return Value;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::Write(const uint8_t Address, const uint8_t Char) 
    {
    if (QThread::currentThread() != qApp->thread())
        {
        SignalWrite(Address, Char);
        return;
        }
    if (Address != 0) 
        {
        qFatal("Address : %02X - Decoding violation.", Address);
        }

    // VT52 Esc-Y-row-column doesn't fit in the ANSI/DEC parser.
    // Let's hack this here.
    if (m_Vt52EscYCollectingRow or m_Vt52EscYCollectingCol) 
        {
        Vt52DCA(Char);
        return;
        }

    // Normal parse.
    VtParse(Char);
 
    // Poor mens re-entry for ESC-0X40..ESC-0X5F reparsing as C1.
    if (m_VtParserReparse) 
        {
        m_VtParserReparse = false;
        VtParse(m_VtParserReparseChar);
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtParserInit() {
  m_VtParserState         = VTPARSE_STATE_GROUND;
  m_VtParserNrIChars      = 0;
  m_VtParserNrParms       = 0;
  m_VtParserIgnoreFlagged = false;
  m_VtParserReparse       = false;
  m_Vt52EscYCollectingRow = false;
  m_Vt52EscYCollectingCol = false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtParserDoAction(const int Action, const uint8_t Char) {

  /*
  qDebug("%s@%s:%d (%10s,%02X)",
         __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
         ACTION_NAMES[Action],Char);
  */

  switch(Action) {
    case VTPARSE_ACTION_PRINT:
    case VTPARSE_ACTION_EXECUTE:
    case VTPARSE_ACTION_HOOK:
    case VTPARSE_ACTION_PUT:
    case VTPARSE_ACTION_OSC_START:
    case VTPARSE_ACTION_OSC_PUT:
    case VTPARSE_ACTION_OSC_END:
    case VTPARSE_ACTION_UNHOOK:
    case VTPARSE_ACTION_CSI_DISPATCH:
    case VTPARSE_ACTION_ESC_DISPATCH:
      VtParserCallback(Action,Char);
      break;

    case VTPARSE_ACTION_IGNORE:
      break;

    case VTPARSE_ACTION_COLLECT:
      if(m_VtParserNrIChars + 1 > MAX_INTERMEDIATE_CHARS) {
        m_VtParserIgnoreFlagged = true;
      } else {
        m_VtParserIChars[m_VtParserNrIChars++] = Char;
      }
      break;

    case VTPARSE_ACTION_PARAM:
      if (m_VtParserNrParms == 0) {
        if (Char == ';') {
          m_VtParserNrParms  = 2;
          m_VtParserParms[0] = 0;
          m_VtParserParms[1] = 0;
        } else {
          m_VtParserNrParms  = 1;
          m_VtParserParms[0] = (Char - '0');
        }
      } else {
        if(Char == ';') {
          m_VtParserNrParms += 1;
          m_VtParserParms[m_VtParserNrParms-1] = 0;
        } else {
          m_VtParserParms[m_VtParserNrParms-1] *= 10;
          m_VtParserParms[m_VtParserNrParms-1] += (Char - '0');
        }
      }
      break;

    case VTPARSE_ACTION_CLEAR:
      m_VtParserNrIChars      = 0;
      m_VtParserNrParms       = 0;
      m_VtParserIgnoreFlagged = false;
      break;

    default:
      VtParserCallback(VTPARSE_ACTION_ERROR, 0);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtParserDoStateChange(const uint8_t Change, const uint8_t Char) {

  /* A state change is an action and/or a new state to transition to. */

  int NewState = STATE(Change);
  int Action   = ACTION(Change);

  if (NewState) {

    /* Perform up to three actions:
     *   1. the exit action of the old state
     *   2. the action associated with the transition
     *   3. the entry action of the new state
     */

    int ExitAction  = EXIT_ACTIONS[m_VtParserState-1];
    int EntryAction = ENTRY_ACTIONS[NewState-1];

    if (ExitAction)  VtParserDoAction(ExitAction,0);
    if (Action)      VtParserDoAction(Action,Char);
    // CB WAS  if (EntryAction) VtParserDoAction(EntryAction,0);
    // I want hook (an entry action) to reveal its character.
    if (EntryAction) VtParserDoAction(EntryAction,Char);

    m_VtParserState = NewState;
  } else {
    VtParserDoAction(Action,Char);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtParse(const uint8_t Char) {
  
  /*
  qDebug("%s@%s:%d Char : %02X ('%c')",
         __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
         Char,Char);
  */

  uint8_t Change = STATE_TABLE[m_VtParserState-1][Char];
  VtParserDoStateChange(Change,Char);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtParserCallback(const int Action, const uint8_t Char) {

  m_VtParserCallbackAction = Action;
  m_VtParserCallbackChar   = Char;

  // VtParserShowResult();

  switch (Action) {
    case VTPARSE_ACTION_PRINT :
      VtPutChar(Char);
      break;
    case VTPARSE_ACTION_EXECUTE :
      VtExecute(Char);
      break;
    case VTPARSE_ACTION_CSI_DISPATCH :
      VtCsiCommand(Char);
      break;
    case VTPARSE_ACTION_ESC_DISPATCH :
      VtEscCommand(Char);
      break;
    case VTPARSE_ACTION_HOOK :
      VtHookCommand(Char);
      break;
    case VTPARSE_ACTION_UNHOOK :
      VtUnHookCommand(Char);
      break;
    case VTPARSE_ACTION_PUT :
      VtPutCommand(Char);
      break;
    default : 
      qFatal("%s@%s:%d Unexpected action %d ('%s').",
             __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
             Action,ACTION_NAMES[Action]);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtParserShowResult() {

  fprintf(stderr,"Received action %s\n",
          ACTION_NAMES[m_VtParserCallbackAction]);
  fprintf(stderr,"  Char: 0X%02X ('%c')\n",
          m_VtParserCallbackChar,
          m_VtParserCallbackChar);
  if (m_VtParserNrIChars > 0) {
    fprintf(stderr,"  Intermediate chars (%d) :\n",m_VtParserNrIChars);
    for(int i = 0; i < m_VtParserNrIChars; i++) {
      fprintf(stderr,"    0X%02X ('%c')\n",
              m_VtParserIChars[i],m_VtParserIChars[i]);
    }
  }
  if (m_VtParserNrParms > 0) {
    fprintf(stderr,"  Parameters (%d) :\n", m_VtParserNrParms);
    for(int i = 0; i < m_VtParserNrParms; i++) {
      fprintf(stderr,"    %d\n",m_VtParserParms[i]);
    }
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtHookCommand(const uint8_t Char) {
  m_VtParserDCSFinalChar = Char;
  if (m_VtParserDCSFinalChar == '|') {
    m_VtParserUDKDefinition.clear();
  } else if (m_VtParserDCSFinalChar == '!') {
    ; // DECAUPSS not implented.
  } else if (m_VtParserDCSFinalChar == '{') {
    m_VtParserDLDDefinition.clear();
  } else {
    VtParserShowResult();
    qFatal("%s@%s:%d Unexpected DCS Final Character '%c'.",
           __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
           Char);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtPutCommand(const uint8_t Char) {
  if (m_VtParserDCSFinalChar == '|') {
    m_VtParserUDKDefinition.append(Char);
  } else if (m_VtParserDCSFinalChar == '!') {
    ; // DECAUPSS not implented.
  } else if (m_VtParserDCSFinalChar == '{') {
    m_VtParserDLDDefinition.append(Char);
  } else {
    VtParserShowResult();
    qFatal("%s@%s:%d Unexpected DCS Final Character '%c'.",
           __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
           Char);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtUnHookCommand(const uint8_t Char) {
  if (m_VtParserDCSFinalChar == '|') {
    VtDECUDK();
  } else if (m_VtParserDCSFinalChar == '!') {
    VtDECAUPSS(); 
  } else if (m_VtParserDCSFinalChar == '{') {
    VtDECDLD();
  } else {
    VtParserShowResult();
    qFatal("%s@%s:%d Unexpected DCS Final Character '%c'.",
           __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
           Char);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtPutChar(const uint8_t Char) {

  /*
  qDebug("%s@%s:%d Char : %02X ('%c') @(%3d,%3d)(AW:%1d) (CS:%3d)",
         __PRETTY_FUNCTION__,
         CB_BASENAME(__FILE__),
         __LINE__,
         Char,
         Char,
         m_CursorRow,
         m_CursorCol,
         m_AutoWrap,
         GetNrCols(m_CursorRow));
  */

  if (m_CursorCol > GetNrCols(m_CursorRow)-1) {
    if (m_AutoWrap) {
      m_CursorCol = 0;
      LineDown();
    } else {
      m_CursorCol = GetNrCols(m_CursorRow)-1;
    }
  }

  int Offset = m_CursorRow * m_NrCols;
  if (m_InsertMode) {
    for (int Col = GetNrCols(m_CursorRow)-1; Col > m_CursorCol; Col--) {
      m_Cells[Offset+Col] = m_Cells[Offset+Col-1];
    }
  }
      
  uint16_t TheChar = ToCellChar(Char);

  /*
  qDebug("%s@%s:%d VtPutChar(%02X) ('%c') with Attr %04X", 
         __PRETTY_FUNCTION__,CB_BASENAME(__FILE__),__LINE__,
         Char,Char,m_CurrentAttr);
  */
  m_Cells[Offset + m_CursorCol].Char = TheChar;
  m_Cells[Offset + m_CursorCol].Attr = m_CurrentAttr;

  m_CursorCol++;

  m_Dirty = true;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtExecute(const uint8_t Char) 
    {
    /*
    qDebug("%s@%s:%d Char : %02X ('%c')",
           __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
           Char,Char);
    */
  
    // Much of > 0X80 is actually polluted by VT52 non-ANSI exceptions
    // but is 'cleaned' by this macro.
    #define VT52ALT(VT52,VT100) {if (m_TermLevel == 0) {VT52;} else {VT100;}}
    #define VT52ONLY(VT52) {if (m_TermLevel == 0) {VT52;} else \
         {qDebug("ESC-%c (0X%02X) is currently NOP",Char-0X40,Char);}}
    
    switch(Char) 
        {
        case 0X04 : qDebug("0X%02X is currently NOP",Char);             break; // EOT.
        case 0X05 : GenerateReply(C_STRING(cbEmu8080::ProgramName));    break; // ENQ
        case 0X07 : qDebug("0X%02X is currently NOP",Char);             break; // BELL
        case 0X08 : VtBS();                                             break;
        case 0X09 : VtHT();                                             break;
        case 0X0A : VtLF();                                             break; // LF.
        case 0X0B : VtLF();                                             break; // VT.
        case 0X0C : VtLF();                                             break; // FF.
        case 0X0D : VtCR();                                             break;
        case 0X0E : VtSO();                                             break;
        case 0X0F : VtSI();                                             break;
        case 0X81 : VT52ONLY(Vt52CUU());                                break;
        case 0X82 : VT52ONLY(Vt52CUD());                                break;
        case 0X83 : VT52ONLY(Vt52CUR());                                break;
        case 0X84 : VT52ALT(Vt52CUL(),VtIND());                         break;
        case 0X85 : VtNEL();                                            break;
        case 0X86 : VT52ONLY(Vt52SSA());                                break;
        case 0X87 : VT52ONLY(Vt52ESA());                                break;
        case 0X88 : VT52ALT(Vt52Home(),VtHTS());                        break;
        case 0X89 : VT52ONLY(Vt52RI());                                 break;
        case 0X8A : VT52ONLY(Vt52ED());                                 break;
        case 0X8B : VT52ONLY(Vt52EL());                                 break;
        case 0X8D : VtRI();                                             break;
        case 0X8E : VtSS2();                                            break;
        case 0X8F : VtSS3();                                            break;
        case 0X99 : VT52ONLY(Vt52DCA(Char));                            break;
        case 0X9A : VT52ONLY(GenerateReply("\x1B/Z"));                  break; // Emulating VT52
  
        default :
            VtParserShowResult();
            qFatal("%s@%s:%d Unexpected VtExecute(%02X) ('%c')",
                   __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
                   Char,Char);
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtCsiCommand(const uint8_t Char) {

  /*
  qDebug("%s@%s:%d Char : %02X ('%c')",
         __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
         Char,Char);
  */
  
  uint16_t Discriminant = 
      MAKECHARPAIR(m_VtParserNrIChars ? m_VtParserIChars[0] : 0, Char);

  switch(Discriminant) {

    case MAKECHARPAIR( 0 ,'@') : VtICH();         break;
    case MAKECHARPAIR( 0 ,'A') : VtCUU();         break;
    case MAKECHARPAIR( 0 ,'B') : VtCUD();         break;
    case MAKECHARPAIR( 0 ,'C') : VtCUF();         break;
    case MAKECHARPAIR( 0 ,'D') : VtCUB();         break;
    case MAKECHARPAIR( 0 ,'E') : VtCNL();         break;
    case MAKECHARPAIR( 0 ,'F') : VtCPL();         break;
    case MAKECHARPAIR( 0 ,'G') : VtCHA();         break;
    case MAKECHARPAIR( 0 ,'H') : VtCUP();         break;
    case MAKECHARPAIR( 0 ,'I') : VtCHT();         break;
    case MAKECHARPAIR( 0 ,'J') : VtED();          break;
    case MAKECHARPAIR( 0 ,'K') : VtEL();          break;
    case MAKECHARPAIR( 0 ,'L') : VtIL();          break;
    case MAKECHARPAIR( 0 ,'M') : VtDL();          break;
    case MAKECHARPAIR( 0 ,'P') : VtDCH();         break;
    case MAKECHARPAIR( 0 ,'X') : VtECH();         break;
    case MAKECHARPAIR( 0 ,'Z') : VtCBT();         break;
    case MAKECHARPAIR( 0 ,'`') : VtHPA();         break;
    case MAKECHARPAIR( 0 ,'a') : VtHPR();         break;
    case MAKECHARPAIR( 0 ,'c') : VtDA1();         break;
    case MAKECHARPAIR( 0 ,'d') : VtVPA();         break;
    case MAKECHARPAIR( 0 ,'e') : VtVPR();         break;
    case MAKECHARPAIR( 0 ,'f') : VtHVP();         break;
    case MAKECHARPAIR( 0 ,'g') : VtTBC();         break;
    case MAKECHARPAIR( 0 ,'h') : VtMode(true);    break;
    case MAKECHARPAIR( 0 ,'l') : VtMode(false);   break;
    case MAKECHARPAIR( 0 ,'m') : VtSGR();         break;
    case MAKECHARPAIR( 0 ,'n') : VtDSR();         break;
    case MAKECHARPAIR( 0 ,'q') : VtDECLL();       break;
    case MAKECHARPAIR( 0 ,'r') : VtDECSTBM();     break;
    case MAKECHARPAIR( 0 ,'s') : VtDECSLRM();     break;
    case MAKECHARPAIR( 0 ,'x') : VtDECREQTPARM(); break;
    case MAKECHARPAIR( 0 ,'y') : VtDECTST();      break;
    case MAKECHARPAIR('>','c') : VtDA2();         break;
    case MAKECHARPAIR('=','c') : VtDA3();         break;
    case MAKECHARPAIR('?','J') : VtDECSED();      break;
    case MAKECHARPAIR('?','K') : VtDECSEL();      break;
    case MAKECHARPAIR('?','h') : VtMode(true);    break;
    case MAKECHARPAIR('?','l') : VtMode(false);   break;
    case MAKECHARPAIR('?','n') : VtDSR();         break;
    case MAKECHARPAIR('"','p') : VtDECSCL();      break;
    case MAKECHARPAIR('"','q') : VtDECSCA();      break;
    case MAKECHARPAIR('!','p') : VtDECSTR();      break;
    case MAKECHARPAIR('$','r') : VtDECCARA();     break;

    default : 
      VtParserShowResult();
      qFatal("%s@%s:%d Unexpected VtCsiCommand (Discriminant %04X)",
             __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
             Discriminant);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtEscCommand(const uint8_t Char) {

  /*
  qDebug("%s@%s:%d Char : %02X ('%c')",
         __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
         Char,Char);
  */

  uint16_t Discriminant = 
      MAKECHARPAIR(m_VtParserNrIChars ? m_VtParserIChars[0] : 0, Char);

  // This define is the simple ESC-OneCharacter stuff.
  // I will do ESC-0X40..ESC-0X5F remapping into C1.
  // This way I can avoid code duplication, at a slight cost of
  // re-entering the parser.
  #define REPARSE(C) {                       \
      m_VtParserReparse     = true;          \
      m_VtParserReparseChar = 0X40 + C;      \
  }

  switch (Discriminant) {
 
    case MAKECHARPAIR( 0 ,'@') : REPARSE(0X40);   break;
    case MAKECHARPAIR( 0 ,'A') : REPARSE(0X41);   break;
    case MAKECHARPAIR( 0 ,'B') : REPARSE(0X42);   break;
    case MAKECHARPAIR( 0 ,'C') : REPARSE(0X43);   break;
    case MAKECHARPAIR( 0 ,'D') : REPARSE(0X44);   break;
    case MAKECHARPAIR( 0 ,'E') : REPARSE(0X45);   break;
    case MAKECHARPAIR( 0 ,'F') : REPARSE(0X46);   break;
    case MAKECHARPAIR( 0 ,'G') : REPARSE(0X47);   break;
    case MAKECHARPAIR( 0 ,'H') : REPARSE(0X48);   break;
    case MAKECHARPAIR( 0 ,'I') : REPARSE(0X49);   break;
    case MAKECHARPAIR( 0 ,'J') : REPARSE(0X4A);   break;
    case MAKECHARPAIR( 0 ,'K') : REPARSE(0X4B);   break;
    case MAKECHARPAIR( 0 ,'L') : REPARSE(0X4C);   break;
    case MAKECHARPAIR( 0 ,'M') : REPARSE(0X4D);   break;
    case MAKECHARPAIR( 0 ,'N') : REPARSE(0X4E);   break;
    case MAKECHARPAIR( 0 ,'O') : REPARSE(0X4F);   break;
    case MAKECHARPAIR( 0 ,'P') : REPARSE(0X50);   break;
    case MAKECHARPAIR( 0 ,'Q') : REPARSE(0X51);   break;
    case MAKECHARPAIR( 0 ,'R') : REPARSE(0X52);   break;
    case MAKECHARPAIR( 0 ,'S') : REPARSE(0X53);   break;
    case MAKECHARPAIR( 0 ,'T') : REPARSE(0X54);   break;
    case MAKECHARPAIR( 0 ,'U') : REPARSE(0X55);   break;
    case MAKECHARPAIR( 0 ,'V') : REPARSE(0X56);   break;
    case MAKECHARPAIR( 0 ,'W') : REPARSE(0X57);   break;
    case MAKECHARPAIR( 0 ,'X') : REPARSE(0X58);   break;
    case MAKECHARPAIR( 0 ,'Y') : REPARSE(0X59);   break;
    case MAKECHARPAIR( 0 ,'Z') : REPARSE(0X5A);   break;
    case MAKECHARPAIR( 0 ,'[') : REPARSE(0X5B);   break;
    case MAKECHARPAIR( 0 ,'\\'): REPARSE(0X5C);   break;
    case MAKECHARPAIR( 0 ,']') : REPARSE(0X5D);   break;
    case MAKECHARPAIR( 0 ,'^') : REPARSE(0X5E);   break;
    case MAKECHARPAIR( 0 ,'_') : REPARSE(0X5F);   break;
    case MAKECHARPAIR( 0 ,'6') : VtDECBI();       break;
    case MAKECHARPAIR( 0 ,'7') : VtDECSC();       break;
    case MAKECHARPAIR( 0 ,'8') : VtDECRC();       break;
    case MAKECHARPAIR( 0 ,'<') : VtDECANSI();     break;
    case MAKECHARPAIR( 0 ,'=') : VtDECKPAM();     break;
    case MAKECHARPAIR( 0 ,'>') : VtDECKPNM();     break;
    case MAKECHARPAIR( 0 ,'c') : VtRIS();         break;
    case MAKECHARPAIR( 0 ,'n') : VtLS2();         break;
    case MAKECHARPAIR( 0 ,'o') : VtLS3();         break;
    case MAKECHARPAIR( 0 ,'|') : VtLS3R();        break;
    case MAKECHARPAIR( 0 ,'}') : VtLS2R();        break;
    case MAKECHARPAIR( 0 ,'~') : VtLS1R();        break;
    case MAKECHARPAIR('#','3') : VtDECDHL(false); break;
    case MAKECHARPAIR('#','4') : VtDECDHL(true);  break;
    case MAKECHARPAIR('#','5') : VtDECSWL();      break;
    case MAKECHARPAIR('#','6') : VtDECDWL();      break;
    case MAKECHARPAIR('#','8') : VtDECALN();      break;
    case MAKECHARPAIR('(','A') : VtSCS(0,'A');    break;
    case MAKECHARPAIR('(','B') : VtSCS(0,'B');    break;
    case MAKECHARPAIR('(','0') : VtSCS(0,'0');    break;
    case MAKECHARPAIR('(','1') : VtSCS(0,'1');    break;
    case MAKECHARPAIR('(','2') : VtSCS(0,'2');    break;
    case MAKECHARPAIR('(','<') : VtSCS(0,'<');    break;
    case MAKECHARPAIR('(','4') : VtSCS(0,'4');    break;
    case MAKECHARPAIR('(','5') : VtSCS(0,'5');    break;
    case MAKECHARPAIR('(','C') : VtSCS(0,'C');    break;
    case MAKECHARPAIR('(','R') : VtSCS(0,'R');    break;
    case MAKECHARPAIR('(','Q') : VtSCS(0,'Q');    break;
    case MAKECHARPAIR('(','K') : VtSCS(0,'K');    break;
    case MAKECHARPAIR('(','Y') : VtSCS(0,'Y');    break;
    case MAKECHARPAIR('(','E') : VtSCS(0,'E');    break;
    case MAKECHARPAIR('(','6') : VtSCS(0,'6');    break;
    case MAKECHARPAIR('(','Z') : VtSCS(0,'Z');    break;
    case MAKECHARPAIR('(','H') : VtSCS(0,'H');    break;
    case MAKECHARPAIR('(','7') : VtSCS(0,'7');    break;
    case MAKECHARPAIR('(','=') : VtSCS(0,'=');    break;
    case MAKECHARPAIR('(','@') : VtSCS(0,'@');    break;
    case MAKECHARPAIR(')','A') : VtSCS(1,'A');    break;
    case MAKECHARPAIR(')','B') : VtSCS(1,'B');    break;
    case MAKECHARPAIR(')','0') : VtSCS(1,'0');    break;
    case MAKECHARPAIR(')','1') : VtSCS(1,'1');    break;
    case MAKECHARPAIR(')','2') : VtSCS(1,'2');    break;
    case MAKECHARPAIR(')','<') : VtSCS(1,'<');    break;
    case MAKECHARPAIR(')','4') : VtSCS(1,'4');    break;
    case MAKECHARPAIR(')','5') : VtSCS(1,'5');    break;
    case MAKECHARPAIR(')','C') : VtSCS(1,'C');    break;
    case MAKECHARPAIR(')','R') : VtSCS(1,'R');    break;
    case MAKECHARPAIR(')','Q') : VtSCS(1,'Q');    break;
    case MAKECHARPAIR(')','K') : VtSCS(1,'K');    break;
    case MAKECHARPAIR(')','Y') : VtSCS(1,'Y');    break;
    case MAKECHARPAIR(')','E') : VtSCS(1,'E');    break;
    case MAKECHARPAIR(')','6') : VtSCS(1,'6');    break;
    case MAKECHARPAIR(')','Z') : VtSCS(1,'Z');    break;
    case MAKECHARPAIR(')','H') : VtSCS(1,'H');    break;
    case MAKECHARPAIR(')','7') : VtSCS(1,'7');    break;
    case MAKECHARPAIR(')','=') : VtSCS(1,'=');    break;
    case MAKECHARPAIR(')','@') : VtSCS(1,'@');    break;
    case MAKECHARPAIR('*','A') : VtSCS(2,'A');    break;
    case MAKECHARPAIR('*','B') : VtSCS(2,'B');    break;
    case MAKECHARPAIR('*','0') : VtSCS(2,'0');    break;
    case MAKECHARPAIR('*','1') : VtSCS(2,'1');    break;
    case MAKECHARPAIR('*','2') : VtSCS(2,'2');    break;
    case MAKECHARPAIR('*','<') : VtSCS(2,'<');    break;
    case MAKECHARPAIR('*','4') : VtSCS(2,'4');    break;
    case MAKECHARPAIR('*','5') : VtSCS(2,'5');    break;
    case MAKECHARPAIR('*','C') : VtSCS(2,'C');    break;
    case MAKECHARPAIR('*','R') : VtSCS(2,'R');    break;
    case MAKECHARPAIR('*','Q') : VtSCS(2,'Q');    break;
    case MAKECHARPAIR('*','K') : VtSCS(2,'K');    break;
    case MAKECHARPAIR('*','Y') : VtSCS(2,'Y');    break;
    case MAKECHARPAIR('*','E') : VtSCS(2,'E');    break;
    case MAKECHARPAIR('*','6') : VtSCS(2,'6');    break;
    case MAKECHARPAIR('*','Z') : VtSCS(2,'Z');    break;
    case MAKECHARPAIR('*','H') : VtSCS(2,'H');    break;
    case MAKECHARPAIR('*','7') : VtSCS(2,'7');    break;
    case MAKECHARPAIR('*','=') : VtSCS(2,'=');    break;
    case MAKECHARPAIR('*','@') : VtSCS(2,'@');    break;
    case MAKECHARPAIR('+','A') : VtSCS(3,'A');    break;
    case MAKECHARPAIR('+','B') : VtSCS(3,'B');    break;
    case MAKECHARPAIR('+','0') : VtSCS(3,'0');    break;
    case MAKECHARPAIR('+','1') : VtSCS(3,'1');    break;
    case MAKECHARPAIR('+','2') : VtSCS(3,'2');    break;
    case MAKECHARPAIR('+','<') : VtSCS(3,'<');    break;
    case MAKECHARPAIR('+','4') : VtSCS(3,'4');    break;
    case MAKECHARPAIR('+','5') : VtSCS(3,'5');    break;
    case MAKECHARPAIR('+','C') : VtSCS(3,'C');    break;
    case MAKECHARPAIR('+','R') : VtSCS(3,'R');    break;
    case MAKECHARPAIR('+','Q') : VtSCS(3,'Q');    break;
    case MAKECHARPAIR('+','K') : VtSCS(3,'K');    break;
    case MAKECHARPAIR('+','Y') : VtSCS(3,'Y');    break;
    case MAKECHARPAIR('+','E') : VtSCS(3,'E');    break;
    case MAKECHARPAIR('+','6') : VtSCS(3,'6');    break;
    case MAKECHARPAIR('+','Z') : VtSCS(3,'Z');    break;
    case MAKECHARPAIR('+','H') : VtSCS(3,'H');    break;
    case MAKECHARPAIR('+','7') : VtSCS(3,'7');    break;
    case MAKECHARPAIR('+','=') : VtSCS(3,'=');    break;
    case MAKECHARPAIR('+','@') : VtSCS(3,'@');    break;
    case MAKECHARPAIR(' ','F') : VtS7C1T();       break;
    case MAKECHARPAIR(' ','G') : VtS8C1T();       break;
    case MAKECHARPAIR(' ','L') : VtANSICL(Char);  break;
    case MAKECHARPAIR(' ','M') : VtANSICL(Char);  break;
    case MAKECHARPAIR(' ','N') : VtANSICL(Char);  break;
    default : 
      VtParserShowResult();
      qFatal("%s@%s:%d Unexpected VtEscCommand (Discriminant %04X)",
             __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
             Discriminant);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtANSICL(const uint8_t Char) {
  qDebug("ANSI Conformance Levels (%c) is currently NOP",Char);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtBS() {
  if (m_CursorCol > 0) {
    if (m_CursorCol >= GetNrCols(m_CursorRow)) {
      m_CursorCol = GetNrCols(m_CursorRow)-2;
    } else {
      m_CursorCol--;
    }
  } 
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtCBT() {
  DEFAULT_1(N);
  for (int Step = 0; (Step < N) and (m_CursorCol > 0); Step++) {
    if (m_TabStops[m_CursorCol]) {
      m_CursorCol--;
    }
    while ( (not m_TabStops[m_CursorCol]) and (m_CursorCol >= 0) ) {
      m_CursorCol--;
    }
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtCR() {
  m_CursorCol = 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::ClampCursor() {
  int MinRow = m_OriginMode ? m_ScrollTop : 0;
  int MaxRow = m_OriginMode ? m_ScrollBot : m_NrRows - 1;
  m_CursorRow = MAX(MinRow,MIN(m_CursorRow,MaxRow));
  m_CursorCol = MAX(0,MIN(m_CursorCol,GetNrCols(m_CursorRow)-1));
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtCHA() {
  DEFAULT_1(N);
  m_CursorCol  = N - 1;
  ClampCursor();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtCHT() {
  DEFAULT_1_0(N);
  int ColMax = GetNrCols(m_CursorRow)-1;
  for (int Step = 0; (Step < N) and (m_CursorCol < ColMax); Step++) {
    if (m_TabStops[m_CursorCol]) {
      m_CursorCol++;
    }
    while ( (not m_TabStops[m_CursorCol]) and (m_CursorCol < ColMax) ) {
      m_CursorCol++;
    }
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtCNL() {
  DEFAULT_1(N);
  m_CursorRow += N;
  m_CursorCol  = 0;
  ClampCursor();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtCPL() {
  DEFAULT_1(N);
  m_CursorRow -= N;
  m_CursorCol  = 0;
  ClampCursor();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtCUB() {
  DEFAULT_1(N);
  m_CursorCol -= N;
  ClampCursor();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtCUF() {
  DEFAULT_1(N);
  m_CursorCol += N;
  ClampCursor();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtCUP() {
  switch (m_VtParserNrParms) {
    case 0:
      // [H
      m_CursorRow = m_OriginMode ? 0 : m_ScrollTop;
      m_CursorCol = 0;
      break;
    case 1:
      m_CursorRow = m_VtParserParms[0] - 1 + (m_OriginMode ? m_ScrollTop : 0);
      m_CursorCol = 0;
      break;
    case 2:
      m_CursorRow = m_VtParserParms[0] - 1 + (m_OriginMode ? m_ScrollTop : 0);
      m_CursorCol = m_VtParserParms[1] - 1;
      break;
    default:
      // Malformed.
      VtParserShowResult();
      qFatal("%s@%s:%d Wrong nr of parameters (%d)",
             __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
             m_VtParserNrParms);
  }
  ClampCursor();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtCUU() {
  DEFAULT_1(N);
  m_CursorRow -= N;
  ClampCursor();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtCUD() {
  DEFAULT_1(N);
  m_CursorRow += N;
  ClampCursor();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDA1() {
  // Let's pretend we're VT220
  // Service class 2 terminal (62) with 
  // 132 columns (1), 
  // no printer port (2), 
  // selective erase (6), 
  // no DRCS (7), 
  // UDK (8), 
  // and I support 7-bit national replacement character sets (9)."
  GenerateCsiReply("?62;1;6;8;9c");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDA2() {
  // VT220 with 1.0 firmware.
  GenerateCsiReply(">1;10;0c");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDA3() {
  qDebug("DA3 is not implemented. Not needed for VT220");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDCH() {
  DEFAULT_1(N);
  int Offset = m_CursorRow * m_NrCols;
  for (int Col = m_CursorCol; Col < m_NrCols; Col++) {
    if (Col + N < m_NrCols) {
      m_Cells[Offset+Col] = m_Cells[Offset + Col + N];
    } else {
      m_Cells[Offset+Col].Char = 0X20;
      m_Cells[Offset+Col].Attr = m_CurrentAttr & ATTR_ERASE_MASK;
    }
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECALN() {
  for (int Row = 0; Row < m_NrRows; Row++) {
    int Offset = Row * m_NrCols;
    for (int Col = 0; Col < m_NrCols; Col++) {
      m_Cells[Offset+Col].Char = 'E';
      m_Cells[Offset+Col].Attr = ATTR_DEFAULT;
    }
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECANSI() {
  m_TermLevel = 1;
  qDebug("%s@%s:%d Conformance level set to %d. Tx8bits : %d",
         __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
         m_TermLevel, m_Transmit8Bit);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECAUPSS() {
  qDebug("DECAUPSS is currently NOP");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECBI() {
  qDebug("DECBI is currently NOP");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECCARA() {
  qDebug("DECCARA is currently NOP");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECDHL(const bool Bottom) {
  m_DWDHs[m_CursorRow].DH = true;
  m_DWDHs[m_CursorRow].DW = true;
  m_DWDHs[m_CursorRow].Bt = Bottom;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECDLD() {

  // First we get the fontname (Intermediates + Final);
  QByteArray FontName;

  int Idx = 0;
  uint16_t Byte = m_VtParserDLDDefinition.at(Idx);
  while ( Byte >= 0X20 and Byte <= 0X2F) {
    FontName.append(Byte);
    Byte = m_VtParserDLDDefinition.at(++Idx);
  }
  if ( Byte < 0X30 or Byte > 0X7E) {
    VtParserShowResult();
    qFatal("%s@%s:%d Unexpected Final 0X%02X ('%c')",
           __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
           Byte,Byte);
  }
  FontName.append(Byte);

  // Now the parameters.
  int FontNumber = m_VtParserNrParms > 0 ? m_VtParserParms[0] : 0;

  if (FontNumber > c_NrSoftCharSets - 1) {
    VtParserShowResult();
    qFatal("%s@%s:%d Incorrect Pfn (Font number) : %d",
           __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
           FontNumber);
  }

  // Link the fontname with the index in m_CharSets we will be using.
  //qDebug("FontName : '%s'",FontName.constData());
  m_CharSetNameIdxMap.insert(FontName,c_NrHardCharSets+FontNumber);

  // Erase
  int EraseCtrl = m_VtParserNrParms > 2 ? m_VtParserParms[2] : 0;
  switch (EraseCtrl) {
    case 0 :
      // Make sure all characters in this set are erased.
      for (int Char = 0; Char < c_NrCharsPerSet; Char++) {
        for (int Row = 0; Row < c_RowsPerGlyph_Vt220; Row++) {
          m_CharSets[c_NrHardCharSets+FontNumber].Vt220Glyphs[Char][Row] = 0;
        }
        for (int Style = 0; Style  < c_NrFontStyles; Style++) {
          for (int Row = 0; Row < c_RowsPerGlyph_Host; Row++) {
            m_CharSets[c_NrHardCharSets+FontNumber].
                HostGlyphs[Style][Char][Row] = 0;
          }
        }
      }
      break;
    case 1 :
      break; // Erase while writing.
    case 2 :
      // Make sure all characters in all sets are erased.
      for (int Set = 0; Set < c_NrSoftCharSets; Set++) {
        for (int Char = 0; Char < c_NrCharsPerSet; Char++) {
          for (int Row = 0; Row < c_RowsPerGlyph_Vt220; Row++) {
            m_CharSets[c_NrHardCharSets + Set].Vt220Glyphs[Char][Row] = 0;
          }
          for (int Style = 0; Style  < c_NrFontStyles; Style++) {
            for (int Row = 0; Row < c_RowsPerGlyph_Host; Row++) {
              m_CharSets[c_NrHardCharSets+FontNumber].
                  HostGlyphs[Style][Char][Row] = 0;
            }
          }
        } 
      }
      break;
  }

  //int Pcms      = m_VtParserNrParms > 3 ? m_VtParserParms[3] : 0;
  //int Pw        = m_VtParserNrParms > 4 ? m_VtParserParms[4] : 0;
  //int Pt        = m_VtParserNrParms > 5 ? m_VtParserParms[5] : 0;

  //qDebug("FontNumber:%d StartChar:%d Pe:%d Pcms:%d Pw:%d Pt:%d\n",
  //       FontNumber,StartChar,Pe,Pcms,Pw,Pt);

  // Now we go for the char.
  // Herein we form the char.
  uint16_t Glyph[c_RowsPerGlyph_Vt220];

  // Clear the glyph for this char.
  for (int i = 0; i < c_RowsPerGlyph_Vt220; i++) {
    Glyph[i] = 0;
  }

  // Char loop.
  int  CharIdx  = m_VtParserNrParms > 1 ? m_VtParserParms[1] : 0; // Pcn.
  int  SixelIdx = 0;
  bool Top      = true;

  for (++Idx; Idx < m_VtParserDLDDefinition.size(); Idx++) {
  
    Byte = m_VtParserDLDDefinition.at(Idx);

    if ( (Byte >= 0X3F) and (Byte <= 0X7E) ) {
      // A sixel.
      uint8_t Sixel = Byte - 0X3F;
      if (Top) {
        for (int i = 0; i < 6; i++) {
          Glyph[i] |= (Sixel & 0X01) << SixelIdx;
          Sixel >>= 1;
        }
      } else {
        for (int i = 6; i < 10; i++) {
          Glyph[i] |= (Sixel & 0X01) << SixelIdx;
          Sixel >>= 1;
        }
      }
      SixelIdx++;

    } else if (Byte == ';') {

      // Next char. Store the previous Glyph and re-init.
      for (int Row = 0; Row < c_RowsPerGlyph_Vt220; Row++) {
        m_CharSets[c_NrHardCharSets+FontNumber].Vt220Glyphs[CharIdx][Row] = 
            Glyph[Row];
      }
      // This works because the 2:1 ratio. XXX TODO
      for (int Style = 0; Style < c_NrFontStyles; Style++) {
        for (int Row = 0; Row < c_RowsPerGlyph_Host; Row++) {
          m_CharSets[c_NrHardCharSets+FontNumber].
              HostGlyphs[Style][CharIdx][Row] = Glyph[Row/2];
        }
      }
      for (int Row = 0; Row < c_RowsPerGlyph_Vt220; Row++) {
        Glyph[Row] = 0;
      }
      CharIdx++;
      SixelIdx = 0;
      Top      = true;

    } else if (Byte == '/') {

      // To bottom half.
      SixelIdx = 0;
      Top      = false;

    } else {
      // Skipping is OK. 0X0A f.i. in the test cases.
      /*
      qDebug("%s@%s:%d Skipping 0X%02X",
             __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
             Byte);
      */
    }
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECDWL() {
  m_DWDHs[m_CursorRow].DH = false;
  m_DWDHs[m_CursorRow].DW = true;
  m_DWDHs[m_CursorRow].Bt = false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECKPAM() {
  m_KeypadNumeric = false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECKPNM() {
  m_KeypadNumeric = true;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECLL() {
  qDebug("DECLL is currently NOP");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECRC() {
  m_CursorRow   = m_SavedCursorRow;
  m_CursorCol   = m_SavedCursorCol;
  m_CurrentAttr = m_SavedAttr;
  m_G[0]        = m_SavedG[0];
  m_G[1]        = m_SavedG[1];
  m_G[2]        = m_SavedG[2];
  m_G[3]        = m_SavedG[3];
  m_GL          = m_SavedGL;
  m_GR          = m_SavedGR;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECREQTPARM() {
  DEFAULT_0(N);
  switch(N) {
    case 0 : 
      // Report parameters
      // Pretty fake report. See http://vt100.net/docs/vt100-ug/chapter3.html
      GenerateCsiReply("2;1;2;64;64;1;0x");
      break;
    case 1 : 
      // Answer only on request.
      // Report parameters
      // Pretty fake report. See http://vt100.net/docs/vt100-ug/chapter3.html
      GenerateCsiReply("3;1;2;64;64;1;0x");
      break;
    default : 
      VtParserShowResult();
      qFatal("%s@%s:%d Unexpected DECREQTPARM(%d)",
             __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
             N);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECSC() {
  m_SavedCursorRow = m_CursorRow;
  m_SavedCursorCol = m_CursorCol;
  m_SavedAttr      = m_CurrentAttr;
  m_SavedG[0]      = m_G[0];
  m_SavedG[1]      = m_G[1];
  m_SavedG[2]      = m_G[2];
  m_SavedG[3]      = m_G[3];
  m_SavedGL        = m_GL;
  m_SavedGR        = m_GR;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECSCA() {
  DEFAULT_0(N);
  switch(N) {
    case 0 : 
    case 2 :
      // Eraseable becomes possible by DECSEL/DECSED
      m_CurrentAttr &= ~ATTR_BIT_DECSCA;
      break;
    case 1 : 
      // Eraseable becomes impossible by DECSEL/DECSED
      m_CurrentAttr |= ATTR_BIT_DECSCA;
      break;
    default : 
      VtParserShowResult();
      qFatal("%s@%s:%d Unexpected DECSCA(%d)",
             __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
             N);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECSCL() {

  m_TermLevel = m_VtParserNrParms ? m_VtParserParms[0] - 60 : 2;

  m_Transmit8Bit = (m_TermLevel > 1);
  if (m_VtParserNrParms == 2) {
    m_Transmit8Bit = (m_VtParserParms[1] == 1) ? false : true;
  }

  qDebug("%s@%s:%d Conformance level set to %d. Tx8bits : %d",
         __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
         m_TermLevel, m_Transmit8Bit);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECSED() {
  EraseInDisplay(true);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECSEL() {
  EraseInLine(true);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECSLRM() {
  qDebug("DECSLRM is currently NOP");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECSTBM() {
   
  int Top;
  int Bot;

  if (m_VtParserNrParms == 0) {
    Top = 0;
    Bot = m_NrRows - 1;
  } else if (m_VtParserNrParms < 2) {
    // Malformed.
    VtParserShowResult();
    qFatal("%s@%s:%d Malformed CSI parms",
           __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__);
    return;
  } else {
    Top = m_VtParserParms[0] - 1;
    Bot = m_VtParserParms[1] - 1;
  }

  Top = MAX(0,MIN(Top,m_NrRows-1));
  Bot = MAX(0,MIN(Bot,m_NrRows-1));

  if (Top >= Bot) {
    // Malformed, but should be accepted as a NOP.
    return;
  }

  m_ScrollTop = Top;
  m_ScrollBot = Bot;

  m_CursorRow = m_OriginMode ? m_ScrollTop : 0;
  m_CursorCol = 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECSTR() {
  // XXX CB TODO
  qDebug("%s@%s:%d Refine me. Just Reset(false) now.",
         __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__);
  Reset(false);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECSWL() {
  m_DWDHs[m_CursorRow].DH = false;
  m_DWDHs[m_CursorRow].DW = false;
  m_DWDHs[m_CursorRow].Bt = false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECTST() {
  qDebug("Powerup test is currently NOP");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDECUDK() {

  int     Ps[3]      = {0,0,0};

  if (m_VtParserNrParms) {
    for (int Parm = 0; Parm < m_VtParserNrParms; Parm++) {
      Ps[Parm] = m_VtParserParms[Parm];
    }
  }

  int     ParseState = 0;
  int     Key        = 0;
  uint8_t Value      = 0;

  QByteArray Definition;

  for (int i=0; i < m_VtParserUDKDefinition.size(); i++) {
    uint8_t Tmp = m_VtParserUDKDefinition.at(i);
    if (ParseState == 0) {
      if (Tmp == '/') ParseState++;
      if (Tmp >= '0' and Tmp <= '9') {
        Key = Key * 10 + (Tmp - '0');
      }
    } else if (ParseState == 1) {
      if (Tmp > 'F') Tmp -= 0X20;  // Lower hex -> Upper hex.
      Value += 0X10 * (Tmp - '0');
      ParseState = 2;
    } else if (ParseState == 2) {
      if (Tmp > 'F') Tmp -= 0X20;  // Lower hex -> Upper hex.
      Value += (Tmp - '0');
      Definition.append(Value);
      Value = 0;
      ParseState = 1;
    }
  }
  
  /*
  qDebug("PS1:%d PS2:%d PS3:%d Key:%d Definition : '%s'",
         Ps[0],Ps[1],Ps[2],Key,Definition.constData());
  */

  // Do nothing if keys are locked.
  if (m_UDKLocked) {
    return;
  }

  // Ps1 0 or default clears all. 1 preserves.
  if (Ps[0] != 1) {
    for (int i = 0; i < 4*20; i++) {
      m_FunctionKeys[i].clear();
    }
  }

  // PS2 0 or default lock the keys. 1 does not lock.
  if (Ps[1] != 1) {
    m_UDKLocked = true;
  }

  // PS3 is the modifier. We just recode to Normal,Shift,Alt,Shift-Alt order.
  int Modifier;
  switch (Ps[2]) {
    case 0 : Modifier = 1; break;
    case 1 : Modifier = 0; break;
    case 2 : Modifier = 1; break;
    case 3 : Modifier = 2; break;
    case 4 : Modifier = 3; break;
    default :
      Modifier = 0; // Just to keep compiler happy.
      VtParserShowResult();
      qFatal("%s@%s:%d : Unexpected modifier %d in '%s'",
             __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
             Ps[2],
             m_VtParserUDKDefinition.constData());
  }

  // Key rebasing. Mind the gap !
  switch (Key) {
    case 11 : Key = 0; break;
    case 12 : Key = 1; break;
    case 13 : Key = 2; break;
    case 14 : Key = 3; break;
    case 15 : Key = 4; break;
    case 17 : Key = 5; break;
    case 18 : Key = 6; break;
    case 19 : Key = 7; break;
    case 20 : Key = 8; break;
    case 21 : Key = 9; break;
    case 23 : Key = 10; break;
    case 24 : Key = 11; break;
    case 25 : Key = 12; break;
    case 26 : Key = 13; break;
    case 28 : Key = 14; break;
    case 29 : Key = 15; break;
    case 31 : Key = 16; break;
    case 32 : Key = 17; break;
    case 33 : Key = 18; break;
    case 34 : Key = 19; break;
    default :
      VtParserShowResult();
      qFatal("%s@%s:%d : Unexpected Key %d in '%s'",
             __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
             Key,
             m_VtParserUDKDefinition.constData());
  }

  m_FunctionKeys[Modifier*20+Key] = Definition;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDL() {
  DEFAULT_1(N);
  for (int Row = m_CursorRow; Row <= m_ScrollBot; Row++) {
    if (Row + N <= m_ScrollBot) {
      int Offset1 = Row * m_NrCols;
      int Offset2 = (Row+N) * m_NrCols;
      for (int Col = 0; Col < m_NrCols; Col++) {
        m_Cells[Offset1+Col] = m_Cells[Offset2+Col];
      }
    } else {
      int Offset = Row * m_NrCols;
      for (int Col = 0; Col < m_NrCols; Col++) {
        m_Cells[Offset+Col].Char = 0X20;
        m_Cells[Offset+Col].Attr = m_CurrentAttr & ATTR_ERASE_MASK;
      }
    }
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtDSR() {
  DEFAULT_0(N);
  char CursorPosString[SHORT_BUFFER_SIZE];
  snprintf(CursorPosString,
           SHORT_BUFFER_SIZE,
           "%d;%dR",
           m_CursorRow+1,m_CursorCol+1);

  char KeyboardLangString[SHORT_BUFFER_SIZE];
  snprintf(KeyboardLangString,
           SHORT_BUFFER_SIZE,
           "?27;%dn",
           m_KeyboardCode);

  switch(N) {

    case 5 : 
      // Report status
      GenerateCsiReply("0n"); // Let's pretend never a malfunction.
      break;

    case 6 :
      // Report cursor
      GenerateCsiReply(CursorPosString);
      break;

    case 15 :
      // Report printer
      GenerateCsiReply("?13n");  // I have no printer.
      break;

    case 25 : 
      // User defined keys.
      if (m_UDKLocked) {
        GenerateCsiReply("?21n"); 
      } else {
        GenerateCsiReply("?20n"); 
      }
      break;

    case 26 :
      // Report keyboard language
      GenerateCsiReply(KeyboardLangString);
      break;

    default:
      VtParserShowResult();
      qFatal("%s@%s:%d Unexpected DSR(%d)",
             __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
             N);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtECH() {
  DEFAULT_1(N);
  int Offset = m_CursorRow * m_NrCols;
  for (int Col = m_CursorCol; Col < m_CursorCol + N and Col < m_NrCols; Col++) {
    m_Cells[Offset+Col].Char = 0X20;
    m_Cells[Offset+Col].Attr = m_CurrentAttr & ATTR_ERASE_MASK;
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtED() {
  EraseInDisplay(false);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// DECSED,ED combination.
void cbConsole::EraseInDisplay(const bool Selective) {
  DEFAULT_0(Command);
  int StartRow;
  int StartCol;
  int EndRow;
  int EndCol;
  switch (Command) {
    case 2 :
      StartRow = 0;
      StartCol = 0;
      EndRow   = m_NrRows;
      EndCol   = m_NrCols;
      break;
    case 1 :
      StartRow = 0;
      StartCol = 0;
      EndRow   = m_CursorRow + 1;
      EndCol   = m_CursorCol + 1;
      break;
    default :
      StartRow = m_CursorRow;
      StartCol = m_CursorCol;
      EndRow   = m_NrRows;
      EndCol   = m_NrCols;
      break;
  }
  for (int Row = StartRow; Row < EndRow; Row++,StartCol=0) {
    int Offset = Row * m_NrCols;
    int LEndCol = (Row == EndRow-1) ? EndCol : m_NrCols;
    for (int Col = StartCol; Col < LEndCol; Col++) {
      if (Selective and (m_Cells[Offset+Col].Attr & ATTR_BIT_DECSCA)) {
        continue;
      }
      m_Cells[Offset+Col].Char = 0X20;
      m_Cells[Offset+Col].Attr = m_CurrentAttr & ATTR_ERASE_MASK;
    }
    ResetDWDH(Row);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtEL() {
  EraseInLine(false);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtHPA() {
  DEFAULT_1(N);
  m_CursorCol  = N - 1;
  ClampCursor();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtHPR() {
  DEFAULT_1(N);
  m_CursorCol  += N;
  ClampCursor();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// DECSEL,EL combination.
void cbConsole::EraseInLine(const bool Selective) {
  DEFAULT_0(Command);
  int Start;
  int End;
  switch (Command) {
    case 1 :
      Start = 0;
      End   = m_CursorCol;
      break;
    case 2 :
      Start = 0;
      End   = m_NrCols-1;
      break;
    default :
      Start = m_CursorCol;
      End   = m_NrCols - 1;
      break;
  }
  int Offset = m_CursorRow * m_NrCols;
  for (int Col = Start; Col <= End; Col++) {
    if (Selective and m_Cells[Offset+Col].Attr & ATTR_BIT_DECSCA) {
      continue;
    }
    m_Cells[Offset+Col].Char = 0X20;
    m_Cells[Offset+Col].Attr = m_CurrentAttr & ATTR_ERASE_MASK;
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtHT() {
  if (m_TabStops[m_CursorCol]) {
    VtPutChar(0X20);
  }
  while ( (not m_TabStops[m_CursorCol]) and 
          (m_CursorCol < GetNrCols(m_CursorRow)-1) ) {
    VtPutChar(0X20);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtHTS() {
  m_TabStops[m_CursorCol] = true;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtHVP() {
  VtCUP(); // HVP seems older version of CUP.
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtICH() {
  DEFAULT_1(N);
  int Offset = m_CursorRow * m_NrCols;
  for (int Col = m_NrCols - 1; Col >= m_CursorCol + N; Col--) {
    m_Cells[Offset+Col] = m_Cells[Offset+Col-N];
  }
  for (int Col = m_CursorCol; Col < m_CursorCol + N; Col++) {
    m_Cells[Offset+Col].Char = 0X20;
    m_Cells[Offset+Col].Attr = m_CurrentAttr;
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtIL() {
  DEFAULT_1(N);
  for (int Row = m_ScrollBot; Row >= m_CursorRow + N; Row--) {
    int Offset1 = Row * m_NrCols;
    int Offset2 = (Row-N) * m_NrCols;
    for ( int Col = 0; Col < m_NrCols; Col++) {
      m_Cells[Offset1+Col] = m_Cells[Offset2+Col];
    }
  }
  for (int Row = m_CursorRow; 
       Row < m_CursorRow + N and Row <= m_ScrollBot; 
       Row++) {
    int Offset = Row * m_NrCols;
    for (int Col = 0; Col < m_NrCols; Col++) {
      m_Cells[Offset+Col].Char = 0X20;
      m_Cells[Offset+Col].Attr = m_CurrentAttr;
    }
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtIND() {
  LineDown();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtLF() {
  LineDown();
  if (m_LNM) {
    m_CursorCol = 0;
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtLS1R() {
  m_GR = 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtLS2() {
  m_GL = 2;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtLS2R() {
  m_GR = 2;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtLS3() {
  m_GL = 3;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtLS3R() {
  m_GR = 3;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtMode(const bool Set) {

  if (m_VtParserNrIChars and m_VtParserIChars[0] == '?') {

    for (int Parm = 0; Parm < m_VtParserNrParms; Parm++) {

      int Parameter = m_VtParserParms[Parm];
      int OldNrCols;

      switch (Parameter) {

        case 1:
          // DECCKM
          m_CursorAppSeq = Set;
          break;
  
        case 2:
          if (not Set) {
            // Go in VT52 mode.
            m_TermLevel    = 0;
            m_Transmit8Bit = false;
            qDebug("%s@%s:%d Conformance level set to %d. Tx8bits : %d",
                    __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
                    m_TermLevel, m_Transmit8Bit);
          }
          break;
  
        case 3:
          // DECCOLM (132 columns mode)
          OldNrCols = m_NrCols;
          m_NrCols  = Set ? 132 : 80;
  
          CB_FREE(m_Cells);
          CB_CALLOC(m_Cells,
                    cbScreenCell*,
                    m_NrRows * m_NrCols,
                    sizeof(cbScreenCell));
          for (int Row = 0; Row < m_NrRows; Row++) {
            int Offset = Row * m_NrCols;
            for (int Col = 0; Col < m_NrCols; Col++) {
              m_Cells[Offset+Col].Char = 0X20;
              m_Cells[Offset+Col].Attr = m_CurrentAttr & ATTR_ERASE_MASK;
            }
          }
  
          m_CursorRow   = 0;
          m_CursorCol   = 0;
  
          // Tabstops must survive this operation.
          bool *TabStops;
          CB_CALLOC(TabStops,bool*,m_NrCols,sizeof(bool));
          for (int Col = 0; Col < MIN(m_NrCols,OldNrCols); Col++) {
            TabStops[Col] = m_TabStops[Col];
          }
          CB_FREE(m_TabStops);
          m_TabStops = TabStops;
            
          // 
          OnSizeChange(m_NrRows,m_NrCols);
          break;
        
        case 4:
          //DECSCLM - Scroll Mode : Smooth.
          m_SmoothScroll = Set;
          break;
  
        case 5:
          //DECSCNM - Screen Mode : Invert.
          m_InverseMode = Set;
          break;
  
        case 6:
          //DECOM - Origin Mode, line 1 is relative to scroll region
          m_OriginMode = Set;
          m_CursorRow  = Set ? m_ScrollTop : 0;
          m_CursorCol  = 0;
          break;
  
        case 7:
          //DECAWM - Autowrap Mode
          m_AutoWrap = Set;
          break;
  
        case 8:
          // DECARM (auto repeat)
          // Currently NOP
          qDebug("DECARM(%d) is currently NOP",Set);
          break;
       
        case 25:
          // DECTCEM. (text cursor visible)
          m_CursorVisible = Set;
          break;

        case 40:
        case 45:
          // Issued by test cases for xterm. No VT100 meaning. Just absorb.
          qDebug("Mode command '?%d(%d)' is currently NOP (xterm only)",
                 Parameter,Set);
          break;
  
        case 42:
          m_DECNRCM = Set;
          // qDebug("m_DECNRCM set to  %d",Set);
          break;

        case 66:
          // DECNKM (Numeric keypad)
          m_KeypadNumeric = not Set;
          break;

        case 67 :
          // DECBKM
          qDebug("DECBKM(%d) is currently NOP",Set);
          break;

        case 98 :
          // DECARSM
          qDebug("DECARSM(%d) is currently NOP",Set);
          break;

        case 101 :
          // DECCANSM
          qDebug("DECCANSM(%d) is currently NOP",Set);
          break;

        case 109 :
          // DECCAPSLK
          qDebug("DECCAPSLK(%d) is currently NOP",Set);
          break;
  
        default:
          VtParserShowResult();
          qFatal("%s@%s:%d Unexpected parameter %d",
                 __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
                 Parameter);
      }
    }

  } else if (not m_VtParserNrIChars) {

    for (int Parm = 0; Parm < m_VtParserNrParms; Parm++) {

      int Parameter = m_VtParserParms[Parm];

      switch(Parameter) {
    
        case 3 :
          // CRM Mode
          qDebug("CRM(%d) is currently NOP",Set);
          break;

        case 4:
          // IRM Mode
          m_InsertMode = Set;
          break;
  
        case 12:
          // Local echo mode.
          m_LocalEcho = not Set; // Yes, weird but correct.
          break;
  
        case 20:
          // LNM Mode.
          m_LNM = Set;
          break;
      
        default:
          VtParserShowResult();
          qFatal("%s@%s:%d Unexpected parameter %d",
                 __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
                 Parameter);
      }
    }

  } else {

    VtParserShowResult();
    qFatal("%s@%s:%d Unexpected SM/RM",
           __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__);
  }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtNEL() {
  LineDown();
  m_CursorCol = 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtRI() {
  LineUp();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtRIS() {
  // XXX CB TODO
  qDebug("%s@%s:%d Refine me. Just Reset(true) now.",
         __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__);
  Reset(true);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtS7C1T() {
  m_Transmit8Bit = false;
  qDebug("%s@%s:%d Conformance level set to %d. Tx8bits : %d",
         __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
         m_TermLevel, m_Transmit8Bit);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtS8C1T() {
  if (m_TermLevel > 1) {
    m_Transmit8Bit = true;
    qDebug("%s@%s:%d Conformance level set to %d. Tx8bits : %d",
           __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
           m_TermLevel, m_Transmit8Bit);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtSCS(const int Set, const uint8_t Char) {
  QByteArray Name;
  for (int i = 1; i < m_VtParserNrIChars; i++) {
    Name.append(m_VtParserIChars[i]);
  }
  Name.append(Char);
  m_G[Set] = Name;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtSGR() {

  if (m_VtParserNrParms == 0) {
    m_CurrentAttr = ATTR_DEFAULT;
    return;
  }

  for (int Parm = 0; Parm < m_VtParserNrParms; Parm++) {
  
    int Parameter = m_VtParserParms[Parm];

    switch (Parameter) {

      case 0:  m_CurrentAttr  =  ATTR_DEFAULT;       break;
      case 1:  m_CurrentAttr |=  ATTR_BIT_BOLD;      break;
      case 2:  m_CurrentAttr |=  ATTR_BIT_DIM;       break;
      case 3:  m_CurrentAttr |=  ATTR_BIT_ITALIC;    break;
      case 4:  m_CurrentAttr |=  ATTR_BIT_UNDERLINE; break;
      case 5:  m_CurrentAttr |=  ATTR_BIT_BLINK;     break;
      case 7:  m_CurrentAttr |=  ATTR_BIT_NEGATIVE;  break;
      case 8:  m_CurrentAttr |=  ATTR_BIT_INVISIBLE; break;
      case 22: m_CurrentAttr &= ~ATTR_BIT_BOLD;
               m_CurrentAttr &= ~ATTR_BIT_DIM;       break;
      case 24: m_CurrentAttr &= ~ATTR_BIT_UNDERLINE; break;
      case 25: m_CurrentAttr &= ~ATTR_BIT_BLINK;     break;
      case 27: m_CurrentAttr &= ~ATTR_BIT_NEGATIVE;  break;
      case 28: m_CurrentAttr &= ~ATTR_BIT_INVISIBLE; break;
      case 30:
      case 31:
      case 32:
      case 33:
      case 34:
      case 35:
      case 36:
      case 37:
        m_CurrentAttr = (m_CurrentAttr & 0XFFF0)|(Parameter-30);
        break;
      case 40:
      case 41:
      case 42:
      case 43:
      case 44:
      case 45:
      case 46:
      case 47:
        m_CurrentAttr = (m_CurrentAttr & 0XFF0F)|((Parameter-40)<<4);
        break;
      case 39:
        m_CurrentAttr &= 0XFFF0;
        m_CurrentAttr |= (ATTR_DEFAULT & 0X000F);
        break;
      case 49:
        m_CurrentAttr &= 0XFF0F;
        m_CurrentAttr |= (ATTR_DEFAULT & 0X00F0);
        break;
      default:
        VtParserShowResult();
        qFatal("%s@%s:%d SGR(%d) : Not implemented yet",
               __PRETTY_FUNCTION__, __FILE__, __LINE__,
               Parameter);
    }
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtSI() {
  m_GL = 0;
  //qDebug("%d : m_GL now %d",__LINE__,m_GL);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtSO() {
  m_GL = 1;
  //qDebug("%d : m_GL now %d",__LINE__,m_GL);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtSS2() {
  m_SingleShifting = true;
  m_OldGL = m_GL;
  m_GL = 2;
  //qDebug("%d : m_GL now %d",__LINE__,m_GL);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtSS3() {
  m_SingleShifting = true;
  m_OldGL = m_GL;
  m_GL = 3;
  //qDebug("%d : m_GL now %d",__LINE__,m_GL);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtTBC() {
  DEFAULT_0(N);
  switch(N) {
    case 0 : 
      m_TabStops[m_CursorCol] = false;
      break;
    case 1 : 
    case 2 : 
      // Accepted NOPS.
      qDebug("TBC(%d) is currently NOP",N);
      break;
    case 3 : 
      for (int Col = 0; Col < GetNrCols(m_CursorRow); Col++) {
        m_TabStops[Col] = false;
      }
      break;
    default : 
      VtParserShowResult();
      qFatal("%s@%s:%d Unexpected TBC(%d)",
             __PRETTY_FUNCTION__, __FILE__, __LINE__,
             N);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtVPA() {
  DEFAULT_1(N);
  m_CursorRow  = N - 1;
  ClampCursor();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::VtVPR() {
  DEFAULT_1(N);
  m_CursorRow += N;
  ClampCursor();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::Vt52CUD() {
  if (m_CursorRow < m_NrRows-1) {
    m_CursorRow++;
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::Vt52CUL() {
  if (m_CursorCol > 0) {
    m_CursorCol--;
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::Vt52CUR() {
  if (m_CursorCol < m_NrCols-1) {
    m_CursorCol++;
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::Vt52CUU() {
  if (m_CursorRow > 0) {
    m_CursorRow--;
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::Vt52DCA(const uint8_t Char) {
  if (not m_Vt52EscYCollectingRow and not m_Vt52EscYCollectingCol) {
    m_Vt52EscYCollectingRow = true;
    m_Vt52EscYCollectingCol = false;
  } else if (m_Vt52EscYCollectingRow) {
    m_Vt52EscYCollectingRow = false;
    m_Vt52EscYCollectingCol = true;
    m_CursorRow = Char - 0X20;
  } else {
    m_Vt52EscYCollectingCol = false;
    m_Vt52EscYCollectingRow = false;
    m_CursorCol = Char - 0X20;
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::Vt52ED() {
  for (int Row = m_CursorRow; Row < m_NrRows; Row++) {
    int Offset = Row * m_NrCols;
    for (int Col = (Row == m_CursorRow)?m_CursorCol:0; Col < m_NrCols; Col++) {
      m_Cells[Offset+Col].Char = 0X20;
      m_Cells[Offset+Col].Attr = m_CurrentAttr & ATTR_ERASE_MASK;
    }
    ResetDWDH(Row);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::Vt52EL() {
  int Offset = m_CursorRow * m_NrCols;
  for (int Col = m_CursorCol; Col < m_NrCols; Col++) {
    m_Cells[Offset+Col].Char = 0X20;
    m_Cells[Offset+Col].Attr = m_CurrentAttr & ATTR_ERASE_MASK;
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::Vt52ESA() {
  m_VT52_SSA = false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::Vt52Home() {
  m_CursorRow = 0;
  m_CursorCol = 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::Vt52RI() {
  LineUp();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::Vt52SSA() {
  m_VT52_SSA = true;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::PaintScreen(QWidget* TheWidget) {

  // qDebug("%s@%s:%d",__PRETTY_FUNCTION__,CB_BASENAME(__FILE__),__LINE__);

  // Optimized to time.
  /*
  QTime OperationTimer;
  OperationTimer.start();
  */

  QPainter Painter(TheWidget);
  Painter.setBackgroundMode(Qt::OpaqueMode);
  
  int  RowsPerGlyph;
  int  ColsPerGlyph;
  int  ImgRowsPerChar;
  bool BlankInterline;
  int  UnderlineRow;
 
  if (m_FontMode == c_FontMode_Vt220) {
    BlankInterline = m_BlankInterline;
    UnderlineRow   = m_UnderlineRow;
    RowsPerGlyph   = c_RowsPerGlyph_Vt220;
    ColsPerGlyph   = c_ColsPerGlyph_Vt220;
    if (m_BlankInterline) {
      ImgRowsPerChar = 2* c_RowsPerGlyph_Vt220;
    } else {
      ImgRowsPerChar = c_RowsPerGlyph_Vt220;
    }
  } else {
    BlankInterline = false;
    RowsPerGlyph   = c_RowsPerGlyph_Host;
    ColsPerGlyph   = c_ColsPerGlyph_Host;
    ImgRowsPerChar = c_RowsPerGlyph_Host;
    UnderlineRow   = c_RowsPerGlyph_Host - 2;
  }

  int CharRow       = 0;
  int CharRowOffset = 0;
  int GlyphRow      = 0;
  int DotRow        = 0;  // Real.
  int ImgRow        = 0;  // Within image. Can be != DotRow  (blank interlines)

  const int EndImgRow = m_NrRows * ImgRowsPerChar;

  while (ImgRow < EndImgRow) {

    uint32_t* ScanLine = (uint32_t*) m_ScreenImage->scanLine(ImgRow);

    if (BlankInterline) {
      DotRow = ImgRow >> 1;
      // Interline gap.
      if (ImgRow & 1) {
        memset(ScanLine,0,4*m_NrCols*c_ColsPerChar);
        ImgRow++;
        continue;
      }
    } else {
      DotRow = ImgRow;
    }

    cbDWDH DWDH = m_DWDHs[CharRow];

    bool DH = DWDH.DH;
    bool DW = DWDH.DW or DH; // XXX DH implies DW. 
    bool Bt = DWDH.Bt and DH;

    cbScreenCell Cell = {0X0000,0X0000};
    uint16_t Attr = 0X00;
    uint16_t Char = 0X00;
    int      Set  = 0;

    int  CharCol   = 0;
    int  GlyphCol  = 0;
    bool PrevDotOn = false; // Dotstretch : http://vt100.net/dec/vt220/glyphs
    int  DotCol    = 0;

    uint16_t GlyphSR = 0; // New glyphs will be or'ed into (glyphs with bearing)
   
    int ColsPerChar = DW ? 2 * c_ColsPerChar : c_ColsPerChar;

    const int NrDotCols = m_NrCols * c_ColsPerChar;

    while (DotCol < NrDotCols) {

      // Fetch new character and its attributes.
      if (GlyphCol == 0) {
        Cell = m_Cells[CharRowOffset + CharCol];
        Attr = Cell.Attr;
        Char = Cell.Char & 0X00FF;
        Set  = (Cell.Char & 0XFF00) >> 8;
        if (Char < 0X20) {
          qFatal("%s@%s:%d CHECKME",
                 __PRETTY_FUNCTION__,CB_BASENAME(__FILE__),__LINE__);
        }
        Char = Char - 0X20;
        Char = (Attr & ATTR_BIT_INVISIBLE) ? 0X00 : Char;

        // Rather safe than sorry.
        if (Char >= 96) {
          qFatal("%s@%s:%d CHECKME Char : %d Cell.Char : 0X%04X",
                 __PRETTY_FUNCTION__,CB_BASENAME(__FILE__),__LINE__,
                 Char,Cell.Char);
        }
  
        // Unattributed dot info for this position.
        int GlyphAddress;
        GlyphAddress  = GlyphRow + ( (DH and Bt) ? RowsPerGlyph/2 : 0);
  
        //uint16_t GlyphWord;
        if (m_FontMode == c_FontMode_Vt220) {
          uint16_t GlyphWord = m_CharSets[Set].Vt220Glyphs[Char][GlyphAddress];
          GlyphSR |= GlyphWord;
          if (GlyphWord & 0X80) {
            GlyphSR |= 0X300; // 2 bit extending.
          }
        } else {
          if (Attr & ATTR_BIT_BOLD) {
            GlyphSR |= m_CharSets[Set].
                       HostGlyphs[c_FontStyle_Bold][Char][GlyphAddress];
          } else if (Attr & ATTR_BIT_ITALIC) {
            GlyphSR |= m_CharSets[Set].
                       HostGlyphs[c_FontStyle_Italic][Char][GlyphAddress];
          } else {
            GlyphSR |= m_CharSets[Set].
                       HostGlyphs[c_FontStyle_Normal][Char][GlyphAddress];
          }
        }
      }

      bool GDotOn = (GlyphSR & 1);

      if (not DW or (DotCol & 1)) GlyphSR >>= 1;

      // Underline implementation. 9th row is according to spec the underline.
      if ( (Attr & ATTR_BIT_UNDERLINE) and (GlyphRow == UnderlineRow)) {
        GDotOn = true; 
      }

      // Blink implementation.
      if ((Attr & ATTR_BIT_BLINK) and not m_CursorPhaseOn) {
        GDotOn = false;
      }

      // Dotstretch implementation.
      bool DotOn;
      if (m_FontMode == c_FontMode_Vt220) {
        DotOn = PrevDotOn or GDotOn; // Dotstretch.
      } else {
        DotOn = GDotOn;
      }
      PrevDotOn   = GDotOn;
   
      // Cursor drawing if there.
      if (m_CursorVisible         and 
          m_CursorPhaseOn         and 
          m_CursorRow == CharRow  and 
          m_CursorCol == CharCol  and
          GlyphCol < ColsPerGlyph and
          // -1 To avoid descendent cursor.
          GlyphRow < RowsPerGlyph - 1) {
        if (TheWidget->hasFocus()) {
          // Full block cursor by inverting underlying.
          DotOn = !DotOn;
        } else {
          // Outline block cursor by inverting underlying.
          if ( GlyphCol == 0 or GlyphCol == ColsPerGlyph - 1 or
               // -2 to avoid descendent cursor. See -1 higher.
               GlyphRow == 0 or GlyphRow == RowsPerGlyph - 2 ) {
            DotOn = !DotOn;
          }
        }
      }

      // Color info and Negative/Invers
      uint8_t FG = Attr & 0X000F;
      uint8_t BG = (Attr & 0X00F0)>>4;
      bool Invert = m_InverseMode xor (bool) (Attr & ATTR_BIT_NEGATIVE);
      if (Invert) {
        uint8_t Tmp = FG;
        FG = BG;
        BG = Tmp;
      }
      // Remark the precalculations on color values for speed.
      // Especially with respect to intensity calculations.

      // Bold implementation by higher intensity for Vt220.
      QColor Color;
      if ((Attr & ATTR_BIT_BOLD) and m_FontMode == c_FontMode_Vt220) {
        Color = m_Colors_Bold[DotOn ? FG:BG];
      } else if (Attr & ATTR_BIT_DIM) {
        // Dim implementation.
        Color = m_Colors_Dim[DotOn ? FG:BG];
      } else {
        // Just intensity modulated.
        Color = m_Colors_Intensity[DotOn ? FG:BG];
      }

      // And that's it for this dot.
      ScanLine[DotCol] = Color.rgb();

      // Next DotCol
      DotCol++;

      // GlyphCol only increments every second column in DW mode.
      if (not DW or (DotCol & 1)) {
        GlyphCol++;
      }

      // DotCol arrived at new character (mind : ColsPerChar can be 2*c_ !)?
      if ( (DotCol % ColsPerChar) == 0) {
        CharCol++;
        GlyphCol = 0;
      }

    }

    // Next row. (if we are here, we are not the blanking line !)
    ImgRow++;
    DotRow++;
    // GlyphRow only increments every second row in DH mode.
    if (not DH or ((DotRow & 1)==0)) {
      GlyphRow++;
    }
    // DotRow arrived at new character ? (RowsPerChar can be 2*c_ in DH)
    if ( (DotRow % RowsPerGlyph) == 0) {
      CharRow++;
      CharRowOffset += m_NrCols;
      GlyphRow = 0;
    }
  }
 
  // Map the image to the painter.
  // In case we used BlankInterline, sizes will be equal.
  // Otherwise it will be scaled vertically by a factor 2.
  QRectF TgtR(0, 0, 
              m_NrCols * c_ColsPerChar , 
              // 2 for interline scan gap.
              m_NrRows * c_RowsPerChar);
  QRectF SrcR(0, 0, 
              m_NrCols * c_ColsPerChar , 
              // 2 for interline scan gap.
              m_NrRows * ImgRowsPerChar);
  Painter.drawImage(TgtR,*m_ScreenImage,SrcR);

  /*
  int ElapsedTime = OperationTimer.elapsed(); // Get the printing out the time.
  qDebug("PaintScreen took %d ms",ElapsedTime);
  */
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool cbConsole::eventFilter(QObject* Object,  QEvent* Event) 
    {
    QWidget*      TheWidget  = qobject_cast <QWidget *> (Object);
    QEvent::Type  EventType  = Event->type();
  
    if (EventType == QEvent::Paint) 
        {
        PaintScreen(TheWidget);
        // Just note we clean, and we're done.
        m_Dirty = false;
        return true;
        }
  
    if (EventType == QEvent::KeyPress) 
        {
        QKeyEvent *KeyEvent = static_cast <QKeyEvent *> (Event);
    
        QString Text      = KeyEvent->text();
        int     TextSize  = Text.size();
        int     KeyCode   = KeyEvent->key();
        int     Modifiers = KeyEvent->modifiers();
    
        bool    Tx8Bit    = m_Transmit8Bit and not m_DECNRCM;
    
        QList <uint8_t> KeyStrokes; // For this key event.

        // qDebug("Text:'%s'",C_STRING(Text));
    
        if (TextSize) 
            {
            for (int i = 0; i < TextSize; i++) 
                {
                uint16_t AsciiValue = Text[i].unicode();
        
                if (AsciiValue > 0XFF) 
                    {
                    qDebug("Not expected AsciiValue %04X", AsciiValue);
                    continue;
                    }
          
                /*
                qDebug("%s@%s:%d '%s' Character '%c' - Ascii : %02X - IsPrint : %d",
                       __PRETTY_FUNCTION__,
                       CB_BASENAME(__FILE__),
                       __LINE__,
                       C_STRING(Text),
                       AsciiValue,
                       AsciiValue,
                       isprint(AsciiValue));
                */
        
                // National replacement here.
                if (m_DECNRCM) 
                    {
                    if (m_KeyMap.contains(AsciiValue)) 
                        {
                        /*
                        qDebug("Due to DECNRCM replacing %04X by %04X",
                               AsciiValue,
                               m_KeyMap.value(AsciiValue));
                        */
                        AsciiValue = m_KeyMap.value(AsciiValue);
                        } 
                    else 
                        { 
                        AsciiValue &= 0X7F;
                        }
                    }
        
                // Special keypad cases.
                if ((Modifiers & Qt::KeypadModifier) and not m_KeypadNumeric) 
                    {
                    if (AsciiValue >= '0' and AsciiValue <= '9') 
                        {
                        KeyStrokes.append(0X1B);
                        if (m_TermLevel == 0) 
                            {
                            KeyStrokes.append('?');
                            } 
                        else 
                            {
                            KeyStrokes.append('O');
                            }
                        KeyStrokes.append('p' + AsciiValue - '0');
                        } 
                    else if (AsciiValue == '-') 
                        {
                        KeyStrokes.append(0X1B);
                        if (m_TermLevel == 0) 
                            {
                            KeyStrokes.append('?');
                            } 
                        else 
                            {
                            KeyStrokes.append('O');
                            }
                        KeyStrokes.append('m');
                        } 
                    else if (AsciiValue == '.') 
                        {
                        KeyStrokes.append(0X1B);
                        if (m_TermLevel == 0) 
                            {
                            KeyStrokes.append('?');
                            } 
                        else 
                            {
                            KeyStrokes.append('O');
                            }
                        KeyStrokes.append('n');
                        } 
                    else if (AsciiValue == 0X0D) 
                        {
                        KeyStrokes.append(0X1B);
                        if (m_TermLevel == 0) 
                            {
                            KeyStrokes.append('?');
                            } 
                        else 
                            {
                            KeyStrokes.append('O');
                            }
                        KeyStrokes.append('M');
                        } 
                    else 
                        {
                        KeyStrokes.append(AsciiValue);
                        }
          
                    // Editing keys as ALT-FirstLetter.
                    } 
                else if ( (m_TermLevel > 1) and (Modifiers & Qt::AltModifier) and 
                          (  (AsciiValue == 'f') or
                             (AsciiValue == 'i') or
                             (AsciiValue == 'r') or
                             (AsciiValue == 's') or
                             (AsciiValue == 'p') or
                             (AsciiValue == 'n') ) ) 
                    {
                    if (Tx8Bit) 
                        {
                        KeyStrokes.append(0X9B);
                        } 
                    else 
                        {
                        KeyStrokes.append(0X1B);
                        KeyStrokes.append('[');
                        }
                    switch (AsciiValue) 
                        {
                        case 'f' : KeyStrokes.append(0X31); break;
                        case 'i' : KeyStrokes.append(0X32); break;
                        case 'r' : KeyStrokes.append(0X33); break;
                        case 's' : KeyStrokes.append(0X34); break;
                        case 'p' : KeyStrokes.append(0X35); break;
                        case 'n' : KeyStrokes.append(0X36); break;
                        }
                    KeyStrokes.append(0X7E);
              
                    } 
                else 
                    {
                    // Normal case for 'ascii' keys.
                    KeyStrokes.append(AsciiValue);
                    }
                } 
            } 
        else 
            {
            // All special non-ascii (text()) keys.
      
            const char* KeyString;
      
            // Convert Modifiers to 0,1,2,3 index (normal,shift,alt,shift-alt)
            int NumModifier = 0;
            if ( (Modifiers & Qt::ShiftModifier) and !(Modifiers & Qt::AltModifier) ) 
                {
                NumModifier = 1;
                } 
            else if ( (Modifiers & Qt::AltModifier) and !(Modifiers & Qt::ShiftModifier) ) 
                {
                NumModifier = 2;
                } 
            else if ( (Modifiers & Qt::AltModifier) and (Modifiers & Qt::ShiftModifier) ) 
                {
                NumModifier = 3;
                }
      
            // Only sensible for F1..F20 !
            int FunctionIdx = NumModifier*20 + KeyCode - Qt::Key_F1;
            FunctionIdx = MAX(0,MIN(79,FunctionIdx));
            bool HaveKeyDef = (m_FunctionKeys[FunctionIdx].size() != 0);
            const char* KeyDef = HaveKeyDef ? m_FunctionKeys[FunctionIdx].constData() : NULL;
      
            switch (KeyCode) 
                {
                // Cursor keys.
                case Qt::Key_Up:
                    KeyString = m_TermLevel ? 
                        (Tx8Bit ? (m_CursorAppSeq ? "\x8F""A" : "\x9B""A" ) : 
                                  (m_CursorAppSeq ? "\x1B""OA": "\x1B""[A") )
                        : "\x1B""A";
                    break;
                case Qt::Key_Down:
                    KeyString = m_TermLevel ? 
                        (Tx8Bit ? (m_CursorAppSeq ? "\x8F""B" : "\x9B""B" ) : 
                                  (m_CursorAppSeq ? "\x1B""OB": "\x1B""[B") )
                        : "\x1B""B";
                    break;
                case Qt::Key_Right:
                    KeyString = m_TermLevel ? 
                        (Tx8Bit ? (m_CursorAppSeq ? "\x8F""C" : "\x9B""C" ) : 
                                  (m_CursorAppSeq ? "\x1B""OC": "\x1B""[C") )
                        : "\x1B""C";
                    break;
                case Qt::Key_Left:
                    KeyString = m_TermLevel ? 
                        (Tx8Bit ? (m_CursorAppSeq ? "\x8F""D" : "\x9B""D" ) : 
                                  (m_CursorAppSeq ? "\x1B""OD": "\x1B""[D") )
                        : "\x1B""D";
                    break;
          
                  // The first four Function keys. PF1..PF4 mapped.
                case Qt::Key_F1:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x8F""P" : "\x1B""OP") 
                                                : "\x1B""P";
                        }
                    break;
                case Qt::Key_F2:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x8F""Q" : "\x1B""OQ") 
                                                : "\x1B""Q";
                        }
                    break;
                case Qt::Key_F3:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x8F""R" : "\x1B""OR") 
                                                : "\x1B""R";
                        }
                    break;
                case Qt::Key_F4:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x8F""S" : "\x1B""OS") 
                                                : "\x1B""S";
                        }
                    break;
          
                // F5 has some intermediate status. Lets return csi 15 though.
                case Qt::Key_F5:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""15~" : "\x1B""[15~") : "";
                        }
                    break;
          
                // F6 .. F20. But mind the gaps !
                case Qt::Key_F6:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""17~" : "\x1B""[17~") : "";
                        }
                    break;
          
                case Qt::Key_F7:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""18~" : "\x1B""[18~") : "";
                        }
                    break;
          
                case Qt::Key_F8:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""19~" : "\x1B""[19~") : "";
                        }
                    break;
          
                case Qt::Key_F9:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""20~" : "\x1B""[20~") : "";
                        }
                    break;
          
                case Qt::Key_F10:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""21~" : "\x1B""[21~") : "";
                        }
                    break;
          
                case Qt::Key_F11:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""23~" : "\x1B""[23~") : "";
                        }
                    break;
          
                case Qt::Key_F12:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""24~" : "\x1B""[24~") : "";
                        }
                    break;
          
                case Qt::Key_F13:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""25~" : "\x1B""[25~") : "";
                        }
                    break;
          
                case Qt::Key_F14:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""26~" : "\x1B""[26~") : "";
                        }
                    break;
          
                case Qt::Key_F15:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""28~" : "\x1B""[28~") : "";
                        }
                    break;
          
                case Qt::Key_F16:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""29~" : "\x1B""[29~") : "";
                        }
                    break;
          
                  case Qt::Key_F17:
                    if (HaveKeyDef) {
                      KeyString = KeyDef;
                    } else {
                      KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""31~" : "\x1B""[31~") : "";
                    }
                    break;
          
                case Qt::Key_F18:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""32~" : "\x1B""[32~") : "";
                        }
                    break;
          
                case Qt::Key_F19:
                    if (HaveKeyDef) {
                      KeyString = KeyDef;
                    } else {
                      KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""33~" : "\x1B""[33~") : "";
                    }
                    break;
          
                case Qt::Key_F20:
                    if (HaveKeyDef) 
                        {
                        KeyString = KeyDef;
                        } 
                    else 
                        {
                        KeyString = m_TermLevel ? (Tx8Bit ? "\x9B""34~" : "\x1B""[34~") : "";
                        }
                    break;
        
                default:
                    qDebug("Unhandled KeyCode %04X", KeyCode);
                    KeyString = "";
                }
            
            for (const char* Ptr = KeyString; *Ptr; Ptr++) 
                {
                KeyStrokes.append(*Ptr);
                }
            }
    
        // And now collect the local generated Keystrokes and issue them.
        for (int i = 0; i < KeyStrokes.size(); i++) 
            {
            uint8_t Value = KeyStrokes.at(i);
            m_KeyStrokesMutex.lock();
            m_KeyStrokes.append(Value);
            m_KeyStrokesMutex.unlock();
            if (m_LocalEcho) 
                {
                //qDebug("Echoing %02X due to m_LocalEcho",AsciiValue);
                Write(0,Value);
                }
            if (m_LNM and Value == 0X0D) 
                {
                m_KeyStrokesMutex.lock();
                m_KeyStrokes.append(0X0A);
                m_KeyStrokesMutex.unlock();
                if (m_LocalEcho) 
                    {
                    Write(0,0X0A);
                    }
                }
            }
    
        // Handled.
        return true;
        }
  
    // Unhandled.
    return QObject::eventFilter(Object,Event);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::OnSizeChange(const int NrRows, const int NrCols) 
    {
    m_NrRows = NrRows;
    m_NrCols = NrCols;

    m_MainWindow->QF_Console->setMinimumSize(m_NrCols * c_ColsPerChar, 
                                             // 2 for interline scan gap.
                                             m_NrRows * c_RowsPerChar);

    delete m_ScreenImage;
    m_ScreenImage = new QImage(m_NrCols * c_ColsPerChar, 
                               // 2 for interline scan gap.
                               m_NrRows * c_RowsPerChar,
                               QImage::Format_RGB32);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::OnFontModeVt220Changed(const int Check) {

  // Could have used the m_Vt220Fonts directly, but
  // vaguely have plan to add fonts.
  if (Check) {
    m_FontMode = c_FontMode_Vt220;
  } else {
    m_FontMode = c_FontMode_Host;
  }
 
  QSettings* UserSettings = Emu8080->m_UserSettings;
  UserSettings->setValue("Console/Vt220Fonts",(m_FontMode == c_FontMode_Vt220));

  m_MainWindow->QCB_FontModeVt220->setChecked(Check);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::OnBlankInterlineChanged(const int Check) {

  m_BlankInterline = Check;
 
  QSettings* UserSettings = Emu8080->m_UserSettings;
  UserSettings->setValue("Console/BlankInterline",m_BlankInterline);

  m_MainWindow->QCB_BlankInterline->setChecked(m_BlankInterline);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::OnIntensityChanged(const int Value) 
    {
    m_Intensity = Value;
   
    QSettings* UserSettings = Emu8080->m_UserSettings;
    UserSettings->setValue("Console/Intensity", m_Intensity);
  
    m_MainWindow->QS_Intensity->setValue(m_Intensity);
  
    m_Colors_Intensity.clear();
    m_Colors_Dim.clear();
    m_Colors_Bold.clear();
    for (int i = 0; i < m_Colors.size(); i++) 
        {
        QColor Color = m_Colors.at(i);
        qreal h,s,v,v1,v2,v3;
        Color.getHsvF(&h,&s,&v);
        v1 = v * m_Intensity / 100;
        v2 = MIN(1.0,v1*2);
        v3 = v1*0.5;
        Color.setHsvF(h,s,v1);
        m_Colors_Intensity << Color;
        Color.setHsvF(h,s,v2);
        m_Colors_Bold << Color;
        Color.setHsvF(h,s,v3);
        m_Colors_Dim << Color;
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::OnUnderlineRowChanged(const int Check) {

  m_UnderlineRow = Check ? 9 : 8;
 
  QSettings* UserSettings = Emu8080->m_UserSettings;
  UserSettings->setValue("Console/UnderlineRow9",Check ? true : false);

  m_MainWindow->QCB_UnderlineRow9->setChecked(m_BlankInterline);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::OnUpdateConsole() 
    {
    // On a per timer basis for performance (otherwise on each char output
    // it can be way too slow).
    if (m_Dirty)  
        {
        m_MainWindow->QF_Console->repaint();
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::OnColorSchemeChanged(const int Scheme) 
    {
    m_ColorScheme = Scheme;
  
    QSettings* UserSettings = Emu8080->m_UserSettings;
    UserSettings->setValue("Console/ColorScheme",m_ColorScheme);

    m_MainWindow->QCB_ConsoleColorScheme->setCurrentIndex(m_ColorScheme);
  
    switch (m_ColorScheme) 
        {
        case 0 :
            m_Foreground  = Qt::green;
            m_Background  = Qt::black;
            break;
        case 1 :
            m_Foreground  = Qt::white;
            m_Background  = Qt::black;
            break;
        case 2 :
            m_Foreground  = QColor(0XFF,0XBF,0X00);
            m_Background  = Qt::black;
            break;
        }
  
    m_Colors[8] = m_Foreground;
    m_Colors[9] = m_Background;
  
    OnIntensityChanged(m_Intensity); // To force recalculation of m_Colors_*
  
    m_MainWindow->QF_Console->repaint();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::OnBlink() {

  m_CursorPhaseOn = not m_CursorPhaseOn;
  m_MainWindow->QF_Console->repaint();
  m_Dirty = false;

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void vttest(cbConsole* Console);

void cbConsole::OnTest() {

  /*
  qDebug("%s@%s:%d",
         __PRETTY_FUNCTION__,
         CB_BASENAME(__FILE__),
         __LINE__);
  */

  #ifdef CB_WITH_VTTEST
    if (RunningVtTest()) {
      qWarning("First stop running test before entering again");
      return;
    }
  
    vttest(this);
  #endif
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::LineDown() {

  m_CursorRow++;
  if (m_CursorRow <= m_ScrollBot) return;
 
  m_CursorRow = m_ScrollBot;

  for (int Row = m_ScrollTop; Row < m_ScrollBot; Row++) {
    int Offset1 = Row * m_NrCols;
    int Offset2 = (Row+1) * m_NrCols;
    for ( int Col = 0; Col < m_NrCols; Col++) {
      m_Cells[Offset1+Col] = m_Cells[Offset2+Col];
    }
    m_DWDHs[Row] = m_DWDHs[Row+1];
  }

  for (int Col = 0; Col < m_NrCols; Col++) {
    int Offset = m_ScrollBot * m_NrCols;
    m_Cells[Offset+Col].Char = 0X20;
    m_Cells[Offset+Col].Attr = m_CurrentAttr;
  }
  ResetDWDH(m_ScrollBot);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// LineUp
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::LineUp() {

  m_CursorRow--;
  if (m_CursorRow >= m_ScrollTop) return;
 
  m_CursorRow = m_ScrollTop;

  for (int Row = m_ScrollBot; Row > m_ScrollTop; Row--) {
    int Offset1 = Row * m_NrCols;
    int Offset2 = (Row-1) * m_NrCols;
    for ( int Col = 0; Col < m_NrCols; Col++) {
      m_Cells[Offset1+Col] = m_Cells[Offset2+Col];
    }
    m_DWDHs[Row] = m_DWDHs[Row-1];
  }

  for (int Col = 0; Col < m_NrCols; Col++) {
    int Offset = m_ScrollTop * m_NrCols;
    m_Cells[Offset+Col].Char = 0X20;
    m_Cells[Offset+Col].Attr = m_CurrentAttr;
  }
  ResetDWDH(m_ScrollTop);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

uint16_t cbConsole::ToCellChar(const uint8_t Byte) {

  bool    High    = Byte & 0X80;
  uint8_t Code    = Byte & 0X7F;
  bool    Control = Code < 0X20;

  QByteArray Set;

  switch (m_TermLevel) {
    case 0 :
      // VT52
      Set = m_VT52_SSA ? "0" : "B";
      break;
    case 1 : 
      // VT100
      Set = m_G[m_GL];
      break;
    default :
      Set = High ? m_G[m_GR] : m_G[m_GL];
  }

  // Single shift reset.
  if (m_SingleShifting) {
    m_SingleShifting = false;
    m_GL = m_OldGL;
    //qDebug("%d : m_GL now %d",__LINE__,m_GL);
  }

  if (not m_CharSetNameIdxMap.contains(Set)) {
    qFatal("%s@%s:%d : Unexpected Set ('%s')",
           __PRETTY_FUNCTION__, CB_BASENAME(__FILE__), __LINE__,
           Set.constData());
  }

  uint8_t Idx = m_CharSetNameIdxMap.value(Set);

  // We don't expect Controls as this is in the VT_ACTION_PRINT stream.
  if (Control) {
    qFatal("%s@%s:%d : Unexpected Byte 0X%02X",
           __PRETTY_FUNCTION__,CB_BASENAME(__FILE__),__LINE__,
            Byte);
  
   
  }

  return (Idx << 8) | Code;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbConsole::CreateHardCharSets() {

  // Name to index map for character sets.
  m_CharSetNameIdxMap.insert("A",0);   
  m_CharSetNameIdxMap.insert("B",1);  
  m_CharSetNameIdxMap.insert("0",2);  
  m_CharSetNameIdxMap.insert("1",3);
  m_CharSetNameIdxMap.insert("2",4);
  m_CharSetNameIdxMap.insert("<",5);
  m_CharSetNameIdxMap.insert("4",6);
  m_CharSetNameIdxMap.insert("5",7);
  m_CharSetNameIdxMap.insert("C",7);
  m_CharSetNameIdxMap.insert("R",8);
  m_CharSetNameIdxMap.insert("Q",9);
  m_CharSetNameIdxMap.insert("K",10);
  m_CharSetNameIdxMap.insert("Y",11);
  m_CharSetNameIdxMap.insert("E",12);
  m_CharSetNameIdxMap.insert("6",12);
  m_CharSetNameIdxMap.insert("Z",13);
  m_CharSetNameIdxMap.insert("H",14);
  m_CharSetNameIdxMap.insert("7",14);
  m_CharSetNameIdxMap.insert("=",15);

  for (int Set = 0; Set < c_NrHardCharSets; Set++) {
    for (int Char = 0; Char < c_NrCharsPerSet; Char++) {
      m_CharSets[Set].Uni16Map[Char] = c_Uni16Maps[Set][Char];
    }
  }

  CreateVt220Glyphs();
  CreateHostGlyphs();
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const uint16_t cbConsole::c_Uni16Maps[c_NrHardCharSets][c_NrCharsPerSet] = {

  // 0 : Charset A. UK National.
  {
  0X0020, 0X0021, 0X0022, 0X00A3, 0X0024, 0X0025, 0X0026, 0X0027,
  0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  0X0040, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  0X0058, 0X0059, 0X005A, 0X005B, 0X005C, 0X005D, 0X005E, 0X005F,
  0X0060, 0X0061, 0X0062, 0X0063, 0X0064, 0X0065, 0X0066, 0X0067,
  0X0068, 0X0069, 0X006A, 0X006B, 0X006C, 0X006D, 0X006E, 0X006F,
  0X0070, 0X0071, 0X0072, 0X0063, 0X0074, 0X0075, 0X0076, 0X0077,
  0X0078, 0X0079, 0X007A, 0X007B, 0X007C, 0X007D, 0X007E, 0X007F
  },

  // 1 : Charset B. US ASCII.
  {
  0X0020, 0X0021, 0X0022, 0X0023, 0X0024, 0X0025, 0X0026, 0X0027,
  0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  0X0040, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  0X0058, 0X0059, 0X005A, 0X005B, 0X005C, 0X005D, 0X005E, 0X005F,
  0X0060, 0X0061, 0X0062, 0X0063, 0X0064, 0X0065, 0X0066, 0X0067,
  0X0068, 0X0069, 0X006A, 0X006B, 0X006C, 0X006D, 0X006E, 0X006F,
  0X0070, 0X0071, 0X0072, 0X0073, 0X0074, 0X0075, 0X0076, 0X0077,
  0X0078, 0X0079, 0X007A, 0X007B, 0X007C, 0X007D, 0X007E, 0X007F
  },

  // 2 : Charset 0. Special graphics and line drawing.
  {
  0X0020, 0X0021, 0X0022, 0X0023, 0X0024, 0X0025, 0X0026, 0X0027,
  0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  0X0040, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  0X0058, 0X0059, 0X005A, 0X005B, 0X005C, 0X005D, 0X005E, 0X0020,
  0X25C6, 0X2592, 0X2409, 0X240C, 0X240D, 0X240A, 0X00B0, 0X00B1,
  0X2424, 0X240B, 0X2518, 0X2510, 0X250C, 0X2514, 0X253C, 0X23BA,
  0X23BB, 0X2500, 0X23BC, 0X23BD, 0X251C, 0X2524, 0X2534, 0X252C,
  0X2502, 0X2264, 0X2265, 0X03C0, 0X2260, 0X00A3, 0X00B7, 0X007F
  },

  // 3 : Charset 1. Alternate ROM, standard chars.
  {
  0X0020, 0X0021, 0X0022, 0X0023, 0X0024, 0X0025, 0X0026, 0X0027,
  0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  0X0040, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  0X0058, 0X0059, 0X005A, 0X005B, 0X005C, 0X005D, 0X005E, 0X005F,
  0X0060, 0X0061, 0X0062, 0X0063, 0X0064, 0X0065, 0X0066, 0X0067,
  0X0068, 0X0069, 0X006A, 0X006B, 0X006C, 0X006D, 0X006E, 0X006F,
  0X0070, 0X0071, 0X0072, 0X0073, 0X0074, 0X0075, 0X0076, 0X0077,
  0X0078, 0X0079, 0X007A, 0X007B, 0X007C, 0X007D, 0X007E, 0X007F
  },

  // 4 : Charset 1. Alternate ROM, special graphs.
  {
  0X0020, 0X0021, 0X0022, 0X0023, 0X0024, 0X0025, 0X0026, 0X0027,
  0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  0X0040, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  0X0058, 0X0059, 0X005A, 0X005B, 0X005C, 0X005D, 0X005E, 0X0020,
  0X25C6, 0X2592, 0X2409, 0X240C, 0X240D, 0X240A, 0X00B0, 0X00B1,
  0X2424, 0X240B, 0X2518, 0X2510, 0X250C, 0X2514, 0X253C, 0X23BA,
  0X23BB, 0X2500, 0X23BC, 0X23BD, 0X251C, 0X2524, 0X2534, 0X252C,
  0X2502, 0X2264, 0X2265, 0X03C0, 0X2260, 0X00A3, 0X00B7, 0X007F
  },

  // Supplemental graphics (VT200, international)
  {
  // 5 : Multinational. Maps as good as on unicode.
  0X00A0, 0X00A1, 0X00A2, 0X00A3, 0X00A4, 0X00A5, 0X00A6, 0X00A7,
  0X00A8, 0X00A9, 0X00AA, 0X00AB, 0X00AC, 0X00AD, 0X00AE, 0X00AF,
  0X00B0, 0X00B1, 0X00B2, 0X00B3, 0X00B4, 0X00B5, 0X00B6, 0X00B7,
  0X00B8, 0X00B9, 0X00BA, 0X00BB, 0X00BC, 0X00BD, 0X00BE, 0X00BF,
  0X00C0, 0X00C1, 0X00C2, 0X00C3, 0X00C4, 0X00C5, 0X00C6, 0X00C7,
  0X00C8, 0X00C9, 0X00CA, 0X00CB, 0X00CC, 0X00CD, 0X00CE, 0X00CF,
  0X00D0, 0X00D1, 0X00D2, 0X00D3, 0X00D4, 0X00D5, 0X00D6, 0X00D7,
  0X00D8, 0X00D9, 0X00DA, 0X00DB, 0X00DC, 0X00DD, 0X00DE, 0X00DF,
  0X00E0, 0X00E1, 0X00E2, 0X00E3, 0X00E4, 0X00E5, 0X00E6, 0X00E7,
  0X00E8, 0X00E9, 0X00EA, 0X00EB, 0X00EC, 0X00ED, 0X00EE, 0X00EF,
  0X00F0, 0X00F1, 0X00F2, 0X00F3, 0X00F4, 0X00F5, 0X00F6, 0X00F7,
  0X00F8, 0X00F9, 0X00FA, 0X00FB, 0X00FC, 0X00FD, 0X00FE, 0X00FF
  },

  // 6 : Dutch replacement.
  {
  0X0020, 0X0021, 0X0022, 0X00A3, 0X0024, 0X0025, 0X0026, 0X0027,
  0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  0X00BE, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  0X0058, 0X0059, 0X005A, 0X00FF, 0X00BD, 0X007C, 0X005E, 0X005F,
  0X0060, 0X0061, 0X0062, 0X0063, 0X0064, 0X0065, 0X0066, 0X0067,
  0X0068, 0X0069, 0X006A, 0X006B, 0X006C, 0X006D, 0X006E, 0X006F,
  0X0070, 0X0071, 0X0072, 0X0073, 0X0074, 0X0075, 0X0076, 0X0077,
  0X0078, 0X0079, 0X007A, 0X00A8, 0X0066, 0X00BC, 0X00B4, 0X007F
  },

  // 7 : Finish replacement.
  {
  0X0020, 0X0021, 0X0022, 0X0023, 0X0024, 0X0025, 0X0026, 0X0027,
  0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  0X0040, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  0X0058, 0X0059, 0X005A, 0X00C4, 0X00D6, 0X00C5, 0X00DC, 0X005F,
  0X00E9, 0X0061, 0X0062, 0X0063, 0X0064, 0X0065, 0X0066, 0X0067,
  0X0068, 0X0069, 0X006A, 0X006B, 0X006C, 0X006D, 0X006E, 0X006F,
  0X0070, 0X0071, 0X0072, 0X0073, 0X0074, 0X0075, 0X0076, 0X0077,
  0X0078, 0X0079, 0X007A, 0X00E4, 0X00F6, 0X00E5, 0X00FC, 0X007F
  },

  // 8 : French replacement.
  {
  0X0020, 0X0021, 0X0022, 0X00A3, 0X0024, 0X0025, 0X0026, 0X0027,
  0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  0X00E0, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  0X0058, 0X0059, 0X005A, 0X00D0, 0X00E7, 0X00A7, 0X005E, 0X005F,
  0X0060, 0X0061, 0X0062, 0X0063, 0X0064, 0X0065, 0X0066, 0X0067,
  0X0068, 0X0069, 0X006A, 0X006B, 0X006C, 0X006D, 0X006E, 0X006F,
  0X0070, 0X0071, 0X0072, 0X0073, 0X0074, 0X0075, 0X0076, 0X0077,
  0X0078, 0X0079, 0X007A, 0X00E9, 0X00F9, 0X00E8, 0X00A8, 0X007F
  },

  // 9 : French Canadian replacement.
  {
  0X0020, 0X0021, 0X0022, 0X0023, 0X0024, 0X0025, 0X0026, 0X0027,
  0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  0X00E0, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  0X0058, 0X0059, 0X005A, 0X00E2, 0X00E7, 0X00EA, 0X00EE, 0X005F,
  0X00F4, 0X0061, 0X0062, 0X0063, 0X0064, 0X0065, 0X0066, 0X0067,
  0X0068, 0X0069, 0X006A, 0X006B, 0X006C, 0X006D, 0X006E, 0X006F,
  0X0070, 0X0071, 0X0072, 0X0073, 0X0074, 0X0075, 0X0076, 0X0077,
  0X0078, 0X0079, 0X007A, 0X00E9, 0X00F9, 0X00E8, 0X00FB, 0X007F
  },

  // 10 : German replacement.
  {
  0X0020, 0X0021, 0X0022, 0X0023, 0X0024, 0X0025, 0X0026, 0X0027,
  0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  0X00A7, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  0X0058, 0X0059, 0X005A, 0X00C4, 0X00D6, 0X00DC, 0X005E, 0X005F,
  0X0060, 0X0061, 0X0062, 0X0063, 0X0064, 0X0065, 0X0066, 0X0067,
  0X0068, 0X0069, 0X006A, 0X006B, 0X006C, 0X006D, 0X006E, 0X006F,
  0X0070, 0X0071, 0X0072, 0X0073, 0X0074, 0X0075, 0X0076, 0X0077,
  0X0078, 0X0079, 0X007A, 0X00E4, 0X00F6, 0X00FC, 0X00DF, 0X007F
  },

  // 11 : Italian replacement.
  {
  0X0020, 0X0021, 0X0022, 0X00A3, 0X0024, 0X0025, 0X0026, 0X0027,
  0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  0X00A7, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  0X0058, 0X0059, 0X005A, 0X00B0, 0X00E7, 0X00E9, 0X005E, 0X005F,
  0X00F9, 0X0061, 0X0062, 0X0063, 0X0064, 0X0065, 0X0066, 0X0067,
  0X0068, 0X0069, 0X006A, 0X006B, 0X006C, 0X006D, 0X006E, 0X006F,
  0X0070, 0X0071, 0X0072, 0X0073, 0X0074, 0X0075, 0X0076, 0X0077,
  0X0078, 0X0079, 0X007A, 0X00E0, 0X00F2, 0X00E8, 0X00EC, 0X007F
  },

  // 12 : Norwegian/Danish replacement.
  {
  0X0020, 0X0021, 0X0022, 0X0023, 0X0024, 0X0025, 0X0026, 0X0027,
  0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  0X00C4, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  0X0058, 0X0059, 0X005A, 0X00C6, 0X00D8, 0X00C5, 0X00DE, 0X005F,
  0X00E4, 0X0061, 0X0062, 0X0063, 0X0064, 0X0065, 0X0066, 0X0067,
  0X0068, 0X0069, 0X006A, 0X006B, 0X006C, 0X006D, 0X006E, 0X006F,
  0X0070, 0X0071, 0X0072, 0X0073, 0X0074, 0X0075, 0X0076, 0X0077,
  0X0078, 0X0079, 0X007A, 0X00E6, 0X00F8, 0X00E5, 0X00FC, 0X007F
  },
  
  // 13 : Spanish replacement.
  {
  0X0020, 0X0021, 0X0022, 0X00A3, 0X0024, 0X0025, 0X0026, 0X0027,
  0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  0X00A7, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  0X0058, 0X0059, 0X005A, 0X00A1, 0X00D1, 0X00BF, 0X005E, 0X005F,
  0X0060, 0X0061, 0X0062, 0X0063, 0X0064, 0X0065, 0X0066, 0X0067,
  0X0068, 0X0069, 0X006A, 0X006B, 0X006C, 0X006D, 0X006E, 0X006F,
  0X0070, 0X0071, 0X0072, 0X0073, 0X0074, 0X0075, 0X0076, 0X0077,
  0X0078, 0X0079, 0X007A, 0X00D0, 0X00F1, 0X00E7, 0X007E, 0X007F
  },

  // 14 : Swedish replacement.
  {
  0X0020, 0X0021, 0X0022, 0X0023, 0X0024, 0X0025, 0X0026, 0X0027,
  0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  0X00C9, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  0X0058, 0X0059, 0X005A, 0X00C4, 0X00D6, 0X00C5, 0X00DC, 0X005F,
  0X00E9, 0X0061, 0X0062, 0X0063, 0X0064, 0X0065, 0X0066, 0X0067,
  0X0068, 0X0069, 0X006A, 0X006B, 0X006C, 0X006D, 0X006E, 0X006F,
  0X0070, 0X0071, 0X0072, 0X0073, 0X0074, 0X0075, 0X0076, 0X0077,
  0X0078, 0X0079, 0X007A, 0X00E4, 0X00F6, 0X00E5, 0X00FC, 0X007F
  },

  // 15 : Swiss replacement.
  {
  0X0020, 0X0021, 0X0022, 0X00F9, 0X0024, 0X0025, 0X0026, 0X0027,
  0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  0X00E0, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  0X0058, 0X0059, 0X005A, 0X00E9, 0X00E7, 0X00EA, 0X00EE, 0X00E8,
  0X00F4, 0X0061, 0X0062, 0X0063, 0X0064, 0X0065, 0X0066, 0X0067,
  0X0068, 0X0069, 0X006A, 0X006B, 0X006C, 0X006D, 0X006E, 0X006F,
  0X0070, 0X0071, 0X0072, 0X0073, 0X0074, 0X0075, 0X0076, 0X0077,
  0X0078, 0X0079, 0X007A, 0X00E4, 0X00F6, 0X00FC, 0X00FB, 0X007F
  }
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
