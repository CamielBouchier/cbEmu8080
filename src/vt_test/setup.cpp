/* $Id: setup.c,v 1.32 2012/04/22 14:48:15 tom Exp $ */

#include <vttest.h>
#include <esc.h>
#include <ttymodes.h>

static int cur_level = -1;      /* current operating level (VT100=1) */
static int max_level = -1;      /* maximum operating level */

static int
check_8bit_toggle(void)
{
  char *report;

  set_tty_raw(true); 

  cup(1, 1);
  dsr(6);
  report = InString();

  restore_ttymodes();

  if ((report = skip_csi(report)) != 0
      && !strcmp(report, "1;1R"))
    return true;
  return false;
}

/*
 * Determine the current and maximum operating levels of the terminal
 */
static void
find_levels(void)
{
  char *report;

  set_tty_raw(true);
  set_tty_echo(false);

  da();
  report = InString();
  if (!strcmp(report, "\033/Z")) {
    cur_level =
      max_level = 0;  /* must be a VT52 */
  } else if ((report = skip_csi(report)) == 0
             || strncmp(report, "?6", (size_t) 2)
             || !isdigit(CharOf(report[2]))
             || report[3] != ';') {
    cur_level =
      max_level = 1;  /* must be a VT100 */
  } else {      /* "CSI ? 6 x ; ..." */
    cur_level = max_level = report[2] - '0';  /* VT220=2, VT320=3, VT420=4 */
    if (max_level >= 4) {
      decrqss("\"p");
      report = InString();
      if ((report = skip_dcs(report)) != 0
          && isdigit(CharOf(*report++))   /* 0 or 1 (by observation, though 1 is an err) */
          &&*report++ == '$'
          && *report++ == 'r'
          && *report++ == '6'
          && isdigit(CharOf(*report)))
        cur_level = *report - '0';
    }
  }

  /*
  fprintf("Max Operating Level: %d\n", max_level);
  fprintf("Cur Operating Level: %d\n", cur_level);
  */

  restore_ttymodes();
}

static int
toggle_DECSCL(const char*)
{
  int request = cur_level;

  if (max_level <= 1) {
    cup(1, 1);
    cb_printf("Sorry, terminal supports only VT%d", terminal_id());
    cup(max_lines - 1, 1);
    return MENU_HOLD;
  }

  if (++request > max_level)
    request = 1;
  set_level(request);

  restore_ttymodes();

  return MENU_NOHOLD;
}

static int
toggle_Slowly(const char*)
{
  slow_motion = !slow_motion;
  return MENU_NOHOLD;
}

static int
toggle_8bit_in(const char*)
{
  int old = input_8bits;

  s8c1t(!old);
  if (!check_8bit_toggle()) {
    input_8bits = old;
    ed(2);
    cup(1, 1);
    println("Sorry, this terminal does not support 8-bit input controls");
    return MENU_HOLD;
  }
  return MENU_NOHOLD;
}

/*
 * This changes the CSI code to/from an escape sequence.
 */
static int
toggle_8bit_out(const char*)
{
  int old = output_8bits;

  output_8bits = !output_8bits;
  if (!check_8bit_toggle()) {
    output_8bits = old;
    ed(2);
    cup(1, 1);
    println("Sorry, this terminal does not support 8-bit output controls");
    return MENU_HOLD;
  }
  return MENU_NOHOLD;
}

/******************************************************************************/

void
reset_level(void)
{
  cur_level = max_level;
}

void
restore_level(VTLEVEL *save) {
  set_level(save->cur_level);
  if (cur_level > 1
      && save->input_8bits != input_8bits)  /* just in case level didn't change */
    s8c1t(save->input_8bits);
  output_8bits = save->output_8bits;  /* in case we thought this was VT100 */
}

void
save_level(VTLEVEL *save) {
  save->cur_level    = cur_level;
  save->input_8bits  = input_8bits;
  save->output_8bits = output_8bits;
}

int
get_level(void)
{
  return cur_level;
}

int
set_level(int request)
{
  if (cur_level < 0)
    find_levels();

  /*
    fprintf("set_level(%d)\n", request);
  */

  if (request > max_level) {
    cb_printf("Sorry, this terminal supports only VT%d\r\n", terminal_id());
    return false;
  }

  if (request != cur_level) {
    if (request == 0) {
      rm("?2"); /* Reset ANSI (VT100) mode, Set VT52 mode  */
      input_8bits = false;
      output_8bits = false;
    } else {
      if (cur_level == 0) {
        esc("<");   /* Enter ANSI mode (VT100 mode) */
      }
      if (request == 1) {
        input_8bits = false;
        output_8bits = false;
      }
      if (request > 1) {
        do_csi("6%d;%d\"p", request, !input_8bits);
      } else {
        do_csi("61\"p");
      }
    }

    cur_level = request;
  }

  /*
    fprintf("...set_level(%d) in=%d, out=%d\n", cur_level,
            input_8bits ? 8 : 7,
            output_8bits ? 8 : 7);
  */

  return true;
}

/*
 * Set the terminal's operating level to the default (i.e., based on what the
 * terminal returns as a response to DA).
 */
void
default_level(void)
{
  if (max_level < 0)
    find_levels();
  set_level(max_level);
}

int
terminal_id(void)
{
  if (max_level >= 1)
    return max_level * 100;
  else if (max_level == 0)
    return 52;
  return 100;
}

int
tst_setup(const char*)
{
  static char txt_output[80] = "send 7/8";
  static char txt_input8[80] = "receive 7/8";
  static char txt_DECSCL[80] = "DECSCL";
  static char txt_slowly[80];
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
    { "Exit",                                                0 },
    { "Setup terminal to original test-configuration",       setup_terminal },
    { txt_output,                                            toggle_8bit_out },
    { txt_input8,                                            toggle_8bit_in },
    { txt_DECSCL,                                            toggle_DECSCL },
    { txt_slowly,                                            toggle_Slowly },
    { "",                                                    0 }
  };
  /* *INDENT-ON* */

  if (cur_level < 0)
    find_levels();

  do {
    sprintf(txt_output, "Send %d-bit controls", output_8bits ? 8 : 7);
    sprintf(txt_input8, "Receive %d-bit controls", input_8bits ? 8 : 7);
    sprintf(txt_DECSCL, "Operating level %d (VT%d)",
            cur_level, cur_level ? cur_level * 100 : 52);
    sprintf(txt_slowly, "Slow-movement/scrolling %s", STR_ENABLED(slow_motion));

    ed(2);
    __(title(0), println("Modify test-parameters"));
    __(title(2), println("Select a number to modify it:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}
