#include <QtCore>
#include <ttymodes.h>

// Most of this is useless in the cb_emu_8080 case.
// Probably only "echo on/off" and "O_CRLF" has some importance.

cb_TTY TTY;

void init_ttymodes() {
  TTY.Echo   = true;
  TTY.Raw    = false;
  TTY.O_CRLF = true;
}

void restore_ttymodes() {
  init_ttymodes();
}

void set_tty_crmod(int) {
}

void set_tty_echo(int Enabled) {
  TTY.Echo = Enabled;
}

void set_tty_raw(int Enabled) {
  TTY.Raw = Enabled;
}
