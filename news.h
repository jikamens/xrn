#ifndef NEWS_H
#define NEWS_H

/*
 * $Header: /d/src/cvsroot/xrn/news.h,v 1.2 1994-10-10 18:46:30 jik Exp $
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
 * news.h: internal data structures
 *
 */

#ifndef AVL_H
#include "avl.h"
#endif

typedef long art_num;  /* easy way to pick out variables refering to articles */
typedef long ng_num;   /* easy way to pick out newsgroup variables            */

extern avl_tree *NewsGroupTable;
extern int ActiveGroupsCount;

/* this is indexed via 'current - first' */
struct article {
    unsigned long status;
        /* ART_* */
    char *subject;        /* subject line                         */
    char *author;         /* author name                          */
    char *lines;          /* number of lines in the article       */
    char *filename;       /* name of the article file             */
    long position;        /* header/body seperation point (bytes) */
};


struct newsgroup {
    char *name;        /* name of the group                                 */
    ng_num newsrc;     /* index of this group into Newsrc                   */
    unsigned long status;
        /* NG_* */
    art_num first;     /* first available article number                    */
    art_num last;      /* last article number                               */
    art_num current;   /* current article number                            */
    struct article *articles;
    struct list *nglist;  /* newsgroup entry for unsubscribed groups        */
};


/* article has been read / is unread */
#define ART_READ      0x0001
#define ART_UNREAD    0x0002
/* printed */
#define ART_PRINTED   0x0004
#define ART_UNPRINTED 0x0008
/* article has been fetched / is unfetched */
#define ART_FETCHED   0x0010
#define ART_UNFETCHED 0x0020
/* article is not available */
#define ART_UNAVAIL   0x0040

/* headers */
#define ART_ALL_HEADERS      0x0100
#define ART_STRIPPED_HEADERS 0x0200

/* rotation */
#define ART_ROTATED   0x0400
#define ART_UNROTATED 0x0800

/* saved */
#define ART_SAVED   0x4000
#define ART_UNSAVED 0x8000

/* marked */
#define ART_MARKED   0x1000
#define ART_UNMARKED 0x2000

/* Translated */
#define ART_XLATED	0x10000
#define ART_UNXLATED	0x20000
#define ART_CLEAR       (ART_UNREAD | ART_UNFETCHED | ART_STRIPPED_HEADERS | ART_UNROTATED | ART_UNMARKED | ART_UNXLATED)
#define ART_CLEAR_READ  (ART_READ | ART_UNFETCHED | ART_STRIPPED_HEADERS | ART_UNROTATED | ART_UNMARKED | ART_UNXLATED)

/* helper macros */
#define IS_READ(art)       (((art).status & ART_READ) == ART_READ)
#define IS_UNREAD(art)     (((art).status & ART_UNREAD) == ART_UNREAD)
#define IS_PRINTED(art)    (((art).status & ART_PRINTED) == ART_PRINTED)
#define IS_UNPRINTED(art)  (((art).status & ART_UNPRINTED) == ART_UNPRINTED)
#define IS_FETCHED(art)    (((art).status & ART_FETCHED) == ART_FETCHED)
#define IS_UNFETCHED(art)  (((art).status & ART_UNFETCHED) == ART_UNFETCHED)
#define IS_UNAVAIL(art)    (((art).status & ART_UNAVAIL) == ART_UNAVAIL)
#define IS_ALL_HEADERS(art) (((art).status & ART_ALL_HEADERS) == ART_ALL_HEADERS)
#define IS_ROTATED(art)    (((art).status & ART_ROTATED) == ART_ROTATED)
#define IS_SAVED(art)    (((art).status & ART_SAVED) == ART_SAVED)
#define IS_MARKED(art)    (((art).status & ART_MARKED) == ART_MARKED)
#define IS_XLATED(art)    (((art).status & ART_XLATED) == ART_XLATED)
  
  
#define SET_READ(art) ((art).status &= ~ART_UNREAD, (art).status |= ART_READ)
#define SET_UNREAD(art) ((art).status &= ~ART_READ, (art).status |= ART_UNREAD)
#define SET_PRINTED(art) ((art).status &= ~ART_PRINTED, (art).status |= ART_PRINTED)
#define SET_UNPRINTED(art) ((art).status &= ~ART_UNPRINTED, (art).status |= ART_UNPRINTED)
#define SET_FETCHED(art) ((art).status &= ~ART_UNFETCHED, (art).status |= ART_FETCHED)
#define SET_UNFETCHED(art) ((art).status &= ~ART_FETCHED, (art).status |= ART_UNFETCHED)
#define SET_STRIPPED_HEADERS(art) ((art).status &= ~ART_ALL_HEADERS, (art).status |= ART_STRIPPED_HEADERS)
#define SET_ALL_HEADERS(art) ((art).status &= ~ART_STRIPPED_HEADERS, (art).status |= ART_ALL_HEADERS)
#define SET_UNROTATED(art) ((art).status &= ~ART_ROTATED, (art).status |= ART_UNROTATED)
#define SET_ROTATED(art) ((art).status &= ~ART_UNROTATED, (art).status |= ART_ROTATED)
#define SET_UNAVAIL(art) ((art).status |= ART_UNAVAIL)
#define SET_SAVED(art) ((art).status &= ~ART_UNSAVED, (art).status |= ART_SAVED)
#define SET_MARKED(art) ((art).status &= ~ART_UNMARKED, (art).status |= ART_MARKED)
#define SET_UNMARKED(art) ((art).status &= ~ART_MARKED, (art).status |= ART_UNMARKED)
#define SET_UNXLATED(art) ((art).status &= ~ART_XLATED, (art).status |= ART_UNXLATED)
#define SET_XLATED(art) ((art).status &= ~ART_UNXLATED, (art).status |= ART_XLATED)

