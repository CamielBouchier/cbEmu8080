/* $Id: printer.c,v 1.8 2011/12/06 01:52:06 tom Exp $ */

#include <vttest.h>
#include <esc.h>

static int pex_mode;
static int pff_mode;
static int started;
static int assigned;
static int margin_lo;
static int margin_hi;

static void
setup_printout(const char* the_title, int visible, const char *whole)
{
  margin_lo = 7;
  margin_hi = max_lines - 5;

  ed(2);
  cup(1, 1);
  println(the_title);
  println("Test screen for printing.  We will set scrolling margins at");
  cbprintf("lines %d and %d, and write a test pattern there.\r\n", margin_lo, margin_hi);
  cbprintf("The test pattern should be %s.\r\n", visible
         ? "visible"
         : "invisible");
  cbprintf("The %s should be in the printer's output.\r\n", whole);
  decstbm(margin_lo, margin_hi);
  cup(margin_lo, 1);
}

static void
test_printout(void)
{
  int row, col;
  cup(margin_hi, 1);
  for (row = 0; row < max_lines; row++) {
    cbprintf("%3d:", row);
    for (col = 0; col < min_cols - 5; col++) {
      cbprintf("%c", ((row + col) % 26) + 'a');
    }
    cbprintf("\r\n");
  }
}

static void
cleanup_printout(void)
{
  decstbm(0, 0);
  cup(max_lines - 2, 1);
}

static int
tst_Assign(const char*)
{
  mc_printer_assign(assigned = !assigned);
  return MENU_HOLD;
}

static int
tst_DECPEX(const char*)
{
  decpex(pex_mode = !pex_mode);
  return MENU_HOLD;
}

static int
tst_DECPFF(const char*)
{
  decpff(pff_mode = !pff_mode);
  return MENU_HOLD;
}

static int
tst_Start(const char*)
{
  mc_printer_start(started = !started);
  return MENU_HOLD;
}

static int
tst_autoprint(const char* the_title)
{
  setup_printout(the_title, true, "scrolling region");
  mc_autoprint(true);
  test_printout();
  mc_autoprint(false);
  cleanup_printout();
  return MENU_HOLD;
}

static int
tst_printer_controller(const char* the_title)
{
  setup_printout(the_title, false, "scrolling region");
  mc_printer_controller(true);
  test_printout();
  mc_printer_controller(false);
  cleanup_printout();
  return MENU_HOLD;
}

static int
tst_print_all_pages(const char* the_title)
{
  setup_printout(the_title, true, "contents of all pages");
  test_printout();
  mc_print_all_pages();
  cleanup_printout();
  return MENU_HOLD;
}

static int
tst_print_cursor(const char* the_title)
{
  int row;
  setup_printout(the_title, true, "reverse of the scrolling region");
  test_printout();
  for (row = margin_hi; row >= margin_lo; row--) {
    cup(row, 1);
    mc_print_cursor_line();
  }
  cleanup_printout();
  return MENU_HOLD;
}

static int
tst_print_display(const char* the_title)
{
  setup_printout(the_title, true, "whole display");
  test_printout();
  mc_print_composed();
  cleanup_printout();
  return MENU_HOLD;
}

static int
tst_print_page(const char* the_title)
{
  setup_printout(the_title, true,
                 pex_mode
                 ? "whole page"
                 : "scrolling region");
  test_printout();
  mc_print_page();
  cleanup_printout();
  return MENU_HOLD;
}

int
tst_printing(const char*)
{
  static char pex_mesg[80];
  static char pff_mesg[80];
  static char assign_mesg[80];
  static char start_mesg[80];
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { assign_mesg,                                         tst_Assign },
      { start_mesg,                                          tst_Start },
      { pex_mesg,                                            tst_DECPEX },
      { pff_mesg,                                            tst_DECPFF },
      { "Test Auto-print mode (MC - DEC private mode)",      tst_autoprint },
      { "Test Printer-controller mode (MC)",                 tst_printer_controller },
      { "Test Print-page (MC)",                              tst_print_page },
      { "Test Print composed main-display (MC)",             tst_print_display },
      { "Test Print all pages (MC)",                         tst_print_all_pages },
      { "Test Print cursor line (MC)",                       tst_print_cursor },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  do {
    sprintf(pex_mesg, "%s Printer-Extent mode (DECPEX)", STR_ENABLE(pex_mode));
    sprintf(pff_mesg, "%s Print Form Feed Mode (DECPFF)", STR_ENABLE(pff_mode));
    strcpy(assign_mesg, assigned
           ? "Release printer (MC)"
           : "Assign printer to active session (MC)");
    sprintf(start_mesg, "%s printer-to-host session (MC)", STR_START(started));
    ed(2);
    __(title(0), cbprintf("Printing-Control Tests"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));

  if (pex_mode)
    decpex(pex_mode = 0);

  if (pff_mode)
    decpex(pff_mode = 0);

  if (assigned)
    mc_printer_start(assigned = 0);

  if (started)
    mc_printer_start(started = 0);

  return MENU_NOHOLD;
}
