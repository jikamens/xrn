#ifndef NEWS_H
#define NEWS_H

/*
 * $Id: news.h,v 1.25 1997-02-14 16:25:43 jik Exp $
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

#include "avl.h"
#include "hash.h"

typedef long art_num;  /* easy way to pick out variables refering to articles */
typedef unsigned short ng_num;   /* easy way to pick out newsgroup variables            */

extern avl_tree *NewsGroupTable;
extern int ActiveGroupsCount;

/* this is indexed via 'current - first' */
struct article {
    unsigned short status; /* ART_* */
    char *subject;        /* subject line                         */
    char *from;		  /* full value of the "From" line	  */
    char *author;         /* author name                          */
    char *lines;          /* number of lines in the article       */
    char *filename;       /* name of the article file             */
    char *newsgroups;	  /* newsgroups list (maybe)		  */
    char *date;		  /* date (maybe)			  */
    char *id;		  /* message ID (maybe)		       	  */
    char *references;	  /* references (maybe)			  */
    char *xref;		  /* xref (maybe)			  */
    art_num parent;	  /* parent article, for threading	  */
    art_num *children;	  /* child articles, for threading	  */
#ifdef ARTSTRUCT_C
    /* These should only be touched in artstruct.c! */
    art_num first;
    struct article *previous, *next;
#else
    art_num dont_touch1;
    struct article *dont_touch2, *dont_touch3;
#endif
};


struct newsgroup {
    char *name;        /* name of the group                                 */
    ng_num newsrc;     /* index of this group into Newsrc                   */
    unsigned char status; /* NG_* */
    art_num first;     /* first available article number                    */
    art_num last;      /* last article number                               */
    art_num current;   /* current article number                            */
    struct list *nglist;  /* newsgroup entry for unsubscribed groups        */
    unsigned char from_cache; /* is this entry from the active file cache?  */
    unsigned char fetch; /* what should we fetch?			    */
    hash_table_object thread_table;
    void *kill_file;
#ifdef ARTSTRUCT_C
    /* These should only be touched in artstruct.c! */
    struct article *articles, *ref_art;
#else
    struct article *dont_touch1, *dont_touch2;
#endif
};


#define ART_READ		(1<<0)
#define ART_PRINTED		(1<<1)
#define ART_FETCHED		(1<<2)
#define ART_UNAVAIL		(1<<3)
#define ART_ALL_HEADERS		(1<<4)
#define ART_ROTATED		(1<<5)
#define ART_SAVED		(1<<6)
#define ART_MARKED		(1<<7)
#define ART_XLATED		(1<<8)
#define ART_KILLED		(1<<9)
#define ART_LISTED		(1<<10)
#define ART_MAYBE_LISTED	(1<<11)

#define ART_CLEAR       (0)
#define ART_CLEAR_READ  (ART_READ | ART_KILLED)

/* helper macros */
#define IS_READ(art)       	(   (art)->status & ART_READ)
#define IS_UNREAD(art)     	(! ((art)->status & ART_READ))
#define IS_PRINTED(art)    	(   (art)->status & ART_PRINTED)
#define IS_UNPRINTED(art)  	(! ((art)->status & ART_PRINTED))
#define IS_FETCHED(art)    	(   (art)->status & ART_FETCHED)
#define IS_UNFETCHED(art)  	(! ((art)->status & ART_FETCHED))
#define IS_AVAIL(art)    	(! ((art)->status & ART_UNAVAIL))
#define IS_UNAVAIL(art)    	(   (art)->status & ART_UNAVAIL)
#define IS_ALL_HEADERS(art)	(   (art)->status & ART_ALL_HEADERS)
#define IS_ROTATED(art)		(   (art)->status & ART_ROTATED)
#define IS_SAVED(art)		(   (art)->status & ART_SAVED)
#define IS_MARKED(art)		(   (art)->status & ART_MARKED)
#define IS_UNMARKED(art)	(! ((art)->status & ART_MARKED))
#define IS_XLATED(art)		(   (art)->status & ART_XLATED)
#define IS_KILLED(art)		(   (art)->status & ART_KILLED)
#define IS_LISTED(art)		(   (art)->status & ART_LISTED)
#define IS_MAYBE_LISTED(art)	(   (art)->status & ART_MAYBE_LISTED)
  
