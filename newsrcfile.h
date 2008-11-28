#ifndef NEWSRCFILE_H
#define NEWSRCFILE_H

/*
 * $Id: newsrcfile.h,v 1.9 1998-04-06 11:41:03 jik Exp $
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
 * newsrcfile.h: handle the newsrc file (reading and updating)
 */

#include "news.h"

extern void checkNewsrcSize _ARGUMENTS((ng_num));

extern int isLongNewsrcFile _ARGUMENTS((void));
/* return 1 for okay, 0 for fatal error */
extern int readnewsrc _ARGUMENTS((void));
extern int updatenewsrc _ARGUMENTS((void));

extern FILE *Newsrcfp;
extern char *optionsLine;

/* values for 'type' */
#define SINGLE 1
#define RANGE  2

struct list {
    int type;
    union {
	art_num single;
	struct rn {
	    art_num start;
	    art_num end;
	} range;
    } contents;
    struct list *next;
};

#endif /* NEWSRCFILE_H */
