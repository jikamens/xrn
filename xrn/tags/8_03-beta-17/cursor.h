#ifndef CURSOR_H
#define CURSOR_H

#include "news.h"

/*
 * $Id: cursor.h,v 1.12 1996-06-11 09:45:59 jik Exp $
 */

/*
 * xrn - an X-based NNTP news reader
 *
 * Copyright (c) 1988-1993, Ellen M. Sentovich and Rick L. Spickelmier.
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

/*
 * cursor.h: functions for manipulating the cursor and/or text in a
 *           text window
 */

#define BACK 0
#define FORWARD 1

extern void moveBeginning _ARGUMENTS((char *,long *));
extern void moveEnd _ARGUMENTS((char *,long *));
extern int moveCursor _ARGUMENTS((int,char *,long *));
extern int moveUpWrap _ARGUMENTS((char *,long *));
extern void endInsertionPoint _ARGUMENTS((char *,long *));
extern int getArtSelection _ARGUMENTS((void));
extern void removeLine _ARGUMENTS((char *, long *));
extern int setCursorCurrent _ARGUMENTS((char *,long *));
extern void currentGroup _ARGUMENTS((int,char *,char **,long));
extern void currentMode _ARGUMENTS((char *,char **,int *,long));
extern int markStringRead _ARGUMENTS((char *,long));
extern void markAllString _ARGUMENTS((char *,long, char *));
extern void markArticles _ARGUMENTS((char *, long, long,
				     /* char */ int));
extern void buildString _ARGUMENTS((char **,long,long,
    char *));
extern int moveToArticle _ARGUMENTS((struct newsgroup *, long, char **, char **));

#endif /* CURSOR_H */
