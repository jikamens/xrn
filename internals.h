#ifndef INTERNALS_H
#define INTERNALS_H

/*
 * $Id: internals.h,v 1.49 1998-07-07 12:32:41 jik Exp $
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
#include "config.h"
#include "file_cache.h"

#define XRN_ERROR 0
#define XRN_NOMORE 0
#define XRN_OKAY   1
#define XRN_NOUNREAD 2
#define XRN_UNSUBBED 3

#define ENTER_SETUP (1<<0)
#define ENTER_UNREAD (1<<1)
#define ENTER_UNSUBBED (1<<2)
#define ENTER_SUBSCRIBE (1<<3)
#define ENTER_REGEXP (1<<4)

/* 
 * kill file stuff
 */
extern char *stringToRegexp _ARGUMENTS((char *, int max_length));
extern char *localKillFile _ARGUMENTS((struct newsgroup*, int));
extern char *globalKillFile _ARGUMENTS((void));

/*
 * routines for adding newsgroups to the newsrc file
 */
 
/* 'status' values for the 'add' functions */
#define IGNORE 2
#define SUBSCRIBE 1
#define UNSUBSCRIBE 0

extern int addToNewsrcAfterGroup _ARGUMENTS((char *,char *,int));
extern int addToNewsrcEnd _ARGUMENTS((char *,int));
extern int addToNewsrcBeginning _ARGUMENTS((char *,int));
extern int removeFromNewsrc _ARGUMENTS((char *));
extern int ignoreGroup _ARGUMENTS((char *));


/*
 * routines for doing newsgroup management
 */

/* jump/goto/add newsgroup codes */
#define BAD_GROUP -1
#define GOOD_GROUP 1

extern int enterNewsgroup _ARGUMENTS((char *name, int flags));
extern void exitNewsgroup _ARGUMENTS((void));

/* subscribe to the current newsgroup */
extern void subscribe _ARGUMENTS((void));
/* unsubscribe to the current newsgroup */
extern void unsubscribe _ARGUMENTS((void));

/* updates the .newsrc file so that the current newsgroup is marked as all read */
extern void catchUp _ARGUMENTS((void));


/*
 * routines for doing article management
 */

extern int checkArticle _ARGUMENTS((art_num));

extern int getArticle _ARGUMENTS((struct newsgroup *, art_num,
				  file_cache_file **, char **));
extern int toggleHeaders _ARGUMENTS((file_cache_file **,char **));
extern int toggleRotation _ARGUMENTS((file_cache_file **,char **));
#ifdef XLATE
extern int toggleXlation _ARGUMENTS((file_cache_file **, char **));
#endif

extern void prefetchNextArticle _ARGUMENTS((void));

/* mark articles */
extern void markArticleAsRead _ARGUMENTS((art_num));
extern void markArticleAsUnread _ARGUMENTS((art_num));

/* get a single subject line, stripped of leading/trailing spaces, Re: */
extern char *getSubject _ARGUMENTS((art_num));
extern char *getAuthor _ARGUMENTS((art_num));

/* get the subject line for the previous subject (and get the article too) */
/* only called when going off the top of the subject string */
extern char *getPrevSubject _ARGUMENTS((int));
extern art_num getPrevNumber _ARGUMENTS((void));

extern void startSearch _ARGUMENTS((void));
extern void failedSearch _ARGUMENTS((void));

/*
 * information gathering routines
 */

extern void checkLock _ARGUMENTS((void));
extern void removeLock _ARGUMENTS((void));
  
/* read the .newsrc file, find out what to read */
extern void initializeNews _ARGUMENTS((void));

/* query the server for new information */
extern void rescanServer _ARGUMENTS((Boolean));
extern void rescanBackground _ARGUMENTS((void));
extern void cancelRescanBackground _ARGUMENTS((void));

/* clear the "NEW" status of a group, by name */
extern void clearNew _ARGUMENTS((char *));

/* return the new newsgroups string */
extern char *newGroups _ARGUMENTS((void));

/* check if an article is available */
extern Boolean articleIsAvailable _ARGUMENTS((struct newsgroup *, art_num));

/* return a count of unread articles in all newsgroups */
extern int unreadNews _ARGUMENTS((void));

/* return the unread newsgroups string */
extern char *unreadGroups _ARGUMENTS((int, int));

/* find the index of a group near a named group in the newsgroup list */
extern int getNearbyNewsgroup _ARGUMENTS((char *, char **));