#define SET_READ(art)			((art)->status |= ART_READ)
#define SET_UNREAD(art)			((art)->status &= ~ART_READ)
#define SET_PRINTED(art)		((art)->status |= ART_PRINTED)
#define SET_UNPRINTED(art)		((art)->status &= ~ART_PRINTED)
#define SET_FETCHED(art)		((art)->status |= ART_FETCHED)
#define SET_UNFETCHED(art)		((art)->status &= ~ART_FETCHED)
#define SET_ALL_HEADERS(art)		((art)->status |= ART_ALL_HEADERS)
#define SET_STRIPPED_HEADERS(art)	((art)->status &= ~ART_ALL_HEADERS)
#define SET_ROTATED(art)		((art)->status |= ART_ROTATED)
#define SET_UNROTATED(art)		((art)->status &= ~ART_ROTATED)
#define SET_AVAIL(art)			((art)->status &= ~ART_UNAVAIL)
#define SET_UNAVAIL(art)		((art)->status |= ART_UNAVAIL)
#define SET_SAVED(art)			((art)->status |= ART_SAVED)
#define SET_UNSAVED(art)		((art)->status &= ~ART_SAVED)
#define SET_MARKED(art)			((art)->status |= ART_MARKED)
#define SET_UNMARKED(art)		((art)->status &= ~ART_MARKED)
#define SET_UNXLATED(art)		((art)->status &= ~ART_XLATED)
#define SET_XLATED(art)			((art)->status |= ART_XLATED)
#define SET_KILLED(art)			((art)->status |= ART_KILLED)
#define SET_LISTED(art)			((art)->status |= ART_LISTED)
#define SET_MAYBE_LISTED(art)		((art)->status |= ART_MAYBE_LISTED)
#define SET_UNLISTED(art)		((art)->status &= ~(ART_LISTED|ART_MAYBE_LISTED))

#define _CLEAR_ALL(art,free) \
  _CLEAR_FILE((art),(free)); \
  _CLEAR_SUBJECT((art),(free)); \
  _CLEAR_FROM((art),(free)); \
  _CLEAR_AUTHOR((art),(free)); \
  _CLEAR_LINES((art),(free)); \
  _CLEAR_NEWSGROUPS((art),(free)); \
  _CLEAR_DATE((art),(free)); \
  _CLEAR_ID((art),(free)); \
  _CLEAR_REFS((art),(free)); \
  _CLEAR_XREF((art),(free)); \
  _CLEAR_PARENT((art),(free)); \
  _CLEAR_CHILDREN((art),(free)); \

#define _CLEAR_FILE(art,free) \
  if ((free) && (art)->filename) { \
    (void) unlink((art)->filename); \
    FREE((art)->filename); \
  } \
  SET_UNFETCHED(art); \
  (art)->filename = 0;

#define _CLEAR_SUBJECT(art,free) \
  if ((free) && (art)->subject) { \
    FREE((art)->subject); \
  } \
  (art)->subject = 0;

#define _CLEAR_FROM(art,free) \
  if ((free) && (art)->from) { \
    FREE((art)->from); \
  } \
  (art)->from = 0;

#define _CLEAR_AUTHOR(art,free) \
  if ((free) && (art)->author) { \
    FREE((art)->author); \
  } \
  (art)->author = 0;

#define _CLEAR_LINES(art,free) \
  if ((free) && (art)->lines) { \
    FREE((art)->lines); \
  } \
  (art)->lines = 0;

#define _CLEAR_NEWSGROUPS(art,free) \
  if ((free) && (art)->newsgroups) { \
    FREE((art)->newsgroups); \
  } \
  (art)->newsgroups = 0;

#define _CLEAR_DATE(art,free) \
  if ((free) && (art)->date) { \
    FREE((art)->date); \
  } \
  (art)->date = 0;

#define _CLEAR_ID(art,free) \
  if ((free) && (art)->id) { \
    FREE((art)->id); \
  } \
  (art)->id = 0;

#define _CLEAR_REFS(art,free) \
  if ((free) && (art)->references) { \
    FREE((art)->references); \
  } \
  (art)->references = 0;

#define _CLEAR_XREF(art,free) \
  if ((free) && (art)->xref) { \
    FREE((art)->xref); \
  } \
  (art)->xref = 0;

