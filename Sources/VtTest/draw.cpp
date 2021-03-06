/* $Id: draw.c,v 1.11 2012/05/06 14:13:23 tom Exp $ */

#include <vttest.h>
#include <draw.h>
#include <esc.h>

/*
 * Useful drawing functions for vttest.
 */

static int
check_box_params(BOX *box)
{
  if (box->top >= box->bottom || box->left >= box->right) {
    cbprintf("The screen is too small for box with margins %d,%d.", box->top, box->left);
    holdit();
    return -1;
  }
  return 0;
}

/*
 * Compute box params for given vertical and horizontal margin,
 * returns -1 if the screen is too small, 0 otherwise.
 */
int
make_box_params(BOX *box, int vmargin, int hmargin)
{
  box->top = vmargin;
  box->bottom = get_bottom_margin(max_lines) - vmargin;
  box->left = hmargin;
  box->right = get_right_margin() - hmargin;
  return check_box_params(box);
}

void
draw_box_outline(BOX *box, int mark)
{
  int j;
  int tlc = (mark < 0) ? 'l' : mark;
  int trc = (mark < 0) ? 'k' : mark;
  int llc = (mark < 0) ? 'm' : mark;
  int lrc = (mark < 0) ? 'j' : mark;
  int vrt = (mark < 0) ? 'x' : mark;
  int hrz = (mark < 0) ? 'q' : mark;
  int dot;

  if (mark < 0)
    scs(0, '0');

  for (j = box->top, dot = tlc; j < box->bottom; j++) {
    __(cup(j, box->left), cbputchar(dot));
    dot = vrt;
  }
  for (j = box->top, dot = trc; j < box->bottom; j++) {
    __(cup(j, box->right), cbputchar(dot));
    dot = vrt;
  }

  cup(box->top, box->left + 1);
  for (j = box->left + 1; j < box->right; j++)
    cbputchar(hrz);

  cup(box->bottom, box->left + 1);
  for (j = box->left + 1; j < box->right; j++)
    cbputchar(hrz);

  __(cup(box->bottom, box->left), cbputchar(llc));
  __(cup(box->bottom, box->right), cbputchar(lrc));

  if (mark < 0)
    scs(0, 'B');
}

void
draw_box_filled(BOX *box, int mark)
{
  int i, j;
  int ch = (mark < 0) ? 'A' : mark;

  for (i = box->top; i < box->bottom; i++) {
    cup(i, box->left);
    for (j = box->left; j < box->right; j++) {
      cbputchar(ch);
      if (mark < 0)
        ch = ((ch - 'A' + 1) % 26) + 'A';
    }
  }
}

static int
next_word(const char *s)
{
  const char *base;
  while (*s == ' ')
    s++;
  base = s;
  while (*s && *s != ' ')
    s++;
  return (int) (s - base);
}

void
draw_box_caption(BOX *box, int margin, const char **c)
{
  int x0 = (box->left + margin);
  int y0 = (box->top + margin);
  int x1 = (box->right - margin);
  int y1 = (box->bottom - margin);
  int x = x0;
  int y = y0;
  int t;
  const char *s;

  while ((s = *c++) != 0) {
    while ((t = *s++) != 0) {
      if (x == x0) {
        if (t == ' ')
          continue;
        cup(y, x++);
        cbputchar(' ');
      }
      cbputchar(t);
      x++;
      if ((t == ' ') && (next_word(s) > (x1 - x - 2))) {
        while (x < x1) {
          cbputchar(' ');
          x++;
        }
      }
      if (x >= x1) {
        cbputchar(' ');
        x = x0;
        y++;
      }
    }
  }
  while (y <= y1) {
    if (x == x0) {
      cup(y, x);
    }
    cbputchar(' ');
    if (++x >= x1) {
      cbputchar(' ');
      x = x0;
      y++;
    }
  }
}

void
ruler(int row, int width)
{
  int col;
  cup(row, 1);
  for (col = 1; col <= width; ++col) {
    int ch = (((col % 10) == 0)
              ? ('0' + (col / 10) % 10)
              : (((col % 5) == 0)
                 ? '+'
                 : '-'));
    cbputchar(ch);
  }
  cbputchar('\r');cbputchar('\n');
}
