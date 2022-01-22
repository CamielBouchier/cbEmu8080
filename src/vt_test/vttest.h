/* $Id: vttest.h,v 1.96 2012/05/06 19:32:34 tom Exp $ */

#ifndef VTTEST_H
#define VTTEST_H 1

#define VERSION "1.7b 1985-04-19"
#define RELEASE 2
#define PATCHLEVEL 7
#define PATCH_DATE 20120603

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <setjmp.h>
#include <stdint.h>

#include "cb_Defines.h"

#define CharOf(c) ((unsigned char)(c))

class cb_Console;

extern cb_Console* TheConsole;

extern int do_colors;
extern int input_8bits;
extern int lrmm_flag;
extern int max_cols;
extern int max_lines;
extern int min_cols;
extern int origin_mode;
extern int output_8bits;
extern int reading;
extern int slow_motion;
extern int tty_speed;
extern jmp_buf intrenv;

#define SHOW_SUCCESS "ok"
#define SHOW_FAILURE "failed"

#undef __
#define __(a,b) (void)(a && b)

#define TABLESIZE(table) (int)(sizeof(table)/sizeof(table[0]))

#define DEFAULT_SPEED 9600
#define TABWIDTH 8

#define STR_ENABLE(flag)     ((flag) ? "Disable" : "Enable")
#define STR_ENABLED(flag)    ((flag) ? "enabled" : "disabled")
#define STR_START(flag)      ((flag) ? "Stop" : "Start")

typedef struct {
  const char *description;
  int (*dispatch) (const char*);
} MENU;

typedef struct {
  int cur_level;
  int input_8bits;
  int output_8bits;
} VTLEVEL;

typedef struct {
  int mode;
  const char *name;
  int level;
} RQM_DATA;

#define MENU_NOHOLD 0
#define MENU_HOLD   1
#define MENU_MERGE  2

#define TITLE_LINE  3

#define WHITE_ON_BLUE   "0;37;44"
#define WHITE_ON_GREEN  "0;37;42"
#define YELLOW_ON_BLACK "0;33;40"
#define BLINK_REVERSE   "0;5;7"

// Catchers for stdout writing.
extern int cb_printf(const char *fmt,...);
extern int cb_putchar(int c);
extern int cb_fputs(const char* s);
extern int cb_fputc(int c);

extern char origin_mode_mesg[80];
extern char lrmm_mesg[80];
extern char lr_marg_mesg[80];
extern char tb_marg_mesg[80];
extern char txt_override_color[80];

