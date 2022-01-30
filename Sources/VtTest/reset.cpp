/* $Id: reset.c,v 1.7 2011/12/06 10:43:28 tom Exp $ */

#include <vttest.h>
#include <esc.h>

static int did_reset = false;

int
tst_DECSTR(const char* the_title)
{
  cup(1, 1);
  println(the_title);
  println("(VT220 & up)");
  println("");
  println("The terminal will now soft-reset");
  holdit();
  decstr();
  return MENU_HOLD;
}

static int
tst_DECTST(const char* the_title)
{
  cup(1, 1);
  println(the_title);
  println("");

  if (did_reset)
    println("The terminal is now RESET.  Next, the built-in confidence test");
  else
    cbprintf("The built-in confidence test ");
  cbprintf("will be invoked. ");
  holdit();

  ed(2);
  dectst(1);
  cup(10, 1);
  println("If the built-in confidence test found any errors, a code");
  cbprintf("%s", "is visible above. ");

  did_reset = false;
  return MENU_HOLD;
}

static int
tst_RIS(const char* the_title)
{
  cup(1, 1);
  println(the_title);
  println("(VT100 & up, not recommended)");
  println("");
  cbprintf("The terminal will now be RESET. ");
  holdit();
  ris();

  did_reset = true;
  reset_level();
  input_8bits = false;
  output_8bits = false;
  return MENU_HOLD;
}

int
tst_rst(const char* the_title)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Reset to Initial State (RIS)",                      tst_RIS },
      { "Invoke Terminal Test (DECTST)",                     tst_DECTST },
      { "Soft Terminal Reset (DECSTR)",                      tst_DECSTR },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  did_reset = false;

  do {
    ed(2);
    __(title(0), cbprintf("%s", the_title));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}
