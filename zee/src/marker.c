/* Marker facility functions
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
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

/*	$Id$	*/

#include "config.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "zee.h"
#include "extern.h"

static void unchain_marker(Marker *marker)
{
  Marker *m, *next, *prev = NULL;

  if (!marker->bp)
    return;

  for (m=marker->bp->markers; m; m=next) {
    next = m->next;
    if (m == marker) {
      if (prev)
        prev->next = next;
      else
        m->bp->markers = next;

      m->bp = NULL;
      break;
    }
    prev = m;
  }
}

void free_marker(Marker *marker)
{
  unchain_marker(marker);
  free(marker);
}

void move_marker(Marker *marker, Buffer *bp, Point pt)
{
  if (bp != marker->bp) {
    /* Unchain with the previous pointed buffer. */
    unchain_marker(marker);

    /* Change the buffer. */
    marker->bp = bp;

    /* Chain with the new buffer. */
    marker->next = bp->markers;
    bp->markers = marker;
  }

  /* Change the point. */
  marker->pt = pt;
}

Marker *copy_marker(Buffer *bp, Point pt)
{
  Marker *marker = zmalloc(sizeof(Marker));
  move_marker(marker, bp, pt);
  return marker;
}

Marker *point_marker(void)
{
  return copy_marker(cur_bp, cur_bp->pt);
}