#define CLEAR_FILE(art) \
  if ((art).filename != NIL(char)) { \
      (void) unlink((art).filename); \
      SET_UNFETCHED(art); \
      FREE((art).filename); \
      (art).filename = NIL(char); \
  }	

#define CLEAR_SUBJECT(art) \
  if ((art).subject != NIL(char)) { \
      FREE((art).subject); \
      (art).subject = NIL(char); \
  }	

#define CLEAR_AUTHOR(art) \
  if ((art).author != NIL(char)) { \
      FREE((art).author); \
      (art).author = NIL(char); \
  }	

#define CLEAR_LINES(art) \
  if ((art).lines != NIL(char)) { \
      FREE((art).lines); \
      (art).lines = NIL(char); \
  }	

/* newsgroup is subscribed/unsubscribed to */
#define NG_SUB      0x0010
#define NG_UNSUB    0x0020
/* no entry in the .newsrc for this group */
#define NG_NOENTRY  0x0040
/* newsgroup can be posted to / can not be posted to / is moderated */
#define NG_POSTABLE 0x1000
#define NG_UNPOSTABLE 0x2000
#define NG_MODERATED 0x4000

#define IS_SUBSCRIBED(ng) (((ng)->status & NG_SUB) == NG_SUB)
#define IS_NOENTRY(ng)    (((ng)->status & NG_NOENTRY) == NG_NOENTRY)
  
#define SET_SUB(ng)    ((ng)->status &= ~NG_UNSUB, (ng)->status |= NG_SUB)
#define SET_UNSUB(ng)  ((ng)->status &= ~NG_SUB, (ng)->status |= NG_UNSUB)
#define CLEAR_NOENTRY(ng) ((ng)->status &= ~NG_NOENTRY)

#define EMPTY_GROUP(ng) ((ng)->last < (ng)->first)
  
#define CLEAR_ARTICLES(ng) \
  if ((ng)->articles != NIL(struct article)) { \
      FREE((ng)->articles); \
      (ng)->articles = NIL(struct article); \
  }	

#define INDEX(artnum)  (artnum - newsgroup->first)
#define CURRENT        INDEX(newsgroup->current)
#define LAST           INDEX(newsgroup->last)

extern struct newsgroup *CurrentGroup;   /* current index into the newsrc array       */
extern ng_num MaxGroupNumber;       /* size of the newsrc array                  */

struct newsgroup **Newsrc;          /* sequence list for .newsrc file            */

#define NOT_IN_NEWSRC -1            /* must be less than 0 */

/* not a valid group (must be less than 0) */
#define NO_GROUP -1


#define GROUP_NAME_SIZE 128

#endif /* NEWS_H */
