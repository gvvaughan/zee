/* Point facility functions
   Copyright (c) 2004 David A. Capello.  All rights reserved.

   This file is part of Zile.

   Zile is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Zile is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License
   along with Zile; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

/*	$Id: point.c,v 1.5 2005/01/09 23:56:05 rrt Exp $	*/

#include "config.h"

#include "zile.h"
#include "extern.h"

Point make_point(int lineno, int offset)
{
  Point pt;
  pt.p = cur_bp->limitp->next;
  pt.n = lineno;
  pt.o = offset;
  while (lineno > 0) {
    pt.p = pt.p->next;
    lineno--;
  }
  return pt;
}

int cmp_point(Point pt1, Point pt2)
{
  if (pt1.n < pt2.n)
    return -1;
  else if (pt1.n > pt2.n)
    return +1;
  else
    return ((pt1.o < pt2.o) ? -1:
            (pt1.o > pt2.o) ? +1: 0);
}

int point_dist(Point pt1, Point pt2)
{
  int size = 0;
  Line *lp;

  if (cmp_point(pt1, pt2) > 0)
    swap_point(&pt1, &pt2);

  for (lp=pt1.p; ; lp=lp->next) {
    size += astr_len(lp->text);

    if (lp == pt1.p)
      size -= pt1.o;

    if (lp == pt2.p) {
      size -= astr_len(lp->text) - pt2.o;
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

Point point_min(void)
{
  Point pt;
  pt.p = cur_bp->limitp->next;
  pt.n = 0;
  pt.o = 0;
  return pt;
}

Point point_max(void)
{
  Point pt;
  pt.p = cur_bp->limitp->prev;
  pt.n = cur_bp->num_lines;
  pt.o = astr_len(cur_bp->limitp->prev->text);
  return pt;
}

Point line_beginning_position(int count)
{
  Point pt;

  count--;

  /* Copy current point position without offset (beginning of
   * line).  */
  pt = cur_bp->pt;
  pt.o = 0;

  if (count < 0) {
    while (count && pt.p->prev != cur_bp->limitp) {
      pt.p = pt.p->prev;
      pt.n--;
      count++;
    }
  }
  else if (count > 0) {
    while (count && pt.p->next != cur_bp->limitp) {
      pt.p = pt.p->next;
      pt.n++;
      count--;
    }
  }

  return pt;
}

Point line_end_position(int count)
{
  Point pt = line_beginning_position(count);
  pt.o = astr_len(pt.p->text);
  return pt;
}

