/* $Id: ttymodes.h,v 1.3 1996/08/22 10:56:18 tom Exp $ */

#ifndef TTYMODES_H
#define TTYMODES_H 1

typedef struct {
  bool Echo;
  bool Raw;  // Pro-forma.
  bool O_CRLF; // On output : translate CR to CR/LF
} cbTTY;

extern cbTTY TTY;

void init_ttymodes();
void restore_ttymodes();
void set_tty_crmod(int enabled);
void set_tty_echo(int enabled);
void set_tty_raw(int enabled);

#endif /* TTYMODES_H */