extern char *skip_csi(char *input);
extern char *skip_dcs(char *input);
extern char *skip_digits(char *src);
extern char *skip_prefix(const char *prefix, char *input);
extern char *skip_ss3(char *input);
extern char *skip_xdigits(char *src);
extern const char *parse_Sdesig(const char *source, int *offset);
extern const char *skip_csi_2(const char *input);
extern const char *skip_dcs_2(const char *input);
extern const char *skip_digits_2(const char *src);
extern const char *skip_prefix_2(const char *prefix, const char *input);
extern const char *skip_ss3_2(const char *input);
extern int any_DSR(const char* title, const char *text, void (*explain) (char *report));
extern int any_RQM(const char* title, RQM_DATA * table, int tablesize, int Private);
extern int any_decrqpsr(const char* title, int Ps);
extern int any_decrqss(const char *msg, const char *func);
extern int any_decrqss2(const char *msg, const char *func, const char *expected);
extern int bug_a(const char* title);
extern int bug_b(const char* title);
extern int bug_c(const char* title);
extern int bug_d(const char* title);
extern int bug_e(const char* title);
extern int bug_f(const char* title);
extern int bug_l(const char* title);
extern int bug_s(const char* title);
extern int bug_w(const char* title);
extern int chrprint2(const char *s, int row, int col);
extern int conv_to_utf32(unsigned *target, const char *source, unsigned limit);
extern int conv_to_utf8(unsigned char *target, unsigned source, unsigned limit);
extern int get_bottom_margin(int n);
extern int get_left_margin(void);
extern int get_level(void);
extern int get_right_margin(void);
extern int get_top_margin(void);
extern int main(int argc, char *argv[]);
extern int menu(MENU *table);
extern int not_impl(const char* title);
extern int parse_decrqss(char *report, const char *func);
extern int rpt_DECSTBM(const char* title);
extern int scan_any(char *str, int *pos, int toc);
extern int scanto(const char *str, int *pos, int toc);
extern int set_DECRPM(int level);
extern int set_level(int level);
extern int setup_terminal(const char* title);
extern int strip_suffix(char *src, const char *suffix);
extern int strip_terminator(char *src);
extern int terminal_id(void);
extern int title(int offset);
extern int toggle_DECOM(const char* title);
extern int toggle_LRMM(const char* title);
extern int toggle_SLRM(const char* title);
extern int toggle_STBM(const char* title);
extern int toggle_color_mode(const char* title);
extern int tst_CBT(const char* title);
extern int tst_CHA(const char* title);
extern int tst_CHT(const char* title);
extern int tst_CNL(const char* title);
extern int tst_CPL(const char* title);
extern int tst_DECRPM(const char* title);
extern int tst_DECSTR(const char* title);
extern int tst_DSR_cursor(const char* title);
extern int tst_DSR_keyboard(const char* title);
extern int tst_DSR_locator(const char* title);
extern int tst_DSR_printer(const char* title);
extern int tst_DSR_userkeys(const char* title);
extern int tst_HPA(const char* title);
extern int tst_HPR(const char* title);
extern int tst_SD(const char* title);
extern int tst_SRM(const char* title);
extern int tst_SU(const char* title);
extern int tst_VPA(const char* title);
extern int tst_VPR(const char* title);
extern int tst_bugs(const char* title);
extern int tst_characters(const char* title);
extern int tst_colors(const char* title);
extern int tst_doublesize(const char* title);
extern int tst_ecma48_curs(const char* title);
extern int tst_ecma48_misc(const char* title);
extern int tst_insdel(const char* title);
extern int tst_keyboard(const char* title);
extern int tst_keyboard_layout(char *scs_params);
extern int tst_mouse(const char* title);
extern int tst_movements(const char* title);
extern int tst_nonvt100(const char* title);
extern int tst_printing(const char* title);
extern int tst_reports(const char* title);
extern int tst_rst(const char* title);
extern int tst_screen(const char* title);
extern int tst_setup(const char* title);
extern int tst_softchars(const char* title);
extern int tst_statusline(const char* title);
extern int tst_tek4014(const char* title);
extern int tst_vt220(const char* title);
extern int tst_vt220_device_status(const char* title);
extern int tst_vt220_reports(const char* title);
extern int tst_vt220_screen(const char* title);
extern int tst_vt320(const char* title);
extern int tst_vt320_DECRQSS(const char* title);
extern int tst_vt320_cursor(const char* title);
extern int tst_vt320_device_status(const char* title);
extern int tst_vt320_report_presentation(const char* title);
extern int tst_vt320_reports(const char* title);
extern int tst_vt320_screen(const char* title);
extern int tst_vt420(const char* title);
extern int tst_vt420_DECRQSS(const char* title);
extern int tst_vt420_cursor(const char* title);
extern int tst_vt420_device_status(const char* title);
extern int tst_vt420_report_presentation(const char* title);
extern int tst_vt420_reports(const char* title);
extern int tst_vt52(const char* title);
extern int tst_vt520(const char* title);
extern int tst_vt520_reports(const char* title);
extern int tst_xterm(const char* title);
extern void bye(void);
extern void chrprint(const char *s);
extern void default_level(void);
extern void do_scrolling(void);
extern void finish_vt420_cursor(const char* title);
extern void initterminal(int pn);
extern void menus_vt420_cursor(void);
extern void print_chr(int c);
extern void print_str(const char *s);
extern void reset_level(void);
extern void restore_level(VTLEVEL *save);
extern void save_level(VTLEVEL *save);
extern void scs_graphics(void);
extern void scs_normal(void);
extern void set_colors(const char *value);
extern void setup_softchars(const char *filename);
extern void setup_vt420_cursor(const char* title);
extern void show_mousemodes(void);
extern void show_result(const char *fmt,...);
extern void slowly(void);
extern void test_with_margins(int enable);

#endif /* VTTEST_H */
