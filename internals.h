#ifndef INTERNALS_H
#define INTERNALS_H

/*
 * $Header: /d/src/cvsroot/xrn/internals.h,v 1.7 1994-11-17 18:51:21 jik Exp $
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
 * internals.h: news system interface
 */

#include "server.h"

#define XRN_ERROR 0
#define XRN_NOMORE 0
#define XRN_OKAY   1
#define XRN_NOUNREAD 2
#define XRN_UNSUBBED 3

#define ENTER_SETUP (1<<0)
#define ENTER_UNREAD (1<<1)
#define ENTER_UNSUBBED (1<<2)
#define ENTER_JUMPING (1<<3)
#define ENTER_REGEXP (1<<4)

/* 
 * kill file stuff
 */
#define KILL_GLOBAL 0
#define KILL_LOCAL 1
extern void killItem _ARGUMENTS((char *,int));
extern char *localKillFile _ARGUMENTS((struct newsgroup*, int));

/*
 * routines for adding newsgroups to the newsrc file
 */
 
/* 'status' values for the 'add' functions */
#define SUBSCRIBE 1
#define UNSUBSCRIBE 0

extern int addToNewsrcAfterGroup _ARGUMENTS((char *,char *,int));
extern int addToNewsrcEnd _ARGUMENTS((char *,int));
extern int addToNewsrcBeginning _ARGUMENTS((char *,int));


/*
 * routines for doing newsgroup management
 */

/* jump/goto/add newsgroup codes */
#define BAD_GROUP -1
#define GOOD_GROUP 1

extern int enterNewsgroup _ARGUMENTS((char *name, int flags));

/* subscribe to the current newsgroup */
extern void subscribe _ARGUMENTS(());
/* unsubscribe to the current newsgroup */
extern void unsubscribe _ARGUMENTS(());

/* check subscription status */
extern int issubscribed _ARGUMENTS(());

/* updates the .newsrc file so that the current newsgroup is marked as all read */
extern void catchUp _ARGUMENTS(());


/*
 * routines for doing article management
 */

extern void gotoArticle _ARGUMENTS((long));
extern int checkArticle _ARGUMENTS((art_num));

#ifndef VMS
extern int getArticle _ARGUMENTS((char **,char **));
#else
extern int getArticleFile _ARGUMENTS((char **,char **));
#endif
extern int toggleHeaders _ARGUMENTS((char **,char **));
extern int toggleRotation _ARGUMENTS((char **,char **));
#ifdef XLATE
extern int toggleXlation _ARGUMENTS((char **, char **));
#endif

#ifdef XRN_PREFETCH
extern void prefetchNextArticle _ARGUMENTS(());
#endif /* XRN_PREFETCH */

#ifdef STUPIDMMU
extern void cornered _ARGUMENTS((struct newsgroup *));
#endif

/* mark articles */
extern void markArticleAsRead _ARGUMENTS((long));
extern void markArticleAsUnread _ARGUMENTS((long));

/* get a single subject line, stripped of leading/trailing spaces, Re: */
extern char *getSubject _ARGUMENTS((long));
extern char *getAuthor _ARGUMENTS((long));

/* get the subject line for the previous subject (and get the article too) */
/* only called when going off the top of the subject string */
extern char *getPrevSubject _ARGUMENTS(());

extern void startSearch _ARGUMENTS(());
extern void failedSearch _ARGUMENTS(());

/*
 * information gathering routines
 */

extern void checkLock _ARGUMENTS(());
extern void removeLock _ARGUMENTS(());
  
/* read the .newsrc file, find out what to read */
extern void initializeNews _ARGUMENTS(());

/* query the server for new information */
extern void rescanServer _ARGUMENTS(());

/* return the new newsgroups string */
extern char *newGroups _ARGUMENTS(());

/* return a count of unread articles in all newsgroups */
extern int unreadNews _ARGUMENTS((void));

/* return the unread newsgroups string */
extern char *unreadGroups _ARGUMENTS((int));

/* return the subject string */
#define ALL 0
#define UNREAD 1
extern char *getSubjects _ARGUMENTS((int));

/* build and return the status string */
extern char *getStatusString _ARGUMENTS(());

extern void bogusNewsgroup _ARGUMENTS(());

extern void releaseNewsgroupResources _ARGUMENTS((struct newsgroup *));


/* handle first and/or last article number changes
 *
 * NOTE: if first > last, that means that there are no articles available for
 * the group and no articles array will exist
 */

extern void articleArrayResync _ARGUMENTS((struct newsgroup *, art_num,
					   art_num, int));
extern void updateArticleArray _ARGUMENTS((struct newsgroup *));

#ifdef XRN_PREFETCH
extern void cancelPrefetch _ARGUMENTS((void));
extern void finishPrefetch _ARGUMENTS((void));
#endif /* XRN_PREFETCH */

extern void fillUpArray(/* art_num art */);

#define GETARTICLES(newsgroup) (newsgroup->articles) ? (newsgroup->articles) : getarticles(newsgroup)

extern char *getinfofromfile _ARGUMENTS((char *));

extern char **parseRegexpList _ARGUMENTS((char *list, char *list_name));

#endif /* INTERNALS_H */
