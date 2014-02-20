/* Plugin for non-shared memory host execution.

   Copyright (C) 2014 Free Software Foundation, Inc.

   Contributed by Thomas Schwinge <thomas@codesourcery.com>.

   This file is part of the GNU OpenMP Library (libgomp).

   Libgomp is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

/* Simple implementation of a libgomp plugin for non-shared memory host
   execution.  */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool
device_available (void)
{
#ifdef DEBUG
  printf ("libgomp plugin: %s:%s\n", __FILE__, __FUNCTION__);
#endif

  return true;
}

void *
device_alloc (size_t size)
{
  void *ptr = malloc (size);

#ifdef DEBUG
  printf ("libgomp plugin: %s:%s (%zd): %p\n", __FILE__, __FUNCTION__, size, ptr);
#endif

  return ptr;
}

void
device_free (void *ptr)
{
#ifdef DEBUG
  printf ("libgomp plugin: %s:%s (%p)\n", __FILE__, __FUNCTION__, ptr);
#endif

  free (ptr);
}

void *device_dev2host (void *dest, const void *src, size_t n)
{
#ifdef DEBUG
  printf ("libgomp plugin: %s:%s (%p, %p, %zd)\n", __FILE__, __FUNCTION__, dest, src, n);
#endif

  return memcpy (dest, src, n);
}

void *device_host2dev (void *dest, const void *src, size_t n)
{
#ifdef DEBUG
  printf ("libgomp plugin: %s:%s (%p, %p, %zd)\n", __FILE__, __FUNCTION__, dest, src, n);
#endif

  return memcpy (dest, src, n);
}