/* return the subject string */
#define ALL 0
#define UNREAD 1
extern char *getSubjects _ARGUMENTS((int, art_num, Boolean));

/* build and return the status string */
extern char *getStatusString _ARGUMENTS((int, int, char *));

extern void releaseNewsgroupResources _ARGUMENTS((struct newsgroup *));


/* handle first and/or last article number changes
 *
 * NOTE: if first > last, that means that there are no articles available for
 * the group and no articles array will exist
 */

extern void articleArrayResync _ARGUMENTS((struct newsgroup *, art_num,
					   art_num, int));
extern Boolean updateArticleArray _ARGUMENTS((struct newsgroup *, Boolean));

extern void cancelPrefetch _ARGUMENTS((void));
extern void resetPrefetch _ARGUMENTS((void));
extern void finishPrefetch _ARGUMENTS((void));
extern void prefetchGroup _ARGUMENTS((char *));

extern int fillUpArray _ARGUMENTS((struct newsgroup *, art_num art, art_num last,
				   Boolean check_abort,
				   Boolean kill_files));

extern char *getinfofromfile _ARGUMENTS((char *));

#ifdef POSIX_REGEX
extern regex_t *parseRegexpList _ARGUMENTS((char *list, char *list_name,
					    int *count));
#else
extern char **parseRegexpList _ARGUMENTS((char *list, char *list_name,
					  int *count));
#endif

extern struct var_rec *cache_variables;
extern char *cache_file;

int subjectIndexLine _ARGUMENTS((int, char *, struct newsgroup *, art_num,
				 Boolean));

char *subjectStrip _ARGUMENTS((char *));

#define PREFETCH_FIRST_STAGE		PREFETCH_GETGROUP_STAGE
#define PREFETCH_GETGROUP_STAGE		1
#define PREFETCH_SETCURRENT_STAGE	2
#define PREFETCH_START_HEADERS_STAGE	PREFETCH_SUBJECT_STAGE
#define PREFETCH_SUBJECT_STAGE		3
#define PREFETCH_AUTHOR_STAGE		4
#define PREFETCH_LINES_STAGE		5
#define PREFETCH_START_OPTIONAL_STAGE	PREFETCH_NEWSGROUPS_STAGE
#define PREFETCH_NEWSGROUPS_STAGE	6
#define PREFETCH_DATE_STAGE		7
#define PREFETCH_IDS_STAGE		8
#define PREFETCH_REFS_STAGE		9
#define PREFETCH_XREF_STAGE		10
#define PREFETCH_LAST_HEADERS_STAGE	PREFETCH_XREF_STAGE
#define PREFETCH_KILL_STAGE		11
#define PREFETCH_THREAD_STAGE		12
#define PREFETCH_LAST_OPTIONAL_STAGE	PREFETCH_THREAD_STAGE
#define PREFETCH_LAST_STAGE		PREFETCH_THREAD_STAGE

/* CAUTION!!!

   Maks sure that PREFETCH_LAST_OPTIONAL_STAGE -
   PREFETCH_START_OPTIONAL_STAGE < 8, because the bits are all assumed
   to fit into a single byte.  If this needs to change, we'll need to
   grow the size of the flag field from char to int (or long).  It's a
   char right now to save on memory usage.
   */

#define PREFETCH_FIELD_BIT(stage) (1<<(stage-PREFETCH_START_OPTIONAL_STAGE))

#define FETCH_NEWSGROUPS	PREFETCH_FIELD_BIT(PREFETCH_NEWSGROUPS_STAGE)
#define FETCH_DATES		PREFETCH_FIELD_BIT(PREFETCH_DATE_STAGE)
#define FETCH_IDS		PREFETCH_FIELD_BIT(PREFETCH_IDS_STAGE)
#define FETCH_REFS		PREFETCH_FIELD_BIT(PREFETCH_REFS_STAGE)
#define FETCH_XREF		PREFETCH_FIELD_BIT(PREFETCH_XREF_STAGE)
#define FETCH_THREADS		PREFETCH_FIELD_BIT(PREFETCH_THREAD_STAGE)

/* Given an article's message ID, get its number in a newsgroup.
   Returns the article number on success, 0 on failure, or -1 on abort.
   */
extern art_num getArticleNumberFromIdXref _ARGUMENTS((struct newsgroup *, char *));

/* Get the first valid message ID in a list of message IDs (i.e., a
   references line).  The returned memory is static, should not be
   tampered with, and is only valid until the next call to the
   function. */
char *getFirstReference _ARGUMENTS((char *));

#endif /* INTERNALS_H */
