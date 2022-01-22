//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/* $Id: main.c,v 1.104 2012/05/04 20:57:12 tom Exp $ */

/*
                               VTTEST.C

         Written November 1983 - July 1984 by Per Lindberg,
         Stockholm University Computer Center (QZ), Sweden.

                  THE MAD PROGRAMMER STRIKES AGAIN!

  Copyright (c) 1984, Per Lindberg
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:
  1.  Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
  2.  Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
  3.  Neither the name of Per Lindberg nor the names of contributors may
      be used to endorse or promote products derived from this software
      without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "cb_emu_8080.h"
#include "cb_Console.h"

/* 
 * With respect to poisoning winnt header files these need to come here.
 */

#include <vttest.h>
#include <ttymodes.h>
#include <esc.h>

/* *INDENT-EQLS* */
int max_lines   = 24;
int max_cols    = 132;
int min_cols    = 80;
int input_8bits = false;
int output_8bits = false;
int slow_motion = false;
jmp_buf intrenv;

static char empty[1];
static char *current_menu = empty;

cb_Console* TheConsole;

// For emergency returns.
jmp_buf    JmpBuf;
bool       RequestToStop;
bool       Running = false;

void StopVtTest() {
  RequestToStop = true;
}

bool RunningVtTest() {
  return Running;
}