#define _CLEAR_PARENT(art,free) \
  (art)->parent = 0;

#define _CLEAR_CHILDREN(art,free) \
  if ((free) && (art)->children) { \
    FREE((art)->children); \
  } \
  (art)->children = 0;

#define CLEAR_ALL(art)		_CLEAR_ALL((art),1)
#define CLEAR_ALL_NO_FREE(art)	_CLEAR_ALL((art),0)

#define CLEAR_FILE(art)		_CLEAR_FILE((art),1)
#define CLEAR_SUBJECT(art)	_CLEAR_SUBJECT((art),1)
#define CLEAR_FROM(art)		_CLEAR_FROM((art),1)
#define CLEAR_AUTHOR(art)	_CLEAR_AUTHOR((art),1)
#define CLEAR_LINES(art)	_CLEAR_LINES((art),1)
#define CLEAR_NEWSGROUPS(art)	_CLEAR_NEWSGROUPS((art),1)
#define CLEAR_DATE(art)		_CLEAR_DATE((art),1)
#define CLEAR_ID(art)		_CLEAR_ID((art),1)
#define CLEAR_REFS(art)		_CLEAR_REFS((art),1)
#define CLEAR_XREF(art)		_CLEAR_XREF((art),1)
#define CLEAR_PARENT(art)	_CLEAR_PARENT((art),1)
#define CLEAR_CHILDREN(art)	_CLEAR_CHILDREN((art),1)

#define NG_SUB		(1<<0)	/* newsgroup is subscribed/unsubscribed to */
#define NG_NOENTRY	(1<<1)	/* no entry in the .newsrc for this group */
#define NG_POSTABLE	(1<<2)	/* newsgroup can be posted to */
#define NG_MODERATED	(1<<3)	/* newsgroup is moderated */
#define NG_NEW		(1<<4)	/* newsgroup is new */

#define IS_SUBSCRIBED(ng)	((ng)->status & NG_SUB)
#define IS_NOENTRY(ng)		((ng)->status & NG_NOENTRY)
#define IS_POSTABLE(ng)	  	((ng)->status & NG_POSTABLE)
#define IS_MODERATED(ng)  	((ng)->status & NG_MODERATED)
#define IS_NEW(ng)		((ng)->status & NG_NEW)

#define SET_SUB(ng)    		((ng)->status |= NG_SUB)
#define SET_UNSUB(ng)  		((ng)->status &= ~NG_SUB)
#define SET_POSTABLE(ng)	((ng)->status |= NG_POSTABLE)
#define SET_UNPOSTABLE(ng)	((ng)->status &= ~NG_POSTABLE)
#define SET_MODERATED(ng)	((ng)->status |= NG_MODERATED)
#define SET_UNMODERATED(ng)	((ng)->status &= ~NG_MODERATED)
#define SET_NOENTRY(ng)		((ng)->status |= NG_NOENTRY)
#define SET_NEW(ng)		((ng)->status |= NG_NEW)

#define CLEAR_NOENTRY(ng)	((ng)->status &= ~NG_NOENTRY)
#define CLEAR_NEW(ng)		((ng)->status &= ~NG_NEW)

#define EMPTY_GROUP(ng) ((ng)->last < (ng)->first)
  
extern struct newsgroup *CurrentGroup;   /* current index into the newsrc array  */
extern ng_num MaxGroupNumber;       /* size of the newsrc array                  */

/*
  If this assertion fails, it means that you've got too many groups to
  fit in an integer of ng_num's size, and you need to change the
  typedef of ng_num above so that it is an integer with more bytes.
  */
#define INC_MAXGROUPNUMBER() { assert(MaxGroupNumber < NOT_IN_NEWSRC-1); \
			       MaxGroupNumber++; }

extern struct newsgroup **Newsrc;          /* sequence list for .newsrc file            */

/*
  NOTE WELL: There are places in the code that depend on the fact that
  when a variable of type ng_num with value 0 is decremented, it
  acquires a value of NOT_IN_NEWSRC.  I don't know of any systems on
  which XRN works for which an unsigned integer type doesn't wrap
  around like this, but if there is one, then the code will fail.
  */
#define NOT_IN_NEWSRC ((ng_num)-1)

/* not a valid group (must be less than 0) */
#define NO_GROUP -1

#endif /* NEWS_H */
