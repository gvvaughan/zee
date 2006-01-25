/* Point facility functions
   Copyright (c) 2004 David A. Capello.  All rights reserved.

   This file is part of Zee.

   Zee is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Zee is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License
   along with Zee; see the file COPYING.  If not, write to the Free
   Software Foundation, Fifth Floor, 51 Franklin Street, Boston, MA
   02111-1301, USA.  */

#include <assert.h>

#include "config.h"

#include "main.h"
#include "extern.h"

Point make_point(size_t lineno, size_t offset)
{
  Point pt;

  pt.p = list_next(buf.lines);
  pt.n = lineno;
  pt.o = offset;
  while (lineno > 0) {
    pt.p = list_next(pt.p);
    lineno--;
  }
  return pt;
}

/* Returns '-1', '0' or '1'. */
int cmp_point(Point pt1, Point pt2)
{
  if (pt1.n < pt2.n)
    return -1;
  else if (pt1.n > pt2.n)
    return 1;
  else
    return ((pt1.o < pt2.o) ? -1 :
            (pt1.o > pt2.o) ? 1 : 0);
}

int point_dist(Point pt1, Point pt2)
{
  int size = 0;
  Line *lp;

  if (cmp_point(pt1, pt2) > 0)
    swap_point(&pt1, &pt2);

  for (lp = pt1.p; ; lp = list_next(lp)) {
    size += astr_len(lp->item);

    if (lp == pt1.p)
      size -= pt1.o;

    if (lp == pt2.p) {
      size -= astr_len(lp->item) - pt2.o;
      break;
    }
    else
      size++;
  }

  return size;
}

int count_lines(Point pt1, Point pt2)
{
  if (cmp_point(pt1, pt2) > 0)
    swap_point(&pt1, &pt2);

  return pt2.n - pt1.n;
}

void swap_point(Point *pt1, Point *pt2)
{
  Point pt0 = *pt1;
  *pt1 = *pt2;
  *pt2 = pt0;
}

Point point_min(Buffer *bp)
{
  Point pt;
  pt.p = list_next(bp->lines);
  pt.n = 0;
  pt.o = 0;
  return pt;
}

Point point_max(Buffer *bp)
{
  Point pt;
  pt.p = list_prev(bp->lines);
  pt.n = buf.num_lines;
  pt.o = astr_len(list_prev(bp->lines)->item);
  return pt;
}

Point line_beginning_position(void)
{
  Point pt;

  /* Copy current point position without offset (beginning of
   * line). */
  pt = buf.pt;
  pt.o = 0;

  if (list_next(pt.p) != buf.lines) {
      pt.p = list_next(pt.p);
      pt.n++;
  }

  return pt;
}

Point line_end_position(void)
{
  Point pt;

  /* Copy current point position without offset (beginning of
   * line). */
  pt = buf.pt;
  pt.o = astr_len(pt.p->item);

  if (list_prev(pt.p) != buf.lines) {
    pt.p = list_prev(pt.p);
    pt.n--;
  }

  return pt;
}