int vttest(cb_Console* Console) {
 
  // This likely leaves a number of leaks, but is 
  // only needed for exiting cb_emu_8080 without crashes caused by
  // further using of already deleted objects of cb_emu_8080.
  RequestToStop = false;
  Running       = true;
  if (setjmp(JmpBuf)) {
    Running = false;
    return EXIT_FAILURE;
  }

  // Soft character test built-in.
  QString TestSet = Emu8080->m_DataLocation + "/charsets/test.set";
  setup_softchars(C_STRING(TestSet));

  //
  TheConsole     = Console;

  /* *INDENT-OFF* */
  static MENU mainmenu[] = {
      { "Exit",                                              0 },
      { "Test of cursor movements",                          tst_movements },
      { "Test of screen features",                           tst_screen },
      { "Test of character sets",                            tst_characters },
      { "Test of double-sized characters",                   tst_doublesize },
      { "Test of keyboard",                                  tst_keyboard },
      { "Test of terminal reports",                          tst_reports },
      { "Test of VT52 mode",                                 tst_vt52 },
      { "Test of VT102 features (Insert/Delete Char/Line)",  tst_insdel },
      { "Test of known bugs",                                tst_bugs },
      { "Test of reset and self-test",                       tst_rst },
      { "Test non-VT100 (e.g., VT220, XTERM) terminals",     tst_nonvt100 },
      { "Modify test-parameters",                            tst_setup },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  init_ttymodes();
  setup_terminal("");

  do {
    ed(2);
    __(title(0), cb_printf("VT100 test program, version %d.%d (%d- %s variant)", RELEASE, PATCHLEVEL,PATCH_DATE,C_STRING(cb_emu_8080::ProgramName)));

    title(1);
    if (max_lines != 24
        || min_cols != 80
        || max_cols != 132)
      cb_printf("Screen size %dx%d (%d max) ", max_lines, min_cols, max_cols);
    __(title(2), println("Choose test type:"));
  } while (menu(mainmenu));
  bye();
  Running = false;
  return EXIT_SUCCESS;
}

int
tst_movements(const char*)
{
  /* Test of:
     CUF (Cursor Forward)
     CUB (Cursor Backward)
     CUD (Cursor Down)      IND (Index)  NEL (Next Line)
     CUU (Cursor Up)        RI  (Reverse Index)
     CUP (Cursor Position)  HVP (Horizontal and Vertical Position)
     ED  (Erase in Display)
     EL  (Erase in Line)
     DECALN (Screen Alignment Display)
     DECAWM (Autowrap)
     <CR> <BS>
     Cursor control characters inside CSI sequences
   */

  int i, row, col, pass, width, hlfxtra;
  const char *ctext = "This is a correct sentence"; 

  set_tty_crmod(true);  /* want to disable tab/space conversion */ 

  for (pass = 0; pass <= 1; pass++) {
    int inner_l, inner_r;

    if (pass == 0) {
      deccolm(false);
      width = min_cols;
    } else {
      deccolm(true);
      width = max_cols;
    }

    /* Compute left/right columns for a 60-column box centered in 'width' */
    inner_l = (width - 60) / 2;
    inner_r = 61 + inner_l;
    hlfxtra = (width - 80) / 2;

    decaln();
    cup(9, inner_l);
    ed(1);
    cup(18, 60 + hlfxtra);
    ed(0);
    el(1);
    cup(9, inner_r);
    el(0);
    /* 132: 36..97 */
    /*  80: 10..71 */
    for (row = 10; row <= 16; row++) {
      cup(row, inner_l);
      el(1);
      cup(row, inner_r);
      el(0);
    }
    cup(17, 30);
    el(2);
    for (col = 1; col <= width; col++) {
      hvp(max_lines, col);
      cb_printf("*");
      hvp(1, col);
      cb_printf("*");
    }
    cup(2, 2);
    for (row = 2; row <= max_lines - 1; row++) {
      cb_printf("+");
      cub(1);
      ind();
    }
    cup(max_lines - 1, width - 1);
    for (row = max_lines - 1; row >= 2; row--) {
      cb_printf("+");
      cub(1);
      ri();
    }
    cup(2, 1);
    for (row = 2; row <= max_lines - 1; row++) {
      cb_printf("*");
      cup(row, width);
      cb_printf("*");
      cub(10);
      if (row < 10)
        nel();
      else
        cb_printf("\r\n");
    }
    cup(2, 10);
    cub(42 + hlfxtra);
    cuf(2);
    for (col = 3; col <= width - 2; col++) {
      cb_printf("+");
      cuf(0);
      cub(2);
      cuf(1);
    }
    cup(max_lines - 1, inner_r - 1);
    cuf(42 + hlfxtra);
    cub(2);
    for (col = width - 2; col >= 3; col--) {
      cb_printf("+");
      cub(1);
      cuf(1);
      cub(0);
      cb_printf("%c", 8);
    }
    cup(1, 1);
    cuu(10);
    cuu(1);
    cuu(0);
    cup(max_lines, width);
    cud(10);
    cud(1);
    cud(0);

    cup(10, 2 + inner_l);
    for (row = 10; row <= 15; row++) {
      for (col = 2 + inner_l; col <= inner_r - 2; col++)
        cb_printf(" ");
      cud(1);
      cub(58);
    }
    cuu(5);
    cuf(1);
    cb_printf("The screen should be cleared,  and have an unbroken bor-");
    cup(12, inner_l + 3);
    cb_printf("der of *'s and +'s around the edge,   and exactly in the");
    cup(13, inner_l + 3);
    cb_printf("middle  there should be a frame of E's around this  text");
    cup(14, inner_l + 3);
    cb_printf("with  one (1) free position around it.    ");
    holdit();
  }
  deccolm(false);

  /* DECAWM demo */
  for (pass = 0; pass <= 1; pass++) {
    static char on_left[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static char on_right[] = "abcdefghijklmnopqrstuvwxyz";
    int height = sizeof(on_left) - 1;
    int region = max_lines - 6;

    /* note: DECCOLM clears the screen */
    if (pass == 0) {
      deccolm(false);
      width = min_cols;
    } else {
      deccolm(true);
      width = max_cols;
    }

    println("Test of autowrap, mixing control and print characters.");
    println("The left/right margins should have letters in order:");

    decstbm(3, region + 3);
    decom(true);  /* this also homes the cursor */
    for (i = 0; i < height; ++i) {
      switch (i % 4) {
      case 0:
        /* draw characters as-is, for reference */
        __(cup(region + 1, 1), cb_printf("%c", on_left[i]));
        __(cup(region + 1, width), cb_printf("%c", on_right[i]));
        cb_printf("\r\n");
        break;
      case 1:
        /* simple wrapping */
        __(cup(region, width), cb_printf("%c%c", on_right[i - 1], on_left[i]));
        /* backspace at right margin */
        __(cup(region + 1, width), cb_printf("%c%c %c",
                                          on_left[i], BS, on_right[i]));
        cb_printf("\r\n");
        break;
      case 2:
        /* tab to right margin */
        __(cup(region + 1, width), cb_printf("%c%c%c%c%c%c",
                                          on_left[i], BS, BS,
                                          TAB, TAB, on_right[i]));
        __(cup(region + 1, 2), cb_printf("%c%c\r\n", BS, on_left[i]));
        break;
      default:
        /* newline at right margin */
        __(cup(region + 1, width), cb_printf("\r\n"));
        __(cup(region, 1), cb_printf("%c", on_left[i]));
        __(cup(region, width), cb_printf("%c", on_right[i]));
        break;
      }
    }
    decom(false);
    decstbm(0, 0);
    cup(max_lines - 2, 1);
    holdit();
  }
  deccolm(false);   /* 80 cols */

  ed(2);
  cup(1, 1);
  println("Test of cursor-control characters inside ESC sequences.");
  println("Below should be four identical lines:");
  println("");
  println("A B C D E F G H I");
  for (i = 1; i < 10; i++) {
    cb_printf("%c", '@' + i);
    do_csi("2%cC", BS);   /* Two forward, one backspace */
  }
  println("");
  /* Now put CR in CUF sequence. */
  cb_printf("A ");
  for (i = 2; i < 10; i++)
    cb_printf("%s%c%dC%c", csi_output(), CR, 2 * i - 2, '@' + i);
  println("");
  /* Now put VT in CUU sequence. */
  rm("20");
  for (i = 1; i < 10; i++) {
    cb_printf("%c ", '@' + i);
    do_csi("1\013A");
  }
  println("");
  println("");
  holdit();

  ed(2);
  cup(1, 1);
  println("Test of leading zeros in ESC sequences.");
  cb_printf("Two lines below you should see the sentence \"%s\".", ctext);
  for (col = 1; *ctext; col++)
    cb_printf("%s00000000004;00000000%dH%c", csi_output(), col, *ctext++);
  cup(20, 1);

  restore_ttymodes();
  return MENU_HOLD;
}

/* Scrolling test (used also in color-testing) */
void
do_scrolling(void)
{
  int soft, first, last, down, i, pass;

  ed(2);
  decom(true);  /* Origin mode (relative) */
  for (soft = -1; soft <= 0; soft++) {
    if (soft)
      decsclm(true);
    else
      decsclm(false);
    for (pass = 0; pass < 2; ++pass) {
      if (pass == 0) {
        first = max_lines / 2;
        last = first + 1;
      } else {
        first = 1;
        last = max_lines;
      }
      decstbm(first, last);
      ed(2);
      for (down = 0; down >= -1; down--) {
        if (down)
          cuu(max_lines);
        else
          cud(max_lines);
        for (i = 1; i <= max_lines + 5; i++) {
          cb_printf("%s scroll %s region [%d..%d] size %d Line %d\r\n",
                 soft ? "Soft" : "Jump",
                 down ? "down" : "up",
                 first, last, last - first + 1, i);
          if (down) {
            ri();
            ri();
          }
        }
      }
      holdit();
    }
  }
}

int
tst_screen(const char*)
{
  /* Test of:
     - DECSTBM (Set Top and Bottom Margins)
     - TBC     (Tabulation Clear)
     - HTS     (Horizontal Tabulation Set)
     - SM RM   (Set/Reset mode): - 80/132 chars
     .                           - Origin: Relative/absolute
     .                           - Scroll: Smooth/jump
     .                           - Wraparound
     - SGR     (Select Graphic Rendition)
     - SM RM   (Set/Reset Mode) - Inverse
     - DECSC   (Save Cursor)
     - DECRC   (Restore Cursor)
   */

  int i, j, cset, row, col, background;

  static const char *tststr = "*qx`";
  static const char *attr[5] =
  {
    ";0", ";1", ";4", ";5", ";7"
  };

  set_tty_crmod(true); /* want to disable tab/space conversion */

  cup(1, 1);
  decawm(true); /* DECAWM: Wrap Around ON */
  for (col = 1; col <= min_cols * 2; col++)
    cb_printf("*");
  decawm(false);  /* DECAWM: Wrap Around OFF */
  cup(3, 1);
  for (col = 1; col <= min_cols * 2; col++)
    cb_printf("*");
  decawm(true); /* DECAWM: Wrap Around ON */
  cup(5, 1);
  println("This should be three identical lines of *'s completely filling");
  println("the top of the screen without any empty lines between.");
  println("(Test of WRAP AROUND mode setting.)");
  holdit();

  ed(2);
  tbc(3);
  cup(1, 1);
  for (col = 1; col <= min_cols - 2; col += 3) {
    cuf(3);
    hts();
  }
  cup(1, 4);
  for (col = 4; col <= min_cols - 2; col += 6) {
    tbc(0);
    cuf(6);
  }
  cup(1, 7);
  tbc(1);
  tbc(2);       /* no-op */
  cup(1, 1);
  for (col = 1; col <= min_cols - 2; col += 6)
    cb_printf("%c*", TAB);
  cup(2, 2);
  for (col = 2; col <= min_cols - 2; col += 6)
    cb_printf("     *");
  cup(4, 1);
  println("Test of TAB setting/resetting. These two lines");
  cb_printf("should look the same. ");
  holdit();
  for (background = 0; background <= 1; background++) {
    if (background)
      decscnm(false);
    else
      decscnm(true);
    deccolm(true);  /* 132 cols */
    ed(2);      /* VT100 clears screen on SM3/RM3, but not obviously, so... */
    cup(1, 1);
    tbc(3);
    for (col = 1; col <= max_cols; col += TABWIDTH) {
      cuf(TABWIDTH);
      hts();
    }
    cup(1, 1);
    for (col = 1; col <= max_cols; col += 10)
      cb_printf("%.*s", (max_cols > col) ? (max_cols - col) : 10, "1234567890");
    for (row = 3; row <= 20; row++) {
      cup(row, row);
      cb_printf("This is %d column mode, %s background.", max_cols,
             background ? "dark" : "light");
    }
    holdit();
    deccolm(false);   /* 80 cols */
    ed(2);      /* VT100 clears screen on SM3/RM3, but not obviously, so... */
    cup(1, 1);
    for (col = 1; col <= min_cols; col += 10)
      cb_printf("%.*s", (min_cols > col) ? (min_cols - col) : 10, "1234567890");
    for (row = 3; row <= 20; row++) {
      cup(row, row);
      cb_printf("This is %d column mode, %s background.", min_cols,
             background ? "dark" : "light");
    }
    holdit();
  }
  do_scrolling();
  ed(2);
  decstbm(max_lines - 1, max_lines);
  cb_printf(
          "\r\nOrigin mode test. This line should be at the bottom of the screen.");
  cup(1, 1);
  cb_printf("%s",
         "This line should be the one above the bottom of the screen. ");
  holdit();
  ed(2);
  decom(false); /* Origin mode (absolute) */
  cup(max_lines, 1);
  cb_printf(
          "Origin mode test. This line should be at the bottom of the screen.");
  cup(1, 1);
  cb_printf("%s", "This line should be at the top of the screen. ");
  holdit();
  decstbm(1, max_lines);

  ed(2);
  /* *INDENT-OFF* */
  cup( 1,20); cb_printf("Graphic rendition test pattern:");
  cup( 4, 1); sgr("0");         cb_printf("vanilla");
  cup( 4,40); sgr("0;1");       cb_printf("bold");
  cup( 6, 6); sgr(";4");        cb_printf("underline");
  cup( 6,45);sgr(";1");sgr("4");cb_printf("bold underline");
  cup( 8, 1); sgr("0;5");       cb_printf("blink");
  cup( 8,40); sgr("0;5;1");     cb_printf("bold blink");
  cup(10, 6); sgr("0;4;5");     cb_printf("underline blink");
  cup(10,45); sgr("0;1;4;5");   cb_printf("bold underline blink");
  cup(12, 1); sgr("1;4;5;0;7"); cb_printf("negative");
  cup(12,40); sgr("0;1;7");     cb_printf("bold negative");
  cup(14, 6); sgr("0;4;7");     cb_printf("underline negative");
  cup(14,45); sgr("0;1;4;7");   cb_printf("bold underline negative");
  cup(16, 1); sgr("1;4;;5;7");  cb_printf("blink negative");
  cup(16,40); sgr("0;1;5;7");   cb_printf("bold blink negative");
  cup(18, 6); sgr("0;4;5;7");   cb_printf("underline blink negative");
  cup(18,45); sgr("0;1;4;5;7"); cb_printf("bold underline blink negative");
  /* *INDENT-ON* */

  sgr("");

  decscnm(false);   /* Inverse video off */
  cup(max_lines - 1, 1);
  el(0);
  cb_printf("Dark background. ");
  holdit();

  decscnm(true);  /* Inverse video */
  cup(max_lines - 1, 1);
  el(0);
  cb_printf("Light background. ");
  holdit();

  decscnm(false);

  ed(2);
  /* *INDENT-OFF* */
  cup(8,12); cb_printf("normal");
  cup(8,24); cb_printf("bold");
  cup(8,36); cb_printf("underscored");
  cup(8,48); cb_printf("blinking");
  cup(8,60); cb_printf("reversed");
  cup(10,1); cb_printf("stars:");
  cup(12,1); cb_printf("line:");
  cup(14,1); cb_printf("x'es:");
  cup(16,1); cb_printf("diamonds:");
  /* *INDENT-ON* */

  for (cset = 0; cset <= 3; cset++) {
    for (i = 0; i <= 4; i++) {
      cup(10 + 2 * cset, 12 + 12 * i);
      sgr(attr[i]);
      if (cset == 0 || cset == 2)
        scs_normal();
      else
        scs_graphics();
      for (j = 0; j <= 4; j++) {
        cb_printf("%c", tststr[cset]);
      }
      decsc();
      cup(cset + 1, i + 1);
      sgr("");
      scs_normal();
      cb_printf("A");
      decrc();
      for (j = 0; j <= 4; j++) {
        cb_printf("%c", tststr[cset]);
      }
    }
  }

  sgr("0");
  scs_normal();
  cup(21, 1);
  println("Test of the SAVE/RESTORE CURSOR feature. There should");
  println("be ten characters of each flavour, and a rectangle");
  println("of 5 x 4 A's filling the top left of the screen.");

  restore_ttymodes();
  return MENU_HOLD;
}

int
tst_doublesize(const char*)
{
  /* Test of:
     DECSWL  (Single Width Line)
     DECDWL  (Double Width Line)
     DECDHL  (Double Height Line) (also implicit double width)
   */

  int col, i, w, w1;

  /* Print the test pattern in both 80 and 132 character width  */

  for (w = 0; w <= 1; w++) {
    w1 = 13 * w;

    ed(2);
    cup(1, 1);
    if (w) {
      deccolm(true);
      cb_printf("%3d column mode", max_cols);
    } else {
      deccolm(false);
      cb_printf("%3d column mode", min_cols);
    }

    cup(5, 3 + 2 * w1);
    cb_printf("v------- left margin");

    cup(7, 3 + 2 * w1);
    cb_printf("This is a normal-sized line");

    decdhl(0);
    decdhl(1);
    decdwl();
    decswl();
    cup(9, 2 + w1);
    cb_printf("This is a Double-width line");

    decswl();
    decdhl(0);
    decdhl(1);
    decdwl();

    cup(11, 2 + w1);
    decdwl();
    decswl();
    decdhl(1);
    decdhl(0);
    cb_printf("This is a Double-width-and-height line");

    cup(12, 2 + w1);
    decdwl();
    decswl();
    decdhl(0);
    decdhl(1);
    cb_printf("This is a Double-width-and-height line");

    cup(14, 2 + w1);
    decdwl();
    decswl();
    decdhl(1);
    decdhl(0);
    el(2);
    cb_printf("This is another such line");

    cup(15, 2 + w1);
    decdwl();
    decswl();
    decdhl(0);
    decdhl(1);
    cb_printf("This is another such line");

    cup(17, 3 + 2 * w1);
    cb_printf("^------- left margin");

    cup(21, 1);
    cb_printf("This is not a double-width line");
    for (i = 0; i <= 1; i++) {
      cup(21, 6);
      if (i) {
        cb_printf("**is**");
        decdwl();
      } else {
        cb_printf("is not");
        decswl();
      }
      cup(max_lines - 1, 1);
      holdit();
    }
  }
  /* Set vanilla tabs for next test */
  cup(1, 1);
  tbc(3);
  for (col = 1; col <= max_cols; col += TABWIDTH) {
    cuf(TABWIDTH);
    hts();
  }
  deccolm(false);
  ed(2);
  /* *INDENT-OFF* */
  scs_graphics();
  cup( 8,1); decdhl(0); cb_printf("lqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqk");
  cup( 9,1); decdhl(1); cb_printf("lqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqk");
  cup(10,1); decdhl(0); cb_printf("x%c%c%c%c%cx",9,9,9,9,9);
  cup(11,1); decdhl(1); cb_printf("x%c%c%c%c%cx",9,9,9,9,9);
  cup(12,1); decdhl(0); cb_printf("x%c%c%c%c%cx",9,9,9,9,9);
  cup(13,1); decdhl(1); cb_printf("x%c%c%c%c%cx",9,9,9,9,9);
  scs(1, '0');  /* should look the same as scs_graphics() */
  cup(14,1); decdhl(0); cb_printf("x                                      x");
  cup(15,1); decdhl(1); cb_printf("x                                      x");
  cup(16,1); decdhl(0); cb_printf("mqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqj");
  cup(17,1); decdhl(1); cb_printf("mqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqj");
  scs_normal();
  /* *INDENT-ON* */

  sgr("1;5");
  cup(12, 3);
  cb_printf("* The mad programmer strikes again * ");

  cup(13, 3);
  cb_printf("%c", 9);

  cub(6);
  cb_printf("* The mad programmer strikes again *");
  sgr("0");

  cup(max_lines - 2, 1);
  println("Another test pattern...  a frame with blinking bold text,");
  cb_printf("all in double-height double-width size. ");
  holdit();

  decstbm(8, max_lines);  /* Absolute origin mode, so cursor is set at (1,1) */
  cup(8, 1);
  for (i = 1; i <= 12; i++) {
    ri();
  }
  decstbm(0, 0);  /* No scroll region     */
  cup(1, 1);
  cb_printf("%s", "Exactly half of the box should remain. ");
  return MENU_HOLD;
}

int
tst_insdel(const char*)
{
  /* Test of:
     SM/RM(4) (= IRM (Insertion/replacement mode))
     ICH (Insert Character)
     DCH (Delete character)
     IL  (Insert line)
     DL  (Delete line)
   */

  int i, row, col, sw, dblchr, scr132;

  for (scr132 = 0; scr132 <= 1; scr132++) {
    if (scr132) {
      deccolm(true);
      sw = max_cols;
    } else {
      deccolm(false);
      sw = min_cols;
    }
    ed(2);
    cup(1, 1);
    for (row = 1; row <= max_lines; row++) {
      cup(row, 1);
      for (col = 1; col <= sw; col++)
        cb_printf("%c", 'A' - 1 + row);
    }
    cup(4, 1);
    cb_printf("Screen accordion test (Insert & Delete Line). ");
    holdit();

    ri();
    el(2);
    decstbm(2, max_lines - 1);
    decom(true);
    cup(1, 1);
    for (row = 1; row <= max_lines; row++) {
      il(row);
      dl(row);
    }
    decom(false);
    decstbm(0, 0);
    cup(2, 1);
    cb_printf("Top line: A's, bottom line: %c's, this line, nothing more. ",
           'A' - 1 + max_lines);
    holdit();
    cup(2, 1);
    ed(0);
    cup(1, 2);
    cb_printf("B");
    cub(1);
    sm("4");
    for (col = 2; col <= sw - 1; col++)
      cb_printf("*");
    rm("4");
    cup(4, 1);
    cb_printf("Test of 'Insert Mode'. The top line should be 'A*** ... ***B'. ");
    holdit();
    ri();            // Reverse index.
    el(2);           // Erase entire current line..
    cup(1, 2);
    dch(sw - 2);
    cup(4, 1);
    cb_printf("Test of 'Delete Character'. The top line should be 'AB'. ");
    holdit();

    for (dblchr = 1; dblchr <= 2; dblchr++) {
      ed(2);
      for (row = 1; row <= max_lines; row++) {
        cup(row, 1);
        if (dblchr == 2)
          decdwl();
        for (col = 1; col <= sw / dblchr; col++)
          cb_printf("%c", 'A' - 1 + row);
        cup(row, sw / dblchr - row);
        dch(row);
      }
      cup(4, 1);
      println("The right column should be staggered ");
      cb_printf("by one.  ");
      holdit();
    }
    ed(2);
    cup(1, 1);
    println("If your terminal has the ANSI 'Insert Character' function");
    println("(the VT102 does not), then you should see a line like this");
    println("  A B C D E F G H I J K L M N O P Q R S T U V W X Y Z");
    println("below:");
    println("");
    for (i = 'Z'; i >= 'A'; i--) {
      cb_printf("%c%c", i, BS);
      ich(2);
    }
    cup(10, 1);
    holdit();

    if (sw == max_cols)
      deccolm(false);
  }
  return MENU_NOHOLD;
}

/*  Test of some known VT100 bugs and misfeatures  */

int
tst_bugs(const char*)
{
  int i;
  /* *INDENT-OFF* */
  static MENU menutable[] = {
    { "Exit to main menu",                                   0 },
    { "Bug A: Smooth scroll to jump scroll",                 bug_a },
    { "Bug B: Scrolling region",                             bug_b },
    { "Bug C: Wide to narrow screen",                        bug_c },
    { "Bug D: Narrow to wide screen",                        bug_d },
    { "Bug E: Cursor move from double- to single-wide line", bug_e },
    { "Bug F: Column mode escape sequence",                  bug_f },
    { "Wrap around with cursor addressing",                  bug_w },
    { "Erase right half of double width lines",              bug_l },
    { "Funny scroll regions",                                bug_s },
    /* Add more here */
    { "",                                                    0 }
  };
  /* *INDENT-ON* */

  static const char *hmsg[] =
  {
    "Test of known bugs in the DEC VT100 series. The numbering of some of",
    "the bugs (A-F) refers to the article 'VT100 MAGIC' by Sami Tabih in",
    "the 'Proceedings of the DEC Users Society' at St. Louis, Missouri, May",
    "1983. To understand some of the tests, you have to look at the source",
    "code or the article. Of course, a good VT100-compatible terminal",
    "should not have these bugs (or have some means of disabling them)! If",
    "a bug appears, you might want to RESET the terminal before continuing",
    "the test. There is a test of the RESET function in the main menu.",
    ""};

  do {
    ed(2);
    cup(1, 1);
    for (i = 0; *hmsg[i]; i++)
      println(hmsg[i]);
    println("");
    println("          Choose bug test number:");
  } while (menu(menutable));
  return MENU_NOHOLD;
}

/* Bug A: Smooth scroll to jump scroll */

int
bug_a(const char*)
{
  int i;

  cup(10, 1);
  println("This is a test of the VT100 'Scroll while toggle softscroll'");
  println("bug.  The cursor may disappear, or move UP the screen, or");
  println("multiple copies of some lines may appear.");
  holdit();

  /*  Invoke the bug  */

  esc("[24H");  /* Simplified cursor movement   */
  decsclm(false);
  for (i = 1; i <= 20; i++)
    cb_printf("\r\n");

  decsclm(true);
  for (i = 1; i <= 10; i++)
    cb_printf("\r\n");

  decsclm(false);
  for (i = 1; i <= 5; i++)
    cb_printf("\r\n");

  /* That should be enough to show the bug. But we'll try another way:  */
  decsclm(true);  /* Set soft scroll              */
  nel();        /* "NextLine", move down        */
  decsclm(false);   /* Reset soft scroll            */
  nel();        /* "NextLine", move down        */
  for (i = 1; i <= 10; i++) {   /* Show the bug                 */
    cb_printf("Softscroll bug test, line %d.  ", i);
    holdit();
  }
  println("That should have been enough to show the bug, if present.");
  return MENU_HOLD;
}

/*  Bug B: Scrolling region  */

int
bug_b(const char*)
{
  char c;

  decaln();

  cup(1, 1);
  el(0);
  cb_printf("Line 11 should be double-wide, line 12 should be cleared.");

  cup(2, 1);
  el(0);
  cb_printf("Then, the letters A-P should be written at the beginning");

  cup(3, 1);
  el(0);
  cb_printf("of lines 12-%d, and the empty line and A-E are scrolled away.", max_lines);

  cup(4, 1);
  el(0);
  cb_printf("If the bug is present, some lines are confused, look at K-P.");

  cup(11, 1);
  decdwl();
  decstbm(12, max_lines);

  cup(12, 1);
  el(0);
  cb_printf("Here we go... ");
  holdit();

  cup(12, 1);
  ri();         /* Bug comes here */
  for (c = 'A'; c <= 'P'; c++)
    cb_printf("%c\r\n", c);  /* Bug shows here */
  holdit();
  decstbm(0, 0);  /* No scr. region */
  return MENU_NOHOLD;
}

/*  Bug C: Wide to narrow screen  */

int
bug_c(const char*)
{
  deccolm(true);  /* 132 column mode */
  cup(1, 81);
  deccolm(false);   /*  80 column mode */
  cup(12, 5);
  cb_printf("Except for this line, the screen should be blank. ");
  return MENU_HOLD;
}

/*  Bug D: Narrow to wide screen  */

int
bug_d(const char*)
{
  int i;
  char result;
  /* Make the bug appear */
  do {
    cup(14, 1);

    /* The original code in the article says
     * PRINT ESC$; "[13;1H"; CHR$(10%);
     * but I guess a cup(14,1); would do.
     * (To output a pure LF might be tricky).
     */

    deccolm(true);  /* Make the bug visible */

    cup(1, 9);
    decdwl();
    println("You should see blinking text at the bottom line.");

    cup(3, 9);
    decdwl();
    println("Enter 0 to exit, 1 to try to invoke the bug again.");

    cup(max_lines, 9);
    decdwl();
    sgr("1;5;7");
    cb_printf("If you can see this then the bug did not appear.");
    sgr("");

    cup(4, 9);
    decdwl();
    result = InChar();
    ConsumeTillNL();
    deccolm(false);
  } while (result == '1');
  decsclm(true);  /* Syrup scroll */
  cup(max_lines - 1, 1);
  for (i = 1; i <= 5; i++)
    println("If the bug is present, this should make things much worse!");
  holdit();
  decsclm(false);   /* Jump scroll */
  return MENU_NOHOLD;
}

/*  Bug E: Cursor move from double- to single-wide line  */

int
bug_e(const char*)
{
  int i;
  static const char *rend[2] =
  {"m", "7m"};
  deccolm(true);
  cup(1, 1);
  decdwl();
  println("This test should put an 'X' at line 3 column 100.");
  for (i = 1; i <= 12; i++)
    cb_printf("1234567890%s%s", csi_output(), rend[i & 1]);
  cup(1, 1);    /* The bug appears when we jump from a double-wide line */
  cup(3, 100);  /* to a single-wide line, column > 66.                  */
  cb_printf("X");
  cup(4, max_cols / 2);
  cb_printf("!                                 !");
  cup(5, 1);
  cb_printf("--------------------------- The 'X' should NOT be above here -");
  cb_printf("---+------------ but above here -----+");
  cup(10, 1);
  decdwl();
  holdit();
  deccolm(false);
  return MENU_NOHOLD;
}

/*  Bug F: Column mode escape sequence  */

int
bug_f(const char*)
{
  /*
   *  VT100 "toggle origin mode, forget rest" bug.  If you try to set
   *     (or clear) parameters and one of them is the "origin mode"
   *     ("?6") parameter, parameters that appear after the "?6"
   *     remain unaffected.  This is also true on CIT-101 terminals.
   */
  decscnm(true);  /* Set reverse mode             */
  deccolm(true);  /* Set 132 column mode          */
  println("Test VT100 'Toggle origin mode, forget rest' bug, part 1.");
  cb_printf("The screen should be in reverse, %d column mode.\r\n", max_cols);
  holdit();
  ed(2);
  rm("?6;5;3"); /* Reset (origin, reverse, 132 col)     */
  println("Test VT100 'Toggle origin mode, forget rest' bug, part 2.\r\n");
  cb_printf("The screen should be in non-reverse, %d column mode.\r\n", min_cols);
  return MENU_HOLD;
}

  /*    Bug W:
   *    The dreaded "wraparound" bug!  You CUP to col 80, write a char,
   *    CUP to another line in col 80, write a char. And the brain-damaged
   *    terminal thinks that "Hokay, so he's written a char in col 80, so
   *    I stay in col 80 and wait for next character. Let's see now, here
   *    comes another character, and I'm still in col 80, so I must make
   *    a NewLine first." -- It doesn't clear that "still in col 80" flag
   *    on a CUP. Argh!
   */

int
bug_w(const char*)
{
  int row, col;

  cup(16, 1);
  println("   This illustrates the \"wrap around bug\" which exists on a");
  println("   standard VT100. At the top of the screen there should be");
  println("   a row of +'s, and the rightmost column should be filled");
  println("   with *'s. But if the bug is present, some of the *'s may");
  println("   be placed in other places, e.g. in the leftmost column,");
  println("   and the top line of +'s may be scrolled away.");

  cup(1, 1);
  for (col = 1; col <= min_cols - 1; col++)
    cb_printf("+");
  for (row = 1; row <= max_lines; row++) {
    hvp(row, min_cols);
    cb_printf("*");
  }
  cup(max_lines, 1);
  return MENU_HOLD;
}

  /*    Bug L:
   *    Check if the right half of double-width lines comes back
   *    when a line is first set to single-width, filled with stuff,
   *    set to double-width, and finally reset to single-width.
   *
   *    A VT100 has this misfeature, and many others. Foo!
   */

int
bug_l(const char*)
{
  cup(15, 1);
  cb_printf("This-is-a-long-line-This-is-a-long-line-");
  cb_printf("This-is-a-long-line-This-is-a-long-line-");
  cup(1, 1);
  cb_printf("This is a test of what happens to the right half of double-width");
  println(" lines.");
  cb_printf("A common misfeature is that the right half does not come back");
  println(" when a long");
  cb_printf("single-width line is set to double-width and then reset to");
  println(" single-width.");

  cup(5, 1);
  println("Now the line below should contain 80 characters in single width.");
  holdit();
  cup(15, 1);
  decdwl();
  cup(8, 1);
  println("Now the line below should contain 40 characters in double width.");
  holdit();
  cup(15, 1);
  decswl();
  cup(11, 1);
  println("Now the line below should contain 80 characters in single width.");
  holdit();

  /* ...and in 132 column mode  */

  deccolm(true);
  ed(2);
  cup(15, 1);
  cb_printf("This-is-a-long-line-This-is-a-long-line-");
  cb_printf("This-is-a-long-line-This-is-a-long-line-");
  cb_printf("This-is-a-long-line-This-is-a-long-line-");
  cb_printf("ending-here-");

  cup(1, 1);
  cb_printf("This is the same test in %d column mode.", max_cols);

  cup(5, 1);
  cb_printf("Now the line below should contain %d characters in single width.\r\n", max_cols);
  holdit();

  cup(15, 1);
  decdwl();

  cup(8, 1);
  cb_printf("Now the line below should contain %d characters in double width.\r\n",
         max_cols / 2);
  holdit();

  cup(15, 1);
  decswl();

  cup(11, 1);
  cb_printf("Now the line below should contain %d characters in single width.\r\n", max_cols);

  holdit();
  deccolm(false);
  return MENU_NOHOLD;
}

int
bug_s(const char*)
{
  int i;
  decstbm(20, 10);  /* 20-10=-10, < 2, so no scroll region. */
  cup(1, 1);
  for (i = 1; i <= 20; i++)
    cb_printf("This is 20 lines of text (line %d), no scroll region.\r\n", i);
  holdit();
  ed(2);
  decstbm(0, 1);  /* Should be interpreted as decstbm(1,1) = none */
  cup(1, 1);
  for (i = 1; i <= 20; i++)
    cb_printf("This is 20 lines of text (line %d), no scroll region.\r\n", i);
  holdit();
  decstbm(0, 0);  /* No scroll region (just in case...)   */
  return MENU_NOHOLD;
}

  /* Set up my personal prejudices      */
int
setup_terminal(const char*)
{
  default_level();  /* Enter ANSI mode (if in VT52 mode)    */
  decckm(false);  /* cursor keys normal   */
  deccolm(false);   /* 80 col mode          */
  decsclm(false);   /* Jump scroll          */
  decscnm(false);   /* Normal screen        */
  decom(false); /* Absolute origin mode */
  decawm(true); /* Wrap around on       */
  decarm(false);  /* Auto repeat off      */
  sm("?40");    /* Enable 80/132 switch (xterm) */
  rm("?45");    /* Disable reverse wrap (xterm) */
  decstbm(0, 0);  /* No scroll region     */
  sgr("0");     /* Normal character attributes  */

  return MENU_NOHOLD;
}

void
bye(void)
{
  /* Force my personal prejudices upon the poor luser   */
  default_level();  /* Enter ANSI mode (if in VT52 mode)    */
  decckm(false);  /* cursor keys normal   */
  deccolm(false);   /* 80 col mode          */
  decscnm(false);   /* Normal screen        */
  decom(false); /* Absolute origin mode */
  decawm(true); /* Wrap around on       */
  decarm(true); /* Auto repeat on       */
  decstbm(0, 0);  /* No scroll region     */
  sgr("0");     /* Normal character attributes  */

  /* Say goodbye */

  ed(2);
  cup(12, 30);
  cb_printf("That's all, folks!\r\n");
  cb_printf("\r\n\r\n\r\n");
  InFlush();
  restore_ttymodes();
}

int
scanto(const char *str, int *pos, int toc)
{
  char c;
  int result = 0;

  while (toc != (c = str[(*pos)])) {
    *pos += 1;
    if (isdigit(CharOf(c)))
      result = result * 10 + c - '0';
    else
      break;
  }
  if (c == toc) {
    *pos += 1;  /* point past delimiter */
    return (result);
  }
  return (0);
}

int
scan_any(char *str, int *pos, int toc)
{
  int save = *pos;
  int value = scanto(str, pos, ';');
  if (value == 0) {
    *pos = save;
    value = scanto(str, pos, toc);
    if (str[*pos] != '\0')
      value = 0;
  }
  return value;
}

static const char *
push_menu(int number)
{
  const char *saved = current_menu;
  current_menu = (char *) malloc(strlen(saved) + 10);
  sprintf(current_menu, "%s%s%d", saved, *saved ? "." : "", number);
  return saved;
}

static void
pop_menu(const char *saved)
{
  if (current_menu && *current_menu)
    free(current_menu);
  current_menu = (char *) saved;
}

#define end_of_menu(table, number) \
        (table[number].description[0] == '\0')

static void
show_entry(MENU *table, int number)
{
  cb_printf("          %d%c %s\r\n", number,
         (table[number].dispatch == not_impl) ? '*' : '.',
         table[number].description);
}

static int
next_menu(MENU *table, int top, int size)
{
  int last;
  int next = top + size;
  for (last = top; last <= next && !end_of_menu(table, last); ++last) {
    ;
  }
  return (last >= next) ? next : top;
}

static int
prev_menu(int top, int size)
{
  return (top > 1) ? (top - size) : top;
}

int
menu(MENU *table)
{
  int i, tablesize, choice;
  char c;
  char storage[BUFSIZ];
  int pagesize = max_lines - 7 - TITLE_LINE;
  int pagetop = 1;
  int redraw = false;

  tablesize = 0;
  for (i = 0; !end_of_menu(table, i); i++) {
    tablesize++;
  }
  tablesize--;

  for (;;) {
    cup(6, 1);
    ed(0);

    println("");
    show_entry(table, 0);
    for (i = 0; i < pagesize; i++) {
      int j = pagetop + i;
      if (end_of_menu(table, j))
        break;
      show_entry(table, pagetop + i);
    }

    cb_printf("\n          Enter choice number (0 - %d): ", tablesize);
    for (;;) {
      char *s = storage;
      InputLine(s);
      choice = 0;
      redraw = false;
      while ((c = *s++) != '\0') {
        if (c == '*') {
          choice = -1;
          break;
        } else if (c == '?') {
          redraw = true;
          break;
        } else if (tablesize > pagesize && c == 'n') {
          pagetop = next_menu(table, pagetop, pagesize);
          redraw = true;
          break;
        } else if (tablesize > pagesize && c == 'p') {
          pagetop = prev_menu(pagetop, pagesize);
          redraw = true;
          break;
        } else if (c >= '0' && c <= '9') {
          choice = 10 * choice + c - '0';
        } else {
          choice = tablesize + 1;
          break;
        }
      }

      if (redraw)
        break;

      if (choice < 0) {
        for (choice = 0; choice <= tablesize; choice++) {
          ed(2);
          if (table[choice].dispatch != 0) {
            const char *save = push_menu(choice);
            const char *name = table[choice].description;
            if ((*table[choice].dispatch) (name) == MENU_HOLD)
              holdit();
            pop_menu(save);
          }
        }
        return 1;
      } else if (choice <= tablesize) {
        ed(2);
        if (table[choice].dispatch != 0) {
          const char *save = push_menu(choice);
          const char *name = table[choice].description;
          if ((*table[choice].dispatch) (name) != MENU_NOHOLD)
            holdit();
          pop_menu(save);
        }
        return (table[choice].dispatch != 0);
      }
      cb_printf("          Bad choice, try again: ");
    }
  }
}

/*
 * Return updated row-number based on the number of characters printed to the
 * screen, e.g., for test_report_ops() to handle very long results.
 */
int
chrprint2(const char *s, int row, int col)
{
  int i;
  int result = row;
  int tracks = (col += 2);
  char temp[80];

  cb_printf("  ");
  sgr("7");
  cb_printf(" ");
  tracks += 3;

  for (i = 0; s[i] != '\0'; i++) {
    int c = (unsigned char) s[i];
    if (c <= ' ' || c >= '\177') {
      sprintf(temp, "<%d> ", c);
    } else {
      sprintf(temp, "%c ", c);
    }
    tracks += (int) strlen(temp);
    if ((tracks > min_cols) && (col > 1)) {
      cup(++result, col);
      tracks = col + (int) strlen(temp);
    }
    cb_fputs(temp);
  }

  sgr("");
  return result + 1;
}

void
chrprint(const char *s)
{
  chrprint2(s, 1, 1);
}

/*
 * Returns a pointer past the prefix, or null if no match is found
 */
char *
skip_prefix(const char *prefix, char *input)
{
  while (*prefix != '\0') {
    if (*prefix++ != *input++)
      return 0;
  }
  return input;
}

char *
skip_csi(char *input)
{
  if ((unsigned char) *input == CSI) {
    return input + 1;
  }
  return skip_prefix(csi_input(), input);
}

char *
skip_dcs(char *input)
{
  if ((unsigned char) *input == DCS) {
    return input + 1;
  }
  return skip_prefix(dcs_input(), input);
}

char *
skip_ss3(char *input)
{
  if ((unsigned char) *input == SS3) {
    return input + 1;
  }
  return skip_prefix(ss3_input(), input);
}

/*
 * Variant with const params
 */
const char *
skip_prefix_2(const char *prefix, const char *input)
{
  while (*prefix != '\0') {
    if (*prefix++ != *input++)
      return 0;
  }
  return input;
}

const char *
skip_csi_2(const char *input)
{
  if ((unsigned char) *input == CSI) {
    return input + 1;
  }
  return skip_prefix_2(csi_input(), input);
}

const char *
skip_dcs_2(const char *input)
{
  if ((unsigned char) *input == DCS) {
    return input + 1;
  }
  return skip_prefix_2(dcs_input(), input);
}

const char *
skip_ss3_2(const char *input)
{
  if ((unsigned char) *input == SS3) {
    return input + 1;
  }
  return skip_prefix_2(ss3_input(), input);
}

/*
 * Returns a pointer past digits, or null if none are found
 */
char *
skip_digits(char *src)
{
  char *base = src;
  while (*src != '\0' && isdigit(CharOf(*src)))
    src++;
  return (base == src) ? 0 : src;
}

char *
skip_xdigits(char *src)
{
  char *base = src;
  while (*src != '\0' && isxdigit(CharOf(*src)))
    src++;
  return (base == src) ? 0 : src;
}

const char *
skip_digits_2(const char *src)
{
  const char *base = src;
  while (*src != '\0' && isdigit(CharOf(*src)))
    src++;
  return (base == src) ? 0 : src;
}

/*
 * Strip the string terminator (ST) from the given string, returning true if
 * we did this.
 */
int
strip_suffix(char *src, const char *suffix)
{
  int have = (int) strlen(src);
  int want = (int) strlen(suffix);
  if (have > want) {
    have -= want;
    if (!strcmp(src + have, suffix)) {
      src[have] = '\0';
      return true;
    }
  }
  return false;
}

/*
 * Strip the string terminator (ST) from the given string, returning true if
 * we did this.
 */
int
strip_terminator(char *src)
{
  int ok = strip_suffix(src, st_input());
  if (!ok) {
    int have = (int) strlen(src);
    if (have > 0 && (unsigned char) src[have - 1] == ST) {
      ok = true;
      src[--have] = '\0';
    }
  }
  return ok;
}

/* Parse the contents of a report from DECRQSS, returning the data as well */
int
parse_decrqss(char *report, const char *func)
{
  int code = -1;
  char *parse = report;

  if ((parse = skip_dcs(parse)) != 0
      && strip_terminator(parse)
      && strip_suffix(parse, func)) {
    if (!strncmp(parse, "1$r", (size_t) 3))
      code = 1;
    else if (!strncmp(parse, "0$r", (size_t) 3))
      code = 0;
  }

  if (code >= 0) {
    while ((*report++ = parse[3]) != '\0')
      parse++;
  }
  return code;
}

int
title(int offset)
{
  cup(TITLE_LINE + offset, 10);
  if (offset == 0 && *current_menu)
    cb_printf("Menu %s: ", current_menu);
  return 1;     /* used for indenting */
}

static void
my_vfprintf(va_list ap, const char *fmt)
{
  while (*fmt != '\0') {
    if (*fmt == '%') {
      switch (*++fmt) {
      case 'c':
        cb_fputc(va_arg(ap, int));
        break;
      case 'x':
        cb_printf("%x", va_arg(ap, int));
        break;
      case 'd':
        cb_printf("%d", va_arg(ap, int));
        break;
      case 's':
        cb_fputs(va_arg(ap, char *));
        break;
      }
    } else {
      cb_fputc(*fmt);
    }
    fmt++;
  }
}

/*
 * Show a test-result
 */
void
show_result(const char *fmt,...)
{
  va_list ap;

  if (*fmt != ' ')
    cb_fputc(' ');
  va_start(ap, fmt);
  my_vfprintf(ap, fmt);
  va_end(ap);
}

/*
 * Use this to make some complex stuff (such as scrolling) slow enough to see.
 */
void
slowly(void)
{
  if (slow_motion) {
  }
}

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
