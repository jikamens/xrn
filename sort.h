/*
 * xrn - an X-based NNTP news reader
 *
 * Copyright (c) 1994-2023, Jonathan Kamens.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of California not
 * be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The University
 * of California makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE UNIVERSITY OF CALIFORNIA DISCLAIMS ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _XRN_SORT_H
#define _XRN_SORT_H

#include "utils.h"

void art_sort_parse_sortlist _ARGUMENTS((char *));
int art_sort_need_dates _ARGUMENTS((void));
int art_sort_need_threads _ARGUMENTS((void));

void *art_sort_init _ARGUMENTS((struct newsgroup *, art_num, art_num));
char *art_sort_done _ARGUMENTS((void *, int));
void art_sort_cancel _ARGUMENTS((void *));

char *art_sort_doit _ARGUMENTS((struct newsgroup *, art_num, art_num, int));

void art_sort_by_subject _ARGUMENTS((void *));
void art_sort_by_date _ARGUMENTS((void *));
void art_sort_by_thread _ARGUMENTS((void *));

#endif /* _XRN_SORT_H */
