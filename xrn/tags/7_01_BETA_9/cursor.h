#ifndef CURSOR_H
#define CURSOR_H

/*
 * $Id: cursor.h,v 1.6 1994-12-07 13:46:48 jik Exp $
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

#ifdef MOTIF
#include "MotifXawHack.h"
#endif

extern void moveBeginning _ARGUMENTS((char *,XawTextPosition *));
extern void moveEnd _ARGUMENTS((char *,XawTextPosition *));
extern int moveCursor _ARGUMENTS((int,char *,XawTextPosition *));
extern int moveUpWrap _ARGUMENTS((char *,XawTextPosition *));
extern void endInsertionPoint _ARGUMENTS((char *,XawTextPosition *));
extern int getSelection _ARGUMENTS((Widget,char *,XawTextPosition *,
    XawTextPosition *));
extern int getArtSelection _ARGUMENTS(());
extern void removeLine _ARGUMENTS((char *,Widget,Widget *,XawTextPosition,
    XawTextPosition*));
extern int setCursorCurrent _ARGUMENTS((char *,XawTextPosition *));
extern void currentGroup _ARGUMENTS((int,char *,char *,XawTextPosition));
extern void currentMode _ARGUMENTS((char *,char *,int *,XawTextPosition));
extern int markStringRead _ARGUMENTS((char *,XawTextPosition));
extern void markAllString _ARGUMENTS((char *,XawTextPosition,XawTextPosition,
    char *));
extern void markArticles _ARGUMENTS((char *, XawTextPosition, XawTextPosition,
				     /* char */ int));
extern void buildString _ARGUMENTS((char **,XawTextPosition,XawTextPosition,
    char *));
extern void findArticle _ARGUMENTS((char *,long,XawTextPosition *));
extern int subjectSearch _ARGUMENTS((int,char **,Widget,XawTextPosition *,XawTextPosition,char *,char **,char **,long *));
extern int moveToArticle _ARGUMENTS((long, char **, char **));

#endif /* CURSOR_H */
