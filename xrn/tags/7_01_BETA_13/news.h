#ifndef NEWS_H
#define NEWS_H

/*
 * $Id: news.h,v 1.5 1995-01-25 03:17:52 jik Exp $
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
typedef short ng_num;   /* easy way to pick out newsgroup variables            */

extern avl_tree *NewsGroupTable;
extern int ActiveGroupsCount;

/* this is indexed via 'current - first' */
struct article {
    unsigned short status; /* ART_* */
    char *subject;        /* subject line                         */
    char *author;         /* author name                          */
    char *lines;          /* number of lines in the article       */
    char *filename;       /* name of the article file             */
    long position;        /* header/body seperation point (bytes) */
};


struct newsgroup {
    char *name;        /* name of the group                                 */
    ng_num newsrc;     /* index of this group into Newsrc                   */
    unsigned char status; /* NG_* */
    art_num first;     /* first available article number                    */
    art_num last;      /* last article number                               */
    art_num current;   /* current article number                            */
    art_num max_killed;/* the highest article KILL files were run on	    */
    struct article *articles;
    struct list *nglist;  /* newsgroup entry for unsubscribed groups        */
};


#define ART_READ	(1<<0)
#define ART_PRINTED	(1<<1)
#define ART_FETCHED	(1<<2)
#define ART_UNAVAIL	(1<<3)
#define ART_ALL_HEADERS	(1<<4)
#define ART_ROTATED	(1<<5)
#define ART_SAVED	(1<<6)
#define ART_MARKED	(1<<7)
#define ART_XLATED	(1<<8)

#define ART_CLEAR       (0)
#define ART_CLEAR_READ  (ART_READ)

/* helper macros */
#define IS_READ(art)       	(   (art).status & ART_READ)
#define IS_UNREAD(art)     	(! ((art).status & ART_READ))
#define IS_PRINTED(art)    	(   (art).status & ART_PRINTED)
#define IS_UNPRINTED(art)  	(! ((art).status & ART_PRINTED))
#define IS_FETCHED(art)    	(   (art).status & ART_FETCHED)
#define IS_UNFETCHED(art)  	(! ((art).status & ART_FETCHED))
#define IS_UNAVAIL(art)    	(   (art).status & ART_UNAVAIL)
#define IS_ALL_HEADERS(art)	(   (art).status & ART_ALL_HEADERS)
#define IS_ROTATED(art)		(   (art).status & ART_ROTATED)
#define IS_SAVED(art)		(   (art).status & ART_SAVED)
#define IS_MARKED(art)		(   (art).status & ART_MARKED)
#define IS_XLATED(art)		(   (art).status & ART_XLATED)
  
  
#define SET_READ(art)			((art).status |= ART_READ)
#define SET_UNREAD(art)			((art).status &= ~ART_READ)
#define SET_PRINTED(art)		((art).status |= ART_PRINTED)
#define SET_UNPRINTED(art)		((art).status &= ~ART_PRINTED)
#define SET_FETCHED(art)		((art).status |= ART_FETCHED)
#define SET_UNFETCHED(art)		((art).status &= ~ART_FETCHED)
#define SET_ALL_HEADERS(art)		((art).status |= ART_ALL_HEADERS)
#define SET_STRIPPED_HEADERS(art)	((art).status &= ~ART_ALL_HEADERS)
#define SET_ROTATED(art)		((art).status |= ART_ROTATED)
#define SET_UNROTATED(art)		((art).status &= ~ART_ROTATED)
#define SET_UNAVAIL(art)		((art).status |= ART_UNAVAIL)
#define SET_SAVED(art)			((art).status |= ART_SAVED)
#define SET_UNSAVED(art)		((art).status &= ~ART_SAVED)
#define SET_MARKED(art)			((art).status |= ART_MARKED)
#define SET_UNMARKED(art)		((art).status &= ~ART_MARKED)
#define SET_UNXLATED(art)		((art).status &= ~ART_XLATED)
#define SET_XLATED(art)			((art).status |= ART_XLATED)

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

#define NG_SUB		(1<<0)	/* newsgroup is subscribed/unsubscribed to */
#define NG_NOENTRY	(1<<1)	/* no entry in the .newsrc for this group */
#define NG_POSTABLE	(1<<2)	/* newsgroup can be posted to */
#define NG_MODERATED	(1<<3)	/* newsgroup is moderated */

#define IS_SUBSCRIBED(ng)	((ng)->status & NG_SUB)
#define IS_NOENTRY(ng)		((ng)->status & NG_NOENTRY)
#define IS_POSTABLE(ng)	  	((ng)->status & NG_POSTABLE)
#define IS_MODERATED(ng)  	((ng)->status & NG_MODERATED)

#define SET_SUB(ng)    		((ng)->status |= NG_SUB)
#define SET_UNSUB(ng)  		((ng)->status &= ~NG_SUB)
#define SET_POSTABLE(ng)	((ng)->status |= NG_POSTABLE)
#define SET_UNPOSTABLE(ng)	((ng)->status &= ~NG_POSTABLE)
#define SET_MODERATED(ng)	((ng)->status |= NG_MODERATED)
#define SET_UNMODERATED(ng)	((ng)->status &= ~NG_MODERATED)

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