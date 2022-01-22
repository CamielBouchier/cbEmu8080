#include <stdarg.h>
#include <unistd.h>
#include "cb_Console.h"
#include "cb_emu_8080.h"  // BaseName stuff.

/* 
 * With respect to poisining winnt header files this must come as last.
 */

#include <vttest.h>
#include <esc.h>
#include <ttymodes.h>

#define BUF_SIZE 1024

extern jmp_buf JmpBuf;
extern bool    RequestToStop;

/*
 * Wait until a character is typed on the terminal then read it, without
 * waiting for CR.
 */

char InChar(void) {
  uint8_t Byte = 0;
  // Loop that waits for input. Qt gentle.
  while (not TheConsole->Read(1)) {
    QCoreApplication::processEvents();
    // Longjmp needs to be nicely synced with event handling of Qt.
    if (RequestToStop) {
      longjmp(JmpBuf,1);
    }
  };
  Byte = TheConsole->Read(2);
  if (TTY.Echo) {
    //qDebug("Echoing %02X due to TTY.Echo",Byte);
    TheConsole->Write(0,Byte);
    if (Byte == 0X0D and TTY.O_CRLF) {
      TheConsole->Write(0,0X0A);
    }
    if (Byte == 0X08 and not TTY.Raw) {
      // CSI-P (DCH)
      TheConsole->Write(0,0X1B);
      TheConsole->Write(0,'[');
      TheConsole->Write(0,'P');
    }
  }
  return Byte;
}

/*
 * Get an unfinished string from the terminal:  wait until a character is typed
 * on the terminal, then read it, and all other available characters.  Return a
 * pointer to that string.
 */

char* InString(void) {
  static char Result[BUF_SIZE];
  int i = 0;
  Result[i++] = InChar();
  while ( (i<BUF_SIZE-1) and TheConsole->Read(1)) {
    Result[i++] = TheConsole->Read(2);
  }
  Result[i] = 0;
  return Result;
}

/*
 * Read to the next newline, truncating the buffer at BUFSIZ-1 characters
 */

void InputLine(char *s) {
  char EndChar = TheConsole->IsLNM() ? 0X0A : 0X0D;
  do {
    char Char;
    char *d = s;
    while ((Char = InChar()) and Char != EndChar) {
      if ((d - s) < BUFSIZ - 2)
        *d++ = Char;
    }
    *d = 0;
  } while (!*s);

  // TODO XXX
  // If worth for this test, I can later add code to clean up 
  // the input (BS, cursor controls, ...) to clean. This is TTY functionality.
  // if (not TTY.Raw) {
  // }

  /*
  qDebug("%s@%s:%d Done : '%s'",
         __PRETTY_FUNCTION__,
         CB_BASENAME(__FILE__),
         __LINE__,
         s);
  */
}

/*
 * Flush input buffer, make sure no pending input character
 */

void InFlush(void) {
  while (TheConsole->Read(1)) {
    TheConsole->Read(2);
  }
}

void holdit(void) {
  InFlush();
  tprintf("Push <RETURN>");
  ConsumeTillNL();
}

void ConsumeTillNL(void) {
  char EndChar = TheConsole->IsLNM() ? 0X0A : 0X0D;
  char Char;
  while ((Char = InChar()) and Char != EndChar) {
  }
}

////////////////////////////////////////////////////////////////////////////////

// Output wrappers to write to our console.

// That's more than 5 screens. Our tests never write this ...
#define MAX_BUFSIZE 16384

int cb_printf(const char *fmt,...) {
  char Buffer[MAX_BUFSIZE];
  va_list ap;
  va_start(ap, fmt);
  int RV = vsnprintf(Buffer,MAX_BUFSIZE,fmt,ap);
  if (RV >= MAX_BUFSIZE) {
    qFatal("%s@%s:%d Attempt to write %d bytes to buffer of size %d",
           __PRETTY_FUNCTION__,
           CB_BASENAME(__FILE__),
           __LINE__,
           RV,
           MAX_BUFSIZE);
  }
  va_end(ap);
  // due to above RV >= MAX_BUFSIZE check, we are sure of a terminating 0.
  for (const char* Ptr = Buffer; *Ptr; Ptr++) {
    TheConsole->Write(0,*Ptr);
  }
  return RV;
};

int cb_putchar(int c) {
  TheConsole->Write(0,c);
  return c;
};

int cb_fputs(const char* s) {
  for (const char* Ptr = s; *Ptr; Ptr++) {
    TheConsole->Write(0,*Ptr);
  }
  return 0;
};

int cb_fputc(int c) {
  TheConsole->Write(0,c);
  return c;
};
