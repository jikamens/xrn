
#if !defined(lint) && !defined(SABER)
static char XRNrcsid[] = "$Header: /d/src/cvsroot/xrn/internals.c,v 1.13 1994-10-11 14:30:54 jik Exp $";
#endif

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
 * internals.c: routines for handling the internal data structures
 *    calls server routines, is calls by the user interface code
 *
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <errno.h>
#include <assert.h>
#ifndef VMS
#include <sys/param.h>
#else
#ifndef R_OK
#define F_OK            0       /* does file exist */
#define X_OK            1       /* is it executable by caller */
#define W_OK            2       /* writable by caller */
#define R_OK            4       /* readable by caller */
#endif /* R_OK */
#define getArticle getArticleFile
#define index strchr
#include <ctype.h>
#endif /* VMS */
#ifdef aiws
#include <unistd.h>
#endif /* aiws */
#include "avl.h"
#include "news.h"
#include "newsrcfile.h"
#include "resources.h"
#include "server.h"
#include "mesg.h"
#include "error_hnds.h"
#include "save.h"
#include "xrn.h"
#include "buttons.h"
#include "internals.h"
#include "mesg_strings.h"

#ifndef R_OK
#define R_OK 4
#endif

#define SETARTICLES(newsgroup) if (newsgroup->articles == 0) (void) getarticles(newsgroup)

static char *strip _ARGUMENTS((char *, /* Boolean */ int));

#define BUFFER_SIZE 1024
#define LABEL_SIZE 128
#define FATAL 0
#define OKAY  1
/*
 * length of lines in the top text window
 */
#define LINE_LENGTH 200

#ifdef XRN_PREFETCH
/* don't prefetch the next group if you got to a group via 'goto newsgroup' */
#define OKAY_TO_XRN_PREFETCH_GROUP 1
#define DO_NOT_XRN_PREFETCH_GROUP  0

/*
 * this flag is set to DO_NOT_XRN_PREFETCH_GROUP if you got to the current
 * group by 'goto newsgroup' (jumpToNewsgroup).  You do not want to prefetch
 * the next group if you are randomly jumping around.  The next group
 * after the one jumped to is probably not the next one you will go to.
 */
static int PrefetchGroupMode = OKAY_TO_XRN_PREFETCH_GROUP;

/* set to the value of a group if that group has been prefetched */
static struct newsgroup *PrefetchedGroup = 0;
static struct newsgroup *PrefetchingGroup = 0;
static XtWorkProcId PrefetchWorkId;
static Boolean PrefetchInProgress = False;

static void prefetchNextGroup _ARGUMENTS((XtPointer));

#endif /* XRN_PREFETCH */

/*
 * next article to get when faulting in articles that are of the top
 * of the subject screen.
 */
static art_num NextPreviousArticle;
  
/* storage for all newsgroups, key is the name of the group */
avl_tree *NewsGroupTable = NIL(avl_tree);

/* number of groups in the active file */
int ActiveGroupsCount = 0;

/* number of the group currently being processed */
struct newsgroup * CurrentGroup = 0;

/* number of groups in the .newsrc file */
ng_num MaxGroupNumber = 0;

/*
 * see if another xrn is running
 */
void checkLock()
{
#ifndef VMS
    char *buffer = utTildeExpand(app_resources.lockFile);
#else
    char *buffer = "SYS$LOGIN:XRN.LOCK";
    char *ptr;
#endif /* VMS */
    char host[64];
    char myhost[64];
    int pid;
    FILE *fp;
    extern int errno;

    if (!buffer) {
	/* silently ignore this condition */
	return;
    }

#ifndef VMS
    if (gethostname(myhost, sizeof(myhost)) == -1) {
	(void) strcpy(myhost, "bogusHost");
    }
#else
    ptr = getenv("SYS$NODE");
    (void) strcpy(myhost, ptr);
    ptr = index(myhost, ':');
    if (ptr != NIL(char)) {
	*ptr = '\0';
    }

    if (*myhost == NIL(char)) {
	(void) strcpy(myhost, "bogusHost");
    }
#endif /* VMS */

    if ((fp = fopen(buffer, "r")) == NULL) {
	if ((fp = fopen(buffer, "w")) == NULL) {
	    /* silently ignore this condition */
	    return;
	}
	(void) fprintf(fp, "%s %d\n", myhost, getpid());
	(void) fclose(fp);
	return;
    }

    (void) fscanf(fp, "%s %d", host, &pid);

    /* see if I'm on the same host */
    if (strcmp(host, myhost) == 0) {
	/* whoa!  I am, see if the process is running */
	if ((kill(pid, 0) == -1) && (errno == ESRCH)) {
	    /* hey, it's not running.... */
	    /* why do it right when you can just do this... */
	    removeLock();
	    checkLock();
	    return;
	}
    }
    (void) fprintf(stderr, "An XRN of yours is running on %s as process %d.\n",
		   host, pid);
    (void) fprintf(stderr,
		   "If it is no longer running, remove the file \"%s\".\n",
		   buffer);
    exit(-1);

    return;
}


void removeLock()
{
#ifndef VMS
    char *buffer = utTildeExpand(app_resources.lockFile);
#else
    char *buffer = "SYS$LOGIN:XRN.LOCK";
#endif /* VMS */

    if (buffer) {
	(void) unlink(buffer);
    }
    return;
}


/*
 * initialize the program and the news system
 *   read newsrc, etc
 *
 *   returns: void
 *
 */
void initializeNews()
{
    start_server(app_resources.nntpServer);

    do {
	 NewsGroupTable = avl_init_table(strcmp);
	 if (! NewsGroupTable) {
	     ehErrorExitXRN("out of memory");
	 }

	 getactive();
	 
	 if (readnewsrc(app_resources.newsrcFile,
			app_resources.saveNewsrcFile))
	      break;
	 
	 ehErrorRetryXRN("Can not read the .newsrc file", True);

	 avl_free_table(NewsGroupTable, XtFree, (void (*)(void *))XtFree);
    } while (1);
	
#ifndef FIXED_ACTIVE_FILE
    badActiveFileCheck();
#endif
    return;
}


/*
 * get the active file again and grow the Newsrc array if necessary
 */
void rescanServer()
{
    int old = ActiveGroupsCount;

    /* update the newsrc file */
    while (!updatenewsrc())
	ehErrorRetryXRN("Can not update the newsrc file", True);

#if !defined(NNTP_REREADS_ACTIVE_FILE)
    stop_server();
    start_server(NIL(char));
#endif
    
    getactive();

    if (ActiveGroupsCount > old) {
	/* new newsgroups were found, allocate a bigger array */
	Newsrc = (struct newsgroup **) realloc((char *) Newsrc, (unsigned) (sizeof(struct newsgroup *) * ActiveGroupsCount));
    }

#ifndef FIXED_ACTIVE_FILE
    badActiveFileCheck();
#endif

    return;
}


#define lsDestroy(lst) \
    if (lst != NIL(struct list)) { \
	struct list *_next; \
	do { \
	    _next = lst->next; \
	    FREE(lst); \
	    lst = _next; \
	} while (lst != NIL(struct list)); \
    }

/*
 * accurately count the number of unread articles in a group
 *
 *   returns: the number of unread articles in the group
 */
static int unreadArticleCount _ARGUMENTS((struct newsgroup *));

static int unreadArticleCount(newsgroup)
    struct newsgroup *newsgroup;
{
#ifdef STUPIDMMU
    struct article *articles;
    struct list *item;
#else
    struct article *articles = GETARTICLES(newsgroup);
#endif
    int count = 0;

    if (EMPTY_GROUP(newsgroup)) {
	return 0;
    }
    
    if ((newsgroup->first == 0) && (newsgroup->last == 0)) {
	return 0;
    }
    
#ifdef STUPIDMMU
    /* try to be clever and not use articles array */
    if (!newsgroup->articles) {
	int cleared = 0;

	if (!newsgroup->nglist)
	  return 0;

	/* process the .newsrc line */

	for (item = newsgroup->nglist; item != NIL(struct list);
		     item = item->next) {

	    /* this is really stupid, we need this because
	       rescans cause a newsgroup to forget its previous idea of
	       first */
	    int start;

	    switch (item->type) {
		case SINGLE:
		if (item->contents.single > newsgroup->last) {
		    /* something really bad has happened */
		    mesgPane(XRN_SERIOUS, ART_NUMBERING_PROBLEM_MSG,
			     newsgroup->name);
		    lsDestroy(newsgroup->nglist);
		    newsgroup->nglist = NIL(struct list);
		    return 0;
		}
		if (item->contents.single >= newsgroup->first) {
		    cleared++;
		}
		break;

		case RANGE:
		if ((item->contents.range.start > newsgroup->last) ||
		    (item->contents.range.end > newsgroup->last)) {
		    /* something really bad has happened */
		    mesgPane(XRN_SERIOUS, ART_NUMBERING_PROBLEM_MSG,
			     newsgroup->name);
		    lsDestroy(newsgroup->nglist);
		    newsgroup->nglist = NIL(struct list);
		    return 0;
		}
		if (item->contents.range.start < newsgroup->first) {
		    start = newsgroup->first;
		} else {
		    start = item->contents.range.start;
		}
		
		if (item->contents.range.end < newsgroup->first) {
		    break;
		}
		cleared += item->contents.range.end - start + 1;
	    }
	}
	return(newsgroup->last - newsgroup->first + 1 - cleared);
    }
#endif

#ifdef STUPIDMMU
    articles = newsgroup->articles;
#endif

    {
	register struct article	*ap;
	register art_num	last	= newsgroup->last;
	register art_num	i;

	ap = &articles[INDEX(newsgroup->first)];

	for (i = newsgroup->first; i <= last; i++) {
	    if (IS_UNREAD(*ap) && !IS_UNAVAIL(*ap)) {
		count++;
	    }
	    ap++;
	}
    }

    return count;
}


/*
 * accurately count the number of articles in a group
 *
 *   returns: the total number of articles in the group
 */
static int totalArticleCount _ARGUMENTS((struct newsgroup *));

static int totalArticleCount(newsgroup)
    struct newsgroup *newsgroup;
{
    struct article *articles = GETARTICLES(newsgroup);
    int count = 0;
  
    if (EMPTY_GROUP(newsgroup)) {
	return 0;
    }
    
    if ((newsgroup->first == 0) && (newsgroup->last == 0)) {
	return 0;
    }
    
    {
	register struct article	*ap;
	register art_num	last	= newsgroup->last;
	register art_num	i;

	ap = &articles[INDEX(newsgroup->first)];

	for (i = newsgroup->first; i <= last; i++) {
	    if (!IS_UNAVAIL(*ap)) {
		count++;
	    }
	    ap++;
	}
    }

    return count;
}


/*
 * find the first unread article in a group and set 'current' to it
 *
 * returns: void
 *
 */
static void setCurrentArticle _ARGUMENTS((struct newsgroup *));

static void setCurrentArticle(newsgroup)
    struct newsgroup *newsgroup;
{
    struct article *articles = GETARTICLES(newsgroup);
    art_num i;
    art_num start;
    
    newsgroup->current = newsgroup->last + 1;

    /* if the resource 'onlyShow' is > 0, then mark all but the last
     * 'onlyShow' articles as read */

    start = newsgroup->first;
    if (app_resources.onlyShow > 0) {
	start = MAX(start, newsgroup->last - app_resources.onlyShow);
    }
    
    for (i = start; i<= newsgroup->last; i++) {
	long indx = INDEX(i);
	if (IS_UNREAD(articles[indx]) && !IS_UNAVAIL(articles[indx])) {
	    newsgroup->current = i;
	    return;
	}
    }
    return;
}

/*
 * convert an array of strings (of 'count' items) into one big string
 * with each substring seperated by newlines
 *
 * bytes is the sum of the string lengths of the strings
 *
 * this routine takes care of allocating the big string
 *
 */
static char * arrayToString _ARGUMENTS((char **, int, int));

static char * arrayToString(array, count, bytes)
    char **array;
    int count;
    int bytes;
{
    char *bigString, *end;
    int i;

    /* bytes for the strings, count for the newlines, 1 for the zero byte */
    bigString = ARRAYALLOC(char, (bytes + count + 1));
    bigString[0] = '\0';

    end = bigString;
    for (i = 0; i < count; i++) {
	(void) strcpy(end, array[i]);
	end += utStrlen(array[i]);
	*end = '\n';
	end++;
    }

    *end = '\0';

    return bigString;
}


int unreadNews()
{
    int i, count = 0;
    struct newsgroup *newsgroup;

    for (i = 0; i < MaxGroupNumber; i++) {
	newsgroup = Newsrc[i];
#ifdef WATCH
	if (IS_SUBSCRIBED(newsgroup) && watchingGroup(newsgroup->name))
#else
	if (IS_SUBSCRIBED(newsgroup))
#endif
        {
	    count += unreadArticleCount(newsgroup);
	}
    }
    return count;
}


/*
 * build and return a string of information about groups that need to be read
 */
char * unreadGroups(mode)
    int mode;	/* 0 for unread groups, 1 for all subscribed to groups */
{
    char dummy[LINE_LENGTH];
    struct newsgroup *newsgroup;
    ng_num i;
    int unread, j;
    int bytes = 0, subscribedGroups = 0;
    char *string, **ar;

    ar = ARRAYALLOC(char *, MaxGroupNumber);

    for (i = 0; i < MaxGroupNumber; i++) {
	newsgroup = Newsrc[i];

	if (IS_SUBSCRIBED(newsgroup) &&
	   (((unread = unreadArticleCount(newsgroup)) > 0) || mode)) {
	    int total = totalArticleCount(newsgroup);
	    (void) sprintf(dummy, "%6s %7s %-40s%5d article%s +%5d old",
	                   (unread > 0 ? "Unread" : ""),
	                   (total > 0 ? "news in" : ""),
			   newsgroup->name, unread,
			   ((unread != 1) ? "s" : " "),
			   total-unread);

	    ar[subscribedGroups++] = XtNewString(dummy);
	    bytes += strlen(dummy);
	}
    }

    string = arrayToString(ar, subscribedGroups, bytes);
    for (j = 0; j < subscribedGroups; j++) {
	FREE(ar[j]);
    }
    FREE(ar);
    return string;
}


/*
 * determine the newsgroups that are not in the .newsrc file
 *
 *   returns a string representing the information
 */
char * newGroups()
{
    int count = 0, bytes = 0;
    avl_generator *gen;
    char *key, *value;
    char **ar;
    char *string;

    ar = ARRAYALLOC(char *, ActiveGroupsCount);

    gen = avl_init_gen(NewsGroupTable, AVL_FORWARD);
    if (! gen) {
	ehErrorExitXRN("out of memory");
    }

    while (avl_gen(gen, &key, &value)) {
	struct newsgroup *newsgroup = (struct newsgroup *) value;

	if (IS_NOENTRY(newsgroup)) {
	    ar[count++] = newsgroup->name;
	    bytes += utStrlen(newsgroup->name);
	}
    }

    avl_free_gen(gen);

    /* no new groups return */
    if (count == 0) {
	FREE(ar);	
	return NIL(char);
    }

    string = arrayToString(ar, count, bytes);
    FREE(ar);
    
    return string;
}

#ifdef XRN_PREFETCH

void freePrefetchedGroupArticle()
{
    struct article *articles;
    long indx;

    if (PrefetchedGroup) {
	articles = GETARTICLES(PrefetchedGroup);
	indx = PrefetchedGroup->current - PrefetchedGroup->first;
    
	/* free the first article if it has been prefetched */
	if (IS_FETCHED(articles[indx])) {
	    CLEAR_FILE(articles[indx]);
	}
    }
    
    return;
}

#endif /* XRN_PREFETCH */

#ifdef STUPIDMMU

/* utterly disgusting interface so things will work like before
	should store this info in some other format than struct list
	and some other field than nglist
*/

static void updateNglist _ARGUMENTS((struct newsgroup *));

static void updateNglist(newsgroup)
    struct newsgroup *newsgroup;
{
	int nocomma = 1, inrange = 1;
	art_num lastread = 1, j;
	struct list *item, *it, *next;

	if ((newsgroup->articles == NIL(struct article))
				|| newsgroup->nglist) {
	  printf("unexpected call to updateNglist\n");
	  return;
	}
	
	if (newsgroup->last >= newsgroup->first) {

	    for (j = newsgroup->first; j <= newsgroup->last; j++) {
		if (inrange && IS_UNREAD(newsgroup->articles[INDEX(j)]) &&
		    !IS_UNAVAIL(newsgroup->articles[INDEX(j)])) {
		    if (lastread == j - 1) {
			item = ALLOC(struct list);
			item->type = SINGLE;
			item->contents.single = lastread;
			item->next = newsgroup->nglist;
			newsgroup->nglist = item;
		    } else {
			if ((j - 1) > 0) {
			  item = ALLOC(struct list);
			  item->type = RANGE;
			  item->contents.range.start = lastread;
			  item->contents.range.end = j - 1;
			  item->next = newsgroup->nglist;
			  newsgroup->nglist = item;
			}
		    }

		    inrange = 0;
		} else if (!inrange && IS_READ(newsgroup->articles[INDEX(j)])) {
		    inrange = 1;
		    lastread = j;
		}
	    }
	    
	    if (inrange) {
		if (lastread == newsgroup->last) {
		  item = ALLOC(struct list);
		  item->type = SINGLE;
		  item->contents.single = lastread;
		  item->next = newsgroup->nglist;
		  newsgroup->nglist = item;
		} else {
		  item = ALLOC(struct list);
		  item->type = RANGE;
		  item->contents.range.start = lastread;
		  item->contents.range.end = newsgroup->last;
		  item->next = newsgroup->nglist;
		  newsgroup->nglist = item;
		}
	    }
	} else {
	    if (newsgroup->last > 1) {
		  item = ALLOC(struct list);
		  item->type = RANGE;
		  item->contents.range.start = 1;
		  item->contents.range.end = lastread;
		  item->next = newsgroup->nglist;
		  newsgroup->nglist = item;
	    }
	}

	/* reverse order */
	for (it = newsgroup->nglist, newsgroup->nglist = NIL(struct list);
			it; it = next) {
	  next = it->next;
	  it->next = newsgroup->nglist;
	  newsgroup->nglist = it;
	}
}


#if 0 /* not used */
void dumpNg(newsgroup)
    struct newsgroup *newsgroup;
{
	struct list *it;

	for (it = (struct list *) newsgroup->nglist; it; it = it->next) {
	  printf("type = %d", it->type);
	  if (it->type == RANGE) {
	    printf(" start = %d, end = %d, next = 0x%x\n",
	      it->contents.range.start, it->contents.range.end,
	      it->next);
	  } else {
	    printf(" single = %d, next = 0x%x\n", it->contents.single,
	      it->next);
	  }
	}
}
#endif

#endif /* STUPIDMMU */


/*
 *   release some resources
 *
 *   returns: void
 */
void releaseNewsgroupResources(newsgroup)
    struct newsgroup *newsgroup;
{
#ifdef DEBUG
    fprintf(stderr, "releaseNewsgroupResources(%s)\n",
	newsgroup->name ? newsgroup->name : "NULL");
#endif /* DEBUG */

    if (newsgroup) {
	struct article *articles = GETARTICLES(newsgroup);
	art_num art;

	if ((newsgroup->last == 0) || EMPTY_GROUP(newsgroup)) {
	    return;
	}

	for (art = newsgroup->first; art <= newsgroup->last; art++) {
	    long indx = INDEX(art);
	    
	    CLEAR_SUBJECT(articles[indx]);
	    CLEAR_AUTHOR(articles[indx]);
	    CLEAR_LINES(articles[indx]);
	    CLEAR_FILE(articles[indx]);
	    SET_UNMARKED(articles[indx]);
#ifndef STUPIDMMU
	    SET_STRIPPED_HEADERS(articles[indx]);
	    SET_UNROTATED(articles[indx]);
	    SET_UNXLATED(articles[indx]);
#endif
	}
#ifdef STUPIDMMU
	/* free the articles array every time */
	updateNglist(newsgroup);
	CLEAR_ARTICLES(newsgroup);
#endif
    }

    return;
}


/*
 * update an article array if the first and/or last article numbers change
 *
 *  returns: void
 */
void articleArrayResync(newsgroup, first, last, number)
    struct newsgroup *newsgroup;
    art_num first, last;
    int number;
{
#ifdef STUPIDMMU
    struct article *articles;
#else
    struct article *articles = GETARTICLES(newsgroup);
#endif
    struct article *newarticles;
    int i;

    /*
     * if there are actually no articles in the group, free up the
     * article array and set the first/last values
     */
       
    if (number == 0) {
	CLEAR_ARTICLES(newsgroup);
	newsgroup->first = newsgroup->last + 1;
	return;
    }
    
    /* refuse to resync if last == 0 */
    if (last == 0) {
	return;
    }

    if (first > last) {
	/* all articles have been removed */

	CLEAR_ARTICLES(newsgroup);
	newsgroup->first = newsgroup->last + 1;
	return;
    }

    /* don't allow last to go backwards!!! */
    if (last < newsgroup->last) {
	last = newsgroup->last;
    }

    if ((first != newsgroup->first) || (last != newsgroup->last)) {

#ifdef STUPIDMMU
	/* only do this if the articles array has been allocated */
	if (newsgroup->articles) {
#endif

	/* the first/last values have changed, resync */
	newarticles = ARRAYALLOC(struct article, last - first + 1);

	/*
	 * initialize the new article structures
	 */
	
	for (i = first; i <= last; i++) {
	    newarticles[i - first].subject = NIL(char);
	    newarticles[i - first].author = NIL(char);
	    newarticles[i - first].filename = NIL(char);
	    newarticles[i - first].lines = NIL(char);
	    if (i < newsgroup->first) {
		/* handle first decreasing... mark them as read */
		newarticles[i - first].status = ART_CLEAR_READ;
	    } else {
		newarticles[i - first].status = ART_CLEAR;
	    }
	}

#ifdef STUPIDMMU
	articles = newsgroup->articles;
#endif
	if (!EMPTY_GROUP(newsgroup) && (newsgroup->first != 0) && (newsgroup->last != 0)) {
	    for (i = first; i <= last; i++) {
		if ((i >= newsgroup->first) && (i <= newsgroup->last)) {
		    newarticles[i - first] = articles[INDEX(i)];
		}
	    }

	    /* free up the old resources - before the new first */
	    for (i = newsgroup->first; i < first; i++) {
		long indx = INDEX(i);
		CLEAR_SUBJECT(articles[indx]);
		CLEAR_AUTHOR(articles[indx]);
		CLEAR_LINES(articles[indx]);
		CLEAR_FILE(articles[indx]);
	    }
	    /* after the new last */
	    for (i = last + 1; i < newsgroup->last; i++) {
		long indx = INDEX(i);
		CLEAR_SUBJECT(articles[indx]);
		CLEAR_AUTHOR(articles[indx]);
		CLEAR_LINES(articles[indx]);
		CLEAR_FILE(articles[indx]);
	    }
	}

	if (articles != NIL(struct article)) {
	    FREE(articles);
	}
	newsgroup->articles = newarticles;

#ifdef STUPIDMMU
	}
#endif

	newsgroup->first = first;
	newsgroup->last = last;
    }

    return;
}


char *localKillFile(newsgroup, mode)
    struct newsgroup *newsgroup;
    int mode;
{
    static char buffer[BUFFER_SIZE];
    char *ptr;
    int i;

    if (!createNewsDir()) {
	return NIL(char);
    }

    (void) strcpy(buffer, app_resources.expandedSaveDir);
    i = strlen(buffer);

#ifndef VMS
    buffer[i++] = '/';

    for (ptr = newsgroup->name; *ptr != 0; ptr++) {
	if (*ptr == '.') {
	    if (mode) {
		buffer[i] = '\0';
		(void) mkdir(buffer, 0777);
	    }
	    buffer[i] = '/';
	    i++;
	} else {
	    buffer[i++] = *ptr;
	}
    }
    buffer[i] = '\0';
    if (mode) {
	(void) mkdir(buffer, 0777);
    }
    (void) strcpy(&buffer[i], "/KILL");
#else
    i = i + utGroupToVmsFilename(&buffer[i], newsgroup->name);
    (void) strcpy(&buffer[i], ".KILL");
#endif

    return buffer;
}

char * globalKillFile()
{
    static char buffer[BUFFER_SIZE];

    if (!createNewsDir()) {
	return NIL(char);
    }
    (void) strcpy(buffer, app_resources.expandedSaveDir);
#ifndef VMS
    (void) strcat(buffer, "/KILL");
#else
    (void) strcat(buffer, "NEWS$GLOBAL.KILL");
#endif
    return buffer;
}


/*
 * add a kill subject/author entry to a kill file
 */
void killItem(item, type)
    char *item;
    int type;
{
    char input[BUFFER_SIZE], *iptr;
    char output[BUFFER_SIZE], *optr;
    FILE *fp;
    struct newsgroup *newsgroup = CurrentGroup;
    char *file;

    if (type == KILL_LOCAL) {
	file = localKillFile(newsgroup, 1);
    } else {
	file = globalKillFile();
    }

    if ((fp = fopen(file, "a")) == NULL) {
	mesgPane(XRN_SERIOUS, CANT_OPEN_KILL_MSG,
		 ((type == KILL_LOCAL) ? "local" : "global"),
		 file, newsgroup->name, errmsg(errno));
	return;
    }

    /* get rid of all magic characters */
    (void) strncpy(input, item, sizeof(input));
    iptr = input;
    optr = output;
    while (*iptr) {
	if ((*iptr == '/')  ||
	    (*iptr == '\\') ||
	    (*iptr == '(')  ||
	    (*iptr == '[')  ||
	    (*iptr == '+')  ||
	    (*iptr == '-')  ||
	    (*iptr == ':')  ||
	    (*iptr == '.')  ||
	    (*iptr == '^')  ||
	    (*iptr == '*')  ||
	    (*iptr == '$')  ||
	    (*iptr == '\'') ||
	    (*iptr == '\"')) {
	    *iptr = '.';
	}
	*optr++ = *iptr++;
    }
    *optr = '\0';

    fprintf(fp, "/%.24s/:j\n", output);
    (void) fclose(fp);
    return;
}


/*
 * kill all subjects in the newsgroup that match the kill
 * orders in fp.
 */
/* XXX  THIS ROUTINE REALLY NEEDS TO BE CLEANED UP */
static void killArticles _ARGUMENTS((struct newsgroup *, FILE *));

static void killArticles(newsgroup, fp)
    struct newsgroup *newsgroup;
    FILE *fp;
{
    char string[BUFFER_SIZE], pattern[BUFFER_SIZE], commands[BUFFER_SIZE];
    char dummy[BUFFER_SIZE];
    art_num i, start = newsgroup->current;
    char *subject, *author, *subj, *ptr, *pptr;
    int scount = 0, kcount = 0, ucount = 0, mcount = 0;
    int append;
    char *reRet;
    char type;

    mesgDisableRedisplay();
    
    info("processing KILL file");

    /* XXX don't reprocess global each time, keep in persistent hash table */

    while (fgets(string, sizeof(string), fp) != NULL) {
	append = 0;

	/*
	 * see if there is a 'THRU artnum' line, if so,
	 * only compare subjects from that article on
	 * XXX need to update THRU
	 */
	if (STREQN(string, "THRU", 4)) {
	    i = atol(&string[5]);
	    /* if THRU is less than current, ignore it */
	    start = MAX(i + 1, newsgroup->current);
	    continue;
	}

	if (*string == '&') {
	    /* 'rn' switch setting, ignore */
	    continue;
	}

	/*
	 * process kill file request should be in the form
	 */
	ptr = string + 1;
	pptr = pattern;

	while (*ptr && (*ptr != '/' || ((*ptr == '/') && *(ptr - 1) == '\\'))) {
	   *pptr++ = *ptr;
	   ptr++;
	}
	*pptr = '\0';

	if (!*ptr) {
	    mesgPane(XRN_SERIOUS, MALFORMED_KILL_ENTRY_MSG, string,
		     newsgroup->name, "no slash terminating the regular expression");
	    continue;
	}

	/* rn puts ': *' in front of patterns, xrn doesn't */
	if (strncmp(pattern, ": *", 3) == 0) {
	    /* deal with overlapping strings */
	    (void) strcpy(dummy, pattern + 3);
	    (void) strcpy(pattern, dummy);
	}

	/* XXX kludge to deal with :.*checks */
	if (*pattern == ':') {
	    /* deal with overlapping strings */
	    (void) strcpy(dummy, pattern + 1);
	    (void) strcpy(pattern, dummy);
	}

#ifdef SYSV_REGEX
	if ((reRet = regcmp(pattern, NULL)) == NULL)
#else
	if ((reRet = re_comp(pattern)) != NIL(char))
#endif
	{
#ifdef SYSV_REGEX
	    mesgPane(XRN_SERIOUS, UNKNOWN_KILL_REGEXP_ERROR_MSG, string);
#else
	    mesgPane(XRN_SERIOUS, KNOWN_KILL_REGEXP_ERROR_MSG, string, reRet);
#endif
	    continue;
	}

	ptr++;	/* skip past the slash */
	(void) strcpy(commands, ptr);
	if ((ptr = index(commands, ':')) == NIL(char)) {
	    mesgPane(XRN_SERIOUS, MALFORMED_KILL_ENTRY_MSG, string,
		     newsgroup->name, "no colon after the regular expression");
	    continue;
	}
	ptr++;	/* skip past the colon */
	type = *ptr;

	switch (type) {
	    case 'j':
	    case 'm':
	    case 's':
	        break;
	    default:
	        mesgPane(XRN_INFO, MALFORMED_KILL_ENTRY_MSG, string,
			 newsgroup->name,
			 "unknown command (valid commands are `j', `m', and `s')");
	        break;
	}
	    
	for (i = start; i <= newsgroup->last; i++) {

	    /* short cut */
	    if (IS_UNAVAIL(newsgroup->articles[INDEX(i)]) ||
		((type == 'j') && IS_READ(newsgroup->articles[INDEX(i)])) ||
		((type == 'm') && IS_UNREAD(newsgroup->articles[INDEX(i)]))) {
		continue;
	    }

	    if (newsgroup->articles[INDEX(i)].subject ||
	        newsgroup->articles[INDEX(i)].author) {

		subject = newsgroup->articles[INDEX(i)].subject;
		author = newsgroup->articles[INDEX(i)].author;

		if (subject) {
		    subj = strip(subject, FALSE);
		}

#ifdef SYSV_REGEX
		if ((subject && (regex(reRet, subj) != NULL)) ||
		    (author  && (regex(reRet, author) != NULL)))
#else
		if ((subject && re_exec(subj)) ||
		    (author  && re_exec(author)))
#endif
                {
		    switch (type) {
			case 'j':
			    SET_READ(newsgroup->articles[INDEX(i)]);
			    if (app_resources.verboseKill == True) {
			        mesgPane(XRN_INFO | append, "killed - %s",
					 subject);
				append = XRN_APPEND;
			    }
			    kcount++;
			    break;

			case 'm':
			    SET_UNREAD(newsgroup->articles[INDEX(i)]);
			    mesgPane(XRN_INFO | append, "marked unread - %s",
				     subject);
			    append = XRN_APPEND;
			    mcount++;
			    break;

			case 's':
			    (void) saveArticle(NIL(char), newsgroup, i);
			    mesgPane(XRN_INFO | append, "saved - %s",
				     subject);
			    append = XRN_APPEND;
			    scount++;
			    break;

			default:
			    ucount++;
			    break;
		    }
		}
	    }
	}
    }

    append = 0;
	    
#define printcount(c,m) \
    if( c != 0 ) \
    {	\
	mesgPane(XRN_INFO | append, m, c, \
		 ((c==1) ? "" : "s"), \
		 newsgroup->name); \
	append = XRN_APPEND; \
    }

    if (app_resources.verboseKill == True) {
	printcount(kcount, "killed %d article%s in %s");
	printcount(mcount, "marked %d article%s unread in %s");
	printcount(scount, "saved %d article%s in %s");
	printcount(ucount, "matched %d article%s with unknown option in %s");
    }

#undef printcount

    mesgEnableRedisplay();

    return;
}

    
/*
 * mark articles as read if in the kill files
 */
static void checkKillFiles _ARGUMENTS((struct newsgroup *));
    
static void checkKillFiles(newsgroup)
    struct newsgroup *newsgroup;
{
    FILE *fp;

    if ((fp = fopen(globalKillFile(), "r")) != NULL) {
	killArticles(newsgroup, fp);
	(void) fclose(fp);
    }

    if ((fp = fopen(localKillFile(newsgroup, 0), "r")) != NULL) {
	killArticles(newsgroup, fp);
	(void) fclose(fp);
    }

    return;
}


#ifdef XRN_PREFETCH

/*ARGSUSED*/
static void checkPrefetch _ARGUMENTS((XtPointer, XtIntervalId *));

static void checkPrefetch(closure, id)
    XtPointer closure;
    XtIntervalId *id;
{
    if (PrefetchInProgress)
	 XtAddTimeOut(1000, checkPrefetch, 0);
}

void finishPrefetch()
{
    while (PrefetchInProgress) {
	XtAddTimeOut(1000, checkPrefetch, 0);
	XtProcessEvent(XtIMTimer);
    }
}

void cancelPrefetch()
{
    if (PrefetchedGroup) {
	freePrefetchedGroupArticle();
	releaseNewsgroupResources(PrefetchedGroup);
	PrefetchedGroup = 0;
    }
    else if (PrefetchInProgress) {
	if (PrefetchingGroup) {
	    releaseNewsgroupResources(PrefetchingGroup);
	}
	PrefetchInProgress = False;
	XtRemoveWorkProc(PrefetchWorkId);
    }
}

#endif /* XRN_PREFETCH */

/*
 * The reason this is divided into "stages" is so that it can be used
 * in a toolkit work procedure.  The stages allow the work to be
 * divided up so a single work procedure invocation doesn't take too long.
 */
static void setUpGroup _ARGUMENTS((struct newsgroup *, int));

static void setUpGroup(newsgroup, stage)
    struct newsgroup *newsgroup;
    int stage;
{
    art_num first, last;
    int number;
    Boolean do_prefetch, unread_only;

#ifdef DEBUG
    fprintf(stderr, "setUpGroup(%s, %d)\n", newsgroup->name, stage);
#endif

#ifdef XRN_PREFETCH
    if (PrefetchInProgress && (stage == 0)) {
	if (PrefetchingGroup == newsgroup) {
	    finishPrefetch();
	}
	else {
	    cancelPrefetch();
	}
    }

    if (PrefetchedGroup && (newsgroup != PrefetchedGroup)) {
	cancelPrefetch();
    }
#endif /* XRN_PREFETCH */

    /* get the latest group information */
    /*
     * No, don't!  This would be a neat thing to do if it were
     * possible to do it without confusing the routines that call
     * setUpGroup, but it's not.  For example, the enterNewsgroup
     * function calls findUnreadArticle before it calls setUpGroup.
     * If there are no unread articles in the newsgroup, then
     * findUnreadArticle will mark one unread so that enterNewsgroup
     * can enter the newsgroup.  However, if we then get the latest
     * group information in setUpGroup and there are new unread
     * articles, then what findUnreadArticle did is no longer correct.
     *
     * It's more reliable to guarantee that the group information is
     * only updated if either (a) a rescan is done or (b) an
     * unsubscribed newsgroup is resubscribed.
    if (getgroup(newsgroup, &first, &last, &number)) {
	return;
    }
    */

#ifdef XRN_PREFETCH
    do_prefetch = ! PrefetchedGroup;
#else
    do_prefetch = True;
#endif

#ifdef XRN_PREFETCH_UNREAD_ONLY
    unread_only = True;
#else
    unread_only = False;
#endif

    if (do_prefetch) {
	if (! stage || stage == 1) {
	    setCurrentArticle(newsgroup);
	}
	if (! EMPTY_GROUP(newsgroup)) {
	    if (! stage || stage == 2)
		 getsubjectlist(newsgroup, newsgroup->current, newsgroup->last,
				unread_only);
	    if (! stage || stage == 3)
		 getauthorlist(newsgroup, newsgroup->current, newsgroup->last,
			       unread_only);
	    if (! stage || stage == 4)
		 getlineslist(newsgroup, newsgroup->current, newsgroup->last,
			      unread_only);
	    if ((! stage || stage == 5) && (app_resources.killFiles == TRUE))
		checkKillFiles(newsgroup);
	}
    }

#ifdef XRN_PREFETCH
    PrefetchedGroup = 0;

#ifndef NO_IMMEDIATE_WORK_PROC_PREFETCH
    if ((stage == 0) && (PrefetchGroupMode == OKAY_TO_XRN_PREFETCH_GROUP)) {
	prefetchNextGroup((XtPointer)newsgroup);
    }
#endif
#endif /* XRN_PREFETCH */

    return;
}


static struct newsgroup * findNewsGroupMatch _ARGUMENTS((char *));

static struct newsgroup * findNewsGroupMatch(name)
    char *name;
{
    static char *reRet;
    int i;

    /* no single character queries, to comfusing */
    if (strlen(name) == 1) {
	return NIL(struct newsgroup);
    }

#ifdef SYSV_REGEX
    if ((reRet = regcmp(name, NULL)) == NULL) {
	return NIL(struct newsgroup);
    }
#else
    if ((reRet = re_comp(name)) != NULL) {
	return NIL(struct newsgroup);
    }
#endif


    for (i = 0; i < MaxGroupNumber; i++) {
	struct newsgroup *newsgroup = Newsrc[i];
#ifdef SYSV_REGEX
	if (regex(reRet, newsgroup->name) != NULL) {
	    return newsgroup;
	}
#else
	if (re_exec(newsgroup->name)) {
	    return newsgroup;
	}
#endif
	    
    }

    return NIL(struct newsgroup);
}


void catchUp()
/*
 * mark all articles in a newsgroup as read
 *
 *   other functions will take care of releasing resources
 *
 *   returns: void
 *
 */
{
    struct newsgroup *newsgroup = CurrentGroup;
    art_num art;

    SETARTICLES(newsgroup);
    for (art = newsgroup->first; art <= newsgroup->last; art++) {
	if (!IS_MARKED(newsgroup->articles[INDEX(art)])) {
	    SET_READ(newsgroup->articles[INDEX(art)]);
	} else {
	    SET_UNREAD(newsgroup->articles[INDEX(art)]);
	    SET_UNMARKED(newsgroup->articles[INDEX(art)]);
	}
    }
    return;
}


int issubscribed()
{
    struct newsgroup *newsgroup = CurrentGroup;

    if (IS_SUBSCRIBED(newsgroup)) {
	return 1;
    } else {
	return 0;
    }
}


/*
 * subscribe to a newsgroup
 *
 *   returns: void
 *
 */
void subscribe()
{
    struct newsgroup *newsgroup = CurrentGroup;

    if (!IS_SUBSCRIBED(newsgroup)) {
	SET_SUB(newsgroup);
	updateArticleArray(newsgroup);
    }
    return;
}


/*
 * unsubscribe to a newsgroup
 *
 *   returns: void
 *
 */
void unsubscribe()
{
    struct newsgroup *newsgroup = CurrentGroup;

    SET_UNSUB(newsgroup);
    return;
}

/*
 * build a string that is used as the question for what
 * to do at the end of an article
 *
 *   returns: the question in a static area
 *
 */
static char * buildQuestion _ARGUMENTS((struct newsgroup *));

static char * buildQuestion(newsgroup)
    struct newsgroup *newsgroup;
{
    static char dummy[LABEL_SIZE];
    art_num i;
    struct article *articles = GETARTICLES(newsgroup);
    long unread = 0;
    ng_num number;
    struct newsgroup *nextnewsgroup;
    int found;

    
    for (i = newsgroup->first; i <= newsgroup->last; i++) {
	long indx = INDEX(i);
	if (IS_UNREAD(articles[indx]) && !IS_UNAVAIL(articles[indx])) {
	    unread++;
	}
    }

    found = 0;
    for (number = CurrentGroup->newsrc + 1; number < MaxGroupNumber;
	 number++) {
	nextnewsgroup = Newsrc[number];
	/* find a group that is subscribed to and has unread articles */
	if (IS_SUBSCRIBED(nextnewsgroup) && (unreadArticleCount(nextnewsgroup) > 0)) {
	    found = 1;
	    break;
	}
    }
	    
    if (found) {
	if (unread <= 0) {
	    (void) sprintf(dummy, "Article %ld in %s (Next group: %s, with %d articles)",
			   newsgroup->current, newsgroup->name,
			   nextnewsgroup->name, unreadArticleCount(nextnewsgroup));
	} else {
	    (void) sprintf(dummy, "Article %ld in %s (%ld remaining) (Next group: %s, with %d articles)",
			   newsgroup->current, newsgroup->name, unread,
			   nextnewsgroup->name, unreadArticleCount(nextnewsgroup));
	}
    } else {
	if (unread <= 0) {
	    (void) sprintf(dummy, "Article %ld in %s",
			   newsgroup->current, newsgroup->name);
	} else {
	    (void) sprintf(dummy, "Article %ld in %s (%ld remaining)",
			   newsgroup->current, newsgroup->name, unread);
	}
    }
    return dummy;
}


static void handleXref _ARGUMENTS((struct newsgroup *, art_num));

static void handleXref(newsgroup, article)
    struct newsgroup *newsgroup;
    art_num article;
{
    char *string, *ptr, *token, group[GROUP_NAME_SIZE], *gptr;
    int count, number;

    xhdr(newsgroup, article, "xref", &string);

    if (string == NIL(char)) {
	/* no xrefs */
	return;
    }

    /*
     * an xrefs line is of the form:
     *
     *   host group:number group:number .... group:number
     */

    if ((ptr = index(string, ' ')) == NIL(char)) {
	FREE(string);
	return;
    }

    while ((token = strtok(ptr, " ")) != NIL(char)) {
	ptr = NIL(char);
	
	count = sscanf(token, "%[^: ]:%d", group, &number);
	if (count != 2) {
	    /* bogus entry */
	    continue;
	}

	if (!avl_lookup(NewsGroupTable, group, &gptr)) {
	    /* bogus group */
	    continue;
	}

	/* only Xref groups that are subscribed to */
	
	newsgroup = (struct newsgroup *) gptr;

	if (IS_SUBSCRIBED(newsgroup) &&
	    (number >= newsgroup->first) && (number <= newsgroup->last)) {

	    SETARTICLES(newsgroup);

	    SET_READ(newsgroup->articles[INDEX(number)]);

#ifdef STUPIDMMU
	    /* this is horribly inefficient
	       should splice directly into nglist
	       or use some other representation XXX */
	    if (newsgroup != CurrentGroup) {
	      updateNglist(newsgroup);
	      CLEAR_ARTICLES(newsgroup);
	    }
#endif

	}
    }
    FREE(string);
    return;
}


/*
 * get the next article
 *
 * returns: XRN_ERROR - article has been canceled
 *          XRN_OKAY  - article returned
 */
int getArticle(filename, question)
    char **filename;
    char **question;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *articles = GETARTICLES(newsgroup);
    long indx = CURRENT;
    int header, rotation;
    int	xlation = 0;

#ifdef DEBUG
    fprintf(stderr, "getArticle(%s)\n", newsgroup->name);
#endif

    if (IS_UNFETCHED(articles[indx])) {
	/* get the article and handle unavailable ones.... */
	header = (IS_ALL_HEADERS(articles[indx]) ?
		  FULL_HEADER : NORMAL_HEADER);
	rotation = (IS_ROTATED(articles[indx]) ?
		    ROTATED : NOT_ROTATED);
#ifdef XLATE
	xlation = (IS_XLATED(articles[indx]) ?
		   XLATED : NOT_XLATED);
#endif
	if ((articles[indx].filename
	     = getarticle(newsgroup, CurrentGroup->current,
			  &articles[indx].position,
			  header, rotation, xlation)) == NIL(char)) {
	    SET_UNAVAIL(articles[indx]);
	    mesgPane(XRN_SERIOUS, ART_NOT_AVAIL_MSG, newsgroup->first + indx);
	    return XRN_ERROR;
	}
	SET_FETCHED(articles[indx]);
    } else {
	/* verify that the file still exists */
	if (access(articles[indx].filename, R_OK) == -1) {
	    /* refetch the file */
	    SET_UNFETCHED(articles[indx]);
	    return getArticle(filename, question);
	}
    }
    
    *filename = articles[indx].filename;
    if (!IS_MARKED(articles[indx])) {
	SET_READ(articles[indx]);
    }
    *question = buildQuestion(newsgroup);
    handleXref(newsgroup, newsgroup->current);
    
    return XRN_OKAY;
}


int toggleHeaders(filename, question)
    char **filename;
    char **question;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *articles = GETARTICLES(newsgroup);

    if (IS_ALL_HEADERS(articles[CURRENT])) {
	SET_STRIPPED_HEADERS(articles[CURRENT]);
    } else {
	SET_ALL_HEADERS(articles[CURRENT]);
    }	
    CLEAR_FILE(articles[CURRENT]);
    return getArticle(filename, question);
}


int toggleRotation(filename, question)
    char **filename;
    char **question;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *articles = GETARTICLES(newsgroup);

    if (IS_ROTATED(articles[CURRENT])) {
	SET_UNROTATED(articles[CURRENT]);
    } else {
	SET_ROTATED(articles[CURRENT]);
    }	
    CLEAR_FILE(articles[CURRENT]);
    return getArticle(filename, question);
}


#ifdef XLATE
int
toggleXlation(filename, question)
char **filename;
char **question;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *articles = GETARTICLES(newsgroup);

    if (IS_XLATED(articles[CURRENT])) {
	SET_UNXLATED(articles[CURRENT]);
    } else {
	SET_XLATED(articles[CURRENT]);
    }	
    CLEAR_FILE(articles[CURRENT]);
    return getArticle(filename, question);
}
#endif /* XLATE */


#ifdef XRN_PREFETCH

typedef struct _workStruct_t
{   long indx;
    struct article * articles;
    struct newsgroup * newsgroup;
}_WorkStruct_t;

static Boolean prefetchSetFetched _ARGUMENTS((XtPointer));

static Boolean prefetchSetFetched(pClient)
    XtPointer pClient;
{
    register long indx = ((_WorkStruct_t *)pClient)->indx;
    register struct article *articles = ((_WorkStruct_t *)pClient)->articles;
    register struct newsgroup *newsgroup = ((_WorkStruct_t *)pClient)->newsgroup;

    if (PrefetchInProgress == False) {
	return True;
    }

    /* if the current article is unfetched, fetch it */
    if (IS_UNFETCHED(articles[indx])) {
	/* if the article can be fetched, mark it so */
	if ((articles[indx].filename =
	     getarticle(newsgroup, newsgroup->current,
			&articles[indx].position, NORMAL_HEADER, NOT_ROTATED,
			NOT_XLATED)) != NIL(char)) {
	    SET_FETCHED(articles[indx]);
	}
    }
    PrefetchedGroup = newsgroup;
#ifdef DEBUG
    fprintf(stderr, "prefetchSetFetched(%s)\n", newsgroup->name);
#endif
    XtFree(pClient);
    PrefetchInProgress = False;

    return True;
}


static Boolean prefetchGetArticles _ARGUMENTS((XtPointer));

static Boolean prefetchGetArticles(pClient)
    XtPointer pClient;
{
    register struct newsgroup *newsgroup = (struct newsgroup *)pClient;
    long indx = CURRENT;
    register struct article *articles = GETARTICLES(newsgroup);

    if (PrefetchInProgress == False) {
	return True;
    }

    if (articles != NIL(struct article)) {
	_WorkStruct_t *w = XtNew(_WorkStruct_t);
	w->indx = indx;
	w->articles = articles;
	w->newsgroup = newsgroup;
	PrefetchWorkId = XtAddWorkProc(prefetchSetFetched,(XtPointer)w);
    } else {
	PrefetchInProgress = False;
    }
    return True;
}


static Boolean prefetchSetupUnreadGroup _ARGUMENTS((XtPointer));

static Boolean prefetchSetupUnreadGroup(pClient)
    XtPointer pClient;
{
    static int setup_stage = 0;

    if (PrefetchInProgress == False) {
	return True;
    }

    if (setup_stage++ < 5) {
	busyCursor();
	setUpGroup((struct newsgroup *)pClient, setup_stage);
	unbusyCursor();
	return False;
    }
    else {
	setup_stage = 0;
	PrefetchWorkId = XtAddWorkProc(prefetchGetArticles,pClient);
	return True;
    }
}


static Boolean prefetchNextUnreadGroup _ARGUMENTS((XtPointer));

/*ARGSUSED*/
static Boolean prefetchNextUnreadGroup(pClient)
    XtPointer pClient;
{
    register struct newsgroup *newsgroup;
    register ng_num number;

    if (! CurrentGroup) {
	 return True;
    }

    number = CurrentGroup->newsrc;

    if (PrefetchInProgress == False) {
	return True;
    }

    /* find a group that is subscribed to and has unread articles */
    while (++number < MaxGroupNumber)
    {   newsgroup = Newsrc[number];
	if (IS_SUBSCRIBED(newsgroup) && (unreadArticleCount(newsgroup) > 0))
	{ break; }
    }
    if (number != MaxGroupNumber
#ifdef XRN_PREFETCH
	&& unreadArticleCount(newsgroup) <= app_resources.prefetchMax
#endif
	)
    {
	PrefetchingGroup = newsgroup;
	PrefetchWorkId = XtAddWorkProc(prefetchSetupUnreadGroup,
	    (XtPointer)newsgroup);
    }
    else
    { PrefetchInProgress = False; }
    return True;
}


/*
 * prefetch the next group
 *
 *   sequence of events:
 *    - put the server into the next group
 *    - get the first unread article
 *    - mark that we are in a new group (but don't change CurrentGroup)
 *
 *    returns: void
 */
static void prefetchNextGroup _ARGUMENTS((XtPointer));

static void prefetchNextGroup(pClient)
    XtPointer pClient;
{
    static XtPointer prefetchReference = 0;

    if (prefetchReference == pClient) {
	return;
    }

    prefetchReference = pClient;

    if (PrefetchInProgress) {
	XtRemoveWorkProc(PrefetchWorkId);
	releaseNewsgroupResources(PrefetchingGroup);
	PrefetchInProgress = False;
    }

    if (PrefetchGroupMode != DO_NOT_XRN_PREFETCH_GROUP) {
	PrefetchWorkId = XtAddWorkProc(prefetchNextUnreadGroup,NULL);
	PrefetchingGroup = 0;
	PrefetchInProgress = True;
    }
    return;
}


/*
 * prefetch the next article, will prefetch the next unread article in
 *   the next group at the end of the current group
 *
 * returns: void
 *
 */
void prefetchNextArticle()
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *articles = GETARTICLES(newsgroup);
    long indx;

#ifdef DEBUG
    fprintf(stderr, "prefetchNextArticle(%s)\n", newsgroup->name);
#endif

    if (newsgroup->current == newsgroup->last) {
#ifdef NO_IMMEDIATE_WORK_PROC_PREFETCH
	/*
	 * if this is the last article, prefetch the next group, and the
	 * first article of the next group
	 */
	prefetchNextGroup((XtPointer)newsgroup);
#endif
	return;
    }

    indx = INDEX(newsgroup->current + 1);

    if (IS_UNFETCHED(articles[indx])) {
	/*
	 * XXX HP Global optimizer did the wrong thing when the
	 * assignment and the if where in the same statement 
	 */
	articles[indx].filename = getarticle(newsgroup, newsgroup->current + 1,
					     &articles[indx].position,
					     NORMAL_HEADER, NOT_ROTATED,
					     NOT_XLATED);
	if (articles[indx].filename == NIL(char)) {
	    return;
	}
	SET_FETCHED(articles[indx]);
	SET_STRIPPED_HEADERS(articles[indx]);
	SET_UNROTATED(articles[indx]);
	return;
    }
    return;
}

#endif /* XRN_PREFETCH */


#ifdef STUPIDMMU
void cornered(newsgroup)
    struct newsgroup *newsgroup;
{
    struct list *item;
    art_num art;

    if (!newsgroup->nglist) {
      fprintf(stderr, "XRN error in `cornered': expecting nglist to be valid\n");
      return;
    }

    /* process the .newsrc line */

    for (item = newsgroup->nglist; item != NIL(struct list); item = item->next) {
	switch (item->type) {
	    case SINGLE:
	    if (item->contents.single > newsgroup->last) {
		/* something really bad has happened, reset */
		mesgPane(XRN_SERIOUS, ART_NUMBERING_PROBLEM_MSG,
			 newsgroup->name);
		for (art = newsgroup->first; art <= newsgroup->last; art++) {
		    newsgroup->articles[INDEX(art)].status = ART_CLEAR;
		}
		lsDestroy(newsgroup->nglist);
		newsgroup->nglist = NIL(struct list);
		return;
	    }
	    if (item->contents.single >= newsgroup->first) {
		newsgroup->articles[INDEX(item->contents.single)].status = ART_CLEAR_READ;
	    }
	    break;

	    case RANGE:
	    if ((item->contents.range.start > newsgroup->last) ||
		(item->contents.range.end > newsgroup->last)) {
		/* something really bad has happened, reset */
		mesgPane(XRN_SERIOUS, ART_NUMBERING_PROBLEM_MSG,
			 newsgroup->name);
		if (newsgroup->articles) {
		    for (art = newsgroup->first; art <= newsgroup->last; art++) {
			newsgroup->articles[INDEX(art)].status = ART_CLEAR;
		    }
		}
		lsDestroy(newsgroup->nglist);
		newsgroup->nglist = NIL(struct list);
		return;
	    }
	    if (item->contents.range.start < newsgroup->first) {
		item->contents.range.start = newsgroup->first;
	    }
	    
	    if (item->contents.range.end < newsgroup->first) {
		break;
	    }
	    for (art = item->contents.range.start; art <= item->contents.range.end; art++) {
		newsgroup->articles[INDEX(art)].status = ART_CLEAR_READ;
	    }
	}
    }

    lsDestroy(newsgroup->nglist);
    newsgroup->nglist = NIL(struct list);
    
}
#endif


/*
 * mark the articles in a group that have been read
 *
 *   returns: void
 *
 */
void updateArticleArray(newsgroup)
    struct newsgroup *newsgroup;    /* newsgroup to update article array for   */
{
    struct list *item;
    art_num art;
#ifndef FIXED_C_NEWS_ACTIVE_FILE
    int number;
#endif

    if (newsgroup->last == 0) {
	return;
    }

    if (EMPTY_GROUP(newsgroup)) {
	newsgroup->articles = NIL(struct article);
	return;
    }

    if (newsgroup->nglist == NIL(struct list)) {
	return;
    }

    if (!IS_SUBSCRIBED(newsgroup)) {
	newsgroup->articles = NIL(struct article);
	return;
    }

#ifdef STUPIDMMU
    if (!newsgroup->articles) {
	return;
    }
#endif

#ifndef FIXED_C_NEWS_ACTIVE_FILE
#ifdef DEBUG
    fprintf(stderr, "updateArticleArray(%s)\n", newsgroup->name);
#endif

    /* get the group range to fix c-news low number problem */
    if ((XRNState & XRN_NEWS_UP) == XRN_NEWS_UP) {
	(void) getgroup(newsgroup, &newsgroup->first, &newsgroup->last, &number);
	if ((newsgroup->first == 0 && newsgroup->last == 0) || number == 0) {
	    lsDestroy(newsgroup->nglist);
	    newsgroup->nglist = NIL(struct list);
	    return;
	}
    }
#endif

    SETARTICLES(newsgroup);

    /* process the .newsrc line */

    for (item = newsgroup->nglist; item != NIL(struct list); item = item->next) {
	switch (item->type) {
	    case SINGLE:
	    if (item->contents.single > newsgroup->last) {
		/* something really bad has happened, reset */
		mesgPane(XRN_SERIOUS, ART_NUMBERING_PROBLEM_MSG,
			 newsgroup->name);
		for (art = newsgroup->first; art <= newsgroup->last; art++) {
		    newsgroup->articles[INDEX(art)].status = ART_CLEAR;
		}
		lsDestroy(newsgroup->nglist);
		newsgroup->nglist = NIL(struct list);
		return;
	    }
	    if (item->contents.single >= newsgroup->first) {
		newsgroup->articles[INDEX(item->contents.single)].status = ART_CLEAR_READ;
	    }
	    break;

	    case RANGE:
	    if ((item->contents.range.start > newsgroup->last) ||
		(item->contents.range.end > newsgroup->last)) {
		/* something really bad has happened, reset */
		mesgPane(XRN_SERIOUS, ART_NUMBERING_PROBLEM_MSG,
			 newsgroup->name);
		for (art = newsgroup->first; art <= newsgroup->last; art++) {
		    newsgroup->articles[INDEX(art)].status = ART_CLEAR;
		}
		lsDestroy(newsgroup->nglist);
		newsgroup->nglist = NIL(struct list);
		return;
	    }
	    if (item->contents.range.start < newsgroup->first) {
		item->contents.range.start = newsgroup->first;
	    }
	    
	    if (item->contents.range.end < newsgroup->first) {
		break;
	    }
	    for (art = item->contents.range.start; art <= item->contents.range.end; art++) {
		newsgroup->articles[INDEX(art)].status = ART_CLEAR_READ;
	    }
	}
    }

    lsDestroy(newsgroup->nglist);
    newsgroup->nglist = NIL(struct list);
    
    return;
}


/*
 * mark an article as read
 */
void markArticleAsRead(article)
    long article;
{
    struct newsgroup *newsgroup = CurrentGroup;

    SETARTICLES(newsgroup);
    SET_READ(newsgroup->articles[INDEX((art_num) article)]);
    SET_UNMARKED(newsgroup->articles[INDEX((art_num) article)]);
    return;
}


/*
 * mark an article as unread
 */
void markArticleAsUnread(article)
    long article;
{
    struct newsgroup *newsgroup = CurrentGroup;

    SETARTICLES(newsgroup);
    SET_UNREAD(newsgroup->articles[INDEX((art_num) article)]);
    SET_MARKED(newsgroup->articles[INDEX((art_num) article)]);
    return;
}


/*
 * handle adding items to the newsrc
 *
 *   3 cases
 *
 *   1. add to the beginning
 *        move 0 to MaxGroupNumber-1 down 1, update newsrc fields
 *        update 0
 *        inc MaxGroupNumber
 *   2. add to the end
 *        inc MaxGroupNumber
 *        update MaxGroupNumber-1
 *   3. add after a group (newloc is the current location of the group)
 *      move newloc+1 to end down 1, update newsrc fields
 *      update newloc
 *      in MaxGroupNumber
 *
 *   And the case of 'subscribe' (assumes it moves)
 *
 *   1. add to the beginning
 *        move 0 to oldloc-1 down 1, update newsrc fields
 *        update 0
 *   2. add to the end
 *        move oldloc+1 to MaxGroupNumber-1 update 1, update newsrc fields
 *        upadte MaxGroupNumber-1
 *   3. add after a group
 *
 */

int addToNewsrcBeginning(newGroup, status)
    char *newGroup;
    int status;
{
    char *ptr;
    struct newsgroup *newsgroup;
    ng_num i;
    
    if (!avl_lookup(NewsGroupTable, newGroup, &ptr)) {
	mesgPane(XRN_SERIOUS, NO_SUCH_NG_MSG, newGroup);
	return BAD_GROUP;
    }
    
    newsgroup = (struct newsgroup *) ptr;

    CLEAR_NOENTRY(newsgroup);
    if (status == SUBSCRIBE) {
	SET_SUB(newsgroup);
    } else {
	SET_UNSUB(newsgroup);
    }
    if (newsgroup->newsrc == NOT_IN_NEWSRC) {
	for (i = MaxGroupNumber - 1; i >= 0; i--) {
	    Newsrc[i + 1] = Newsrc[i];
	    Newsrc[i + 1]->newsrc = i + 1;
	}
    
	MaxGroupNumber++;
	
    } else {
	for (i = newsgroup->newsrc - 1; i >= 0; i--) {
	    Newsrc[i + 1] = Newsrc[i];
	    Newsrc[i + 1]->newsrc = i + 1;
	}
    }
    
    newsgroup->newsrc = 0;
    Newsrc[0] = newsgroup;
    
    return GOOD_GROUP;
}


int addToNewsrcEnd(newGroup, status)
    char *newGroup;
    int status;
{
    char *ptr;
    struct newsgroup *newsgroup;
    ng_num i;
    
    if (!avl_lookup(NewsGroupTable, newGroup, &ptr)) {
	mesgPane(XRN_SERIOUS, NO_SUCH_NG_MSG, newGroup);
	return BAD_GROUP;
    }
    
    newsgroup = (struct newsgroup *) ptr;

    CLEAR_NOENTRY(newsgroup);
    if (status == SUBSCRIBE) {
	SET_SUB(newsgroup);
    } else {
	SET_UNSUB(newsgroup);
    }
    if (newsgroup->newsrc == NOT_IN_NEWSRC) {
	MaxGroupNumber++;
    } else {
	for (i = newsgroup->newsrc + 1; i < MaxGroupNumber; i++) {
	    Newsrc[i - 1] = Newsrc[i];
	    Newsrc[i - 1]->newsrc = i - 1;
	}
    }
    
    newsgroup->newsrc = MaxGroupNumber - 1;
    Newsrc[MaxGroupNumber - 1] = newsgroup;
    
    return GOOD_GROUP;
}


int addToNewsrcAfterGroup(newGroup, afterGroup, status)
    char *newGroup;
    char *afterGroup;
    int status;
{
    char *ptr;
    struct newsgroup *newsgroup, *ng;
    ng_num newloc, i;
    
    if (!avl_lookup(NewsGroupTable, newGroup, &ptr)) {
	mesgPane(XRN_SERIOUS, NO_SUCH_NG_MSG, newGroup);
	return BAD_GROUP;
    }
    
    newsgroup = (struct newsgroup *) ptr;

    CLEAR_NOENTRY(newsgroup);
    if (status == SUBSCRIBE) {
	SET_SUB(newsgroup);
    } else {
	SET_UNSUB(newsgroup);
    }
    if (!avl_lookup(NewsGroupTable, afterGroup, &ptr)) {
	mesgPane(XRN_SERIOUS, NO_SUCH_NG_MSG, afterGroup);
	return BAD_GROUP;
    }
    
    ng = (struct newsgroup *) ptr;
    newloc = ng->newsrc;

    if (newloc == NOT_IN_NEWSRC) {
	mesgPane(XRN_SERIOUS, NOT_IN_NEWSRC_MSG, afterGroup);
	return BAD_GROUP;
    }

    if (newsgroup->newsrc == NOT_IN_NEWSRC) {
	for (i = MaxGroupNumber - 1; i >= newloc + 1; i--) {
	    Newsrc[i + 1] = Newsrc[i];
	    Newsrc[i + 1]->newsrc = i + 1;
	}

	MaxGroupNumber++;
	newsgroup->newsrc = newloc + 1;
	Newsrc[newloc + 1] = newsgroup;

    } else {
	
	if (newloc + 1 < newsgroup->newsrc) {
	    for (i = newsgroup->newsrc - 1; i >= newloc + 1; i--) {
		Newsrc[i + 1] = Newsrc[i];
		Newsrc[i + 1]->newsrc = i + 1;
	    }
	    newsgroup->newsrc = newloc + 1;
	    Newsrc[newloc + 1] = newsgroup;
	    
	} else if (newsgroup->newsrc < newloc + 1) {
	    for (i = newsgroup->newsrc + 1; i < newloc + 1; i++) {
		Newsrc[i - 1] = Newsrc[i];
		Newsrc[i - 1]->newsrc = i - 1;
	    }
	    newsgroup->newsrc = newloc;
	    Newsrc[newloc] = newsgroup;
	}
	/* if its in the correct location already, don't touch it */
    }
    
    return GOOD_GROUP;
}


/*
 * build and return a string that shows the subscription status
 * of all newsgroups; assumes all groups have been subscribed or
 * unsubscribed to by this time.
 *
 *   if sorted is non-zero, the list is sorted alphabetically, if
 *    zero, the list is returned as it exists in the newsrc file
 */
char * getStatusString(sorted)
    int sorted;
{
    int i, count = 0, bytes = 0;
    char buffer[1024];
    avl_generator *gen;
    char *key, *value;
    char **ar;
    char *string;

    ar = ARRAYALLOC(char *, ActiveGroupsCount);

    if (sorted) {
	gen = avl_init_gen(NewsGroupTable, AVL_FORWARD);
	if (! gen) {
	    ehErrorExitXRN("out of memory");
	}

	while (avl_gen(gen, &key, &value)) {
	    struct newsgroup *newsgroup = (struct newsgroup *) value;
	    
	    (void) sprintf(buffer, "%-60s%s",
			   newsgroup->name,
			   (IS_SUBSCRIBED(newsgroup) ? "subscribed  " : "unsubscribed"));
	    ar[count++] = XtNewString(buffer);
	    bytes += strlen(buffer);
	}
	avl_free_gen(gen);
	
    } else {
	for (i = 0; i < MaxGroupNumber; i++) {
	    struct newsgroup *newsgroup = (struct newsgroup *) Newsrc[i];
	    
	    (void) sprintf(buffer, "%-60s%s",
			   newsgroup->name,
			   (IS_SUBSCRIBED(newsgroup) ? "subscribed  " : "unsubscribed"));
	    ar[count++] = XtNewString(buffer);
	    bytes += strlen(buffer);
	}
    }	    
    
    string = arrayToString(ar, count, bytes);
    for (i = 0; i < count; i++) {
	FREE(ar[i]);
    }
    FREE(ar);
    
    return string;
}


/*
 * build and return the subjects string
 */
static char * getUnSortedSubject _ARGUMENTS((int));

static char * getUnSortedSubjects(mode)
    int mode;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *articles = GETARTICLES(newsgroup);
    int subLength;
    int lineLength = app_resources.indexLineLength;
    art_num i;
    char *start, *end;

    if (EMPTY_GROUP(newsgroup)) {
	return XtNewString("");
    }

    if ( lineLength < 50) lineLength = 50;
    else if ( lineLength >= LINE_LENGTH) lineLength = LINE_LENGTH - 1;
    if (app_resources.displayLineCount) {
	subLength = lineLength - 1 - 6 - 1 - 20 - 1;
    } else {
	subLength = lineLength - 2 - 20 - 1;
    }
	

    NextPreviousArticle = newsgroup->current - 1;

    if ((newsgroup->last - newsgroup->current + 1) < 0) {
	(void) sprintf(error_buffer, "Active File Error: last - current + 1 < 0 (%s)\n",
			       newsgroup->name);
	ehErrorExitXRN(error_buffer);
    }
	
    start = ARRAYALLOC(char, ((newsgroup->last - newsgroup->current + 1) * lineLength + 1));
    end = start;
    *start = '\0';

    for (i = newsgroup->current; i <= newsgroup->last; i++) {
	long indx = INDEX(i);

	/* canceled and empty articles will not have a subject entry */
	if (articles[indx].subject != NIL(char)) {
	    /* don't put articles in the string if already read ... */
	    if ((mode == ALL) || (IS_UNREAD(articles[indx]) &&
				  !IS_UNAVAIL(articles[indx]))) {
		if (app_resources.displayLineCount) {
		    (void) sprintf(end, "%*.*s %6.6s %-20.20s\n",
				   -subLength,subLength,
				   articles[indx].subject,
				   articles[indx].lines,
				   (articles[indx].author == NIL(char) ?
				     "(none)" : articles[indx].author));
		} else {
		    (void) sprintf(end, "%*.*s  %-20.20s\n",
				   -subLength,subLength,
				   articles[indx].subject,
				   (articles[indx].author == NIL(char) ?
				     "(none)" : articles[indx].author));
		}

		/* mark articles if they have already been read */
		if (IS_READ(articles[indx])) {
		    *end = '+';
		} else {
		    if (IS_MARKED(articles[indx])) {
			*end = 'u';
		    } else {
			*end = ' ';
		    }
		}
		if (IS_SAVED(articles[indx])) {
		    *(end + 1)  = 'S';
		} else if(IS_PRINTED(articles[indx])) {
		    *(end + 1)  = 'P';
		} else {
		    *(end + 1)  = ' ';
		}

		end += lineLength;
		*end = '\0';
	    }
	} else {
	    /* if you can't get the subject, mark the article as unavailable */
	    SET_UNAVAIL(articles[indx]);
	}
    }

    return start;
}


struct entry {
    char *beginning;
    char *end;
    int left;
    int size;
    int startingArticle;
};


static void valfree _ARGUMENTS((void *));

static void valfree(p)
    void *p;
{
    struct entry *val = (struct entry *)p;
    XtFree(val->beginning);
    XtFree((char *) val);
    return;
}


static int pteCompare _ARGUMENTS((void *, void *));

static int pteCompare(a, b)
    void *a, *b;
{
    struct entry **pa = (struct entry **) a;
    struct entry **pb = (struct entry **) b;

    return (*pa)->startingArticle - (*pb)->startingArticle;
}


/*
 * XXX AVL TREE is the wrong data structure here, hash table would be
 * better.... no need for ordering based on subject string
 */
/*
 * build and return the subjects string
 */

#define CHUNK (4 * lineLength)

static char * getSortedSubjects _ARGUMENTS((int));

static char * getSortedSubjects(mode)
    int mode;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *articles = GETARTICLES(newsgroup);
    int subLength;
    int lineLength = app_resources.indexLineLength;
    art_num i;
    char *start, *end;

    struct entry *pte;
    struct entry **pteArray;

    avl_generator *gen;
    char *key, *ptr;
    avl_tree *tree;
    char curSub[80];
    int treeSize, sz;

    if (EMPTY_GROUP(newsgroup)) {
	return XtNewString("");
    }

    if ( lineLength < 50) lineLength = 50;
    else if ( lineLength >= LINE_LENGTH) lineLength = LINE_LENGTH - 1;
    if (app_resources.displayLineCount) {
	subLength = lineLength - 1 - 6 - 1 - 20 - 1;
    } else {
	subLength = lineLength - 2 - 20 - 1;
    }
	
    tree = avl_init_table(utSubjectCompare);
    if (! tree) {
	ehErrorExitXRN("out of memory");
    }

    NextPreviousArticle = newsgroup->current - 1;

    if ((newsgroup->last - newsgroup->current + 1) < 0) {
	(void) sprintf(error_buffer, "Active File Error: last - current + 1 < 0 (%s)\n",
			       newsgroup->name);
	ehErrorExitXRN(error_buffer);
    }
	
    /* 
     * build the subject groups 
     */
    for (i = newsgroup->current; i <= newsgroup->last; i++) {
	long indx = INDEX(i);

	/* canceled and empty articles will not have a subject entry */
	if (articles[indx].subject != NIL(char)) {
	    char buffer[LINE_LENGTH];

	    /* don't put articles in the string if already read ... */
	    if ((mode == ALL) || (IS_UNREAD(articles[indx]) &&
				  !IS_UNAVAIL(articles[indx]))) {
		if (app_resources.displayLineCount) {
		    (void) sprintf(buffer, "%*.*s %6.6s %-20.20s\n",
				   -subLength,subLength,
				   articles[indx].subject,
				   articles[indx].lines,
				   (articles[indx].author == NIL(char) ?
				     "(none)" : articles[indx].author));
		} else {
		    (void) sprintf(buffer, "%*.*s  %-20.20s\n",
				   -subLength,subLength,
				   articles[indx].subject,
				   (articles[indx].author == NIL(char) ?
				     "(none)" : articles[indx].author));
		}

		(void) strncpy(curSub, getSubject(i), sizeof(curSub));
		/* mark articles if they have already been read */
		if (IS_READ(articles[indx])) {
		    buffer[0] = '+';
		} else {
		    if (IS_MARKED(articles[indx])) {
			buffer[0] = 'u';
		    } else {
			buffer[0] = ' ';
		    }
		}
		if (IS_SAVED(articles[indx])) {
		    buffer[1] = 'S';
		} else if(IS_PRINTED(articles[indx])) {
		    buffer[1] = 'P';
		} else {
		    buffer[1] = ' ';
		}

		if (avl_lookup(tree, curSub, &ptr)) {
		    /* add to the end */
		    pte = (struct entry *) ptr;
		    if (lineLength >= pte->left) {
			/* grow the string */
			pte->size += CHUNK;
			pte->left += CHUNK;
			sz = pte->end - pte->beginning;
			pte->beginning = XtRealloc(pte->beginning, pte->size);
			pte->end = pte->beginning + sz;
			*(pte->end) = '\0';
		    }
		    (void) strcpy(pte->end, buffer);
		    pte->end += lineLength;
		    pte->left -= lineLength;
		    *(pte->end) = '\0';
		} else {
		    /* create new */
		    pte = ALLOC(struct entry);
		    pte->startingArticle = i;
		    pte->beginning = ARRAYALLOC(char, CHUNK);
		    (void) strcpy(pte->beginning, buffer);
		    pte->size = CHUNK;
		    pte->left = pte->size - lineLength;
		    pte->end = pte->beginning + lineLength;
		    *(pte->end) = '\0';
		    if (avl_insert(tree, XtNewString(curSub), (char *) pte)
			< 0) {
			ehErrorExitXRN("out of memory");
		    }
		}
	    }
	} else {
	    /* if you can't get the subject, mark the article as unavailable */
	    SET_UNAVAIL(articles[indx]);
	}
    }

    i = 0;
    treeSize = avl_count(tree);
    pteArray = ARRAYALLOC(struct entry *, treeSize);
    gen = avl_init_gen(tree, AVL_FORWARD);
    if (! gen) {
	ehErrorExitXRN("out of memory");
    }
    while (avl_gen(gen, &key, &ptr)) {
	pteArray[i++] = (struct entry *) ptr;
    }
    avl_free_gen(gen);

    /* sort by article number */
    qsort((char *) pteArray, treeSize, sizeof(struct pte *), pteCompare);

    start = ARRAYALLOC(char, ((newsgroup->last - newsgroup->current + 1) * lineLength + 1));
    end = start;
    *start = '\0';

    for (i = 0; i < treeSize; i++) {
	(void) strcpy(end, pteArray[i]->beginning);
	end += utStrlen(pteArray[i]->beginning);
	*end = '\0';
    }

    avl_free_table(tree, XtFree, valfree);
    FREE(pteArray);

    return start;
}



char * getSubjects(mode)
    int mode;
{
    if (app_resources.sortedSubjects) {
	return getSortedSubjects(mode);
    } else {
	return getUnSortedSubjects(mode);
    }
}


/*
 * set the internal pointers to a particular article
 */
void gotoArticle(article)
    long article;
{
    struct newsgroup *newsgroup = CurrentGroup;
    newsgroup->current = (art_num) article;
    return;
}


int checkArticle(art)
    art_num art;
{
    struct newsgroup *newsgroup = CurrentGroup;

    /* Check if requested article is not available */
    if (art < newsgroup->first || art > newsgroup->last) {
        return XRN_ERROR;
    }
    return XRN_OKAY;
}    


/*
 * first and last are the same and there are no articles
 *
 * representation bug in the news system active file
 *
 */
void bogusNewsgroup()
{
    struct newsgroup *newsgroup = CurrentGroup;

    if (newsgroup->articles != NIL(struct article)) {
	SET_READ(newsgroup->articles[LAST]);
	SET_UNAVAIL(newsgroup->articles[LAST]);
    }

    mesgPane(XRN_INFO, PROBABLY_EXPIRED_MSG, newsgroup->name);
    return;
}


#define STRIPLEADINGSPACES   for (; *start == ' ' || *start == '\t'; start++);
#define STRIPENDINGSPACES  for ( ; *end == ' ' || *end == '\t'; *end = '\0', end--);

static char * strip(str, striprefs)
    char *str;
    Boolean striprefs;
{
    register char *start, *end, *ptr;
    static char work[BUFFER_SIZE];

    (void) strncpy(work, str, BUFFER_SIZE);
    start = work;
    work[BUFFER_SIZE - 1] = '\0';

    /* a tab separates the article number from the subject line */
    start = index(start, '\t');
    assert(start != NIL(char));
    start++;

    STRIPLEADINGSPACES;

    /*
     * strip leading '[rR][eE]: ' and 'Re^N: ' -
     * only if striprefs is TRUE (want to be able to kill follow-ups)
     */
    if (striprefs) {
	while (STREQN(start, "Re: ", 4) ||
	       STREQN(start, "RE: ", 4) ||
	       STREQN(start, "re: ", 4) ||
	       STREQN(start, "Re: ", 4)) {
	    start += 4;
	
	    /* strip leading spaces after '[rR]e: ' */
	    STRIPLEADINGSPACES;
	}

	while (STREQN(start, "Re^", 3)) {
	    start += 3;
	    ptr = index(start, ':');
	    if (ptr != NIL(char)) {
		start = ptr + 1;
	    }
	    STRIPLEADINGSPACES;
	}
	
	for (end = start; (end = index(end, '(')) != NULL;)
	  if (STREQN(end,"(was:",5)
	      ||  STREQN(end,"(Was:",5)
	      ||  STREQN(end,"(WAS:",5)) {
	      *end = '\0';
	      break;
	  }
	  else ++end;
    }

    end = index(start, '\0') - 1;
    STRIPENDINGSPACES;

    return start;
}


/*
 * return the subject of an article with trailing/leading spaces stripped,
 * leading '[rR]e: ' stripped, and trailing ' ([wW]as: ' stripped
 */
char * getSubject(article)
    long article;
{
    struct newsgroup *newsgroup = CurrentGroup;
    art_num artnum = (art_num) article;

    SETARTICLES(newsgroup);

    return strip(newsgroup->articles[INDEX(artnum)].subject, TRUE);
}


/*
 * return the author of an article
 */
char * getAuthor(article)
    long article;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *articles = GETARTICLES(newsgroup);

    return articles[INDEX((art_num) article)].author;
}


/*
 * get the previous subject (article number is NextPreviousArticle).
 * only called when going off the top of the subject string
 *
 *   returns a point to a static area
 *
 *  NextPreviousArticle is set to current-1 on building the subject string.
 *  NextPreviousArticle is decremented by this routine.
 */
char * getPrevSubject()
{
    int subLength;
    int lineLength = app_resources.indexLineLength;
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *articles = GETARTICLES(newsgroup);
    static char buffer[BUFFER_SIZE];

    if ( lineLength < 50) lineLength = 50;
    else if ( lineLength >= LINE_LENGTH) lineLength = LINE_LENGTH - 1;
    if (app_resources.displayLineCount) {
	subLength = lineLength - 1 - 6 - 1 - 20 - 1;
    } else {
	subLength = lineLength - 2 - 20 - 1;
    }

#ifdef DEBUG
    fprintf(stderr, "getPrevSubject(%s)\n", newsgroup->name);
#endif

    /* search for the next available article in the reverse direction */
    for ( ; NextPreviousArticle >= newsgroup->first; NextPreviousArticle--) {
	long indx = INDEX(NextPreviousArticle);
	
#ifdef notdef
	(void) sprintf(error_buffer, "searching article %ld", NextPreviousArticle);
	infoNow(error_buffer);
#endif

	/* get the subject (and author) if it does not already exist */
	if (articles[indx].subject == NIL(char)) {

	    /* get the subject and a few more */
	    getsubjectlist(newsgroup,
			   MAX(newsgroup->first, NextPreviousArticle - SUBJECTS),
			   NextPreviousArticle, False);
	}

	if (articles[indx].author == NIL(char)) {
	    getauthorlist(newsgroup,
			  MAX(newsgroup->first, NextPreviousArticle - SUBJECTS),
			   NextPreviousArticle, False);

	}

	if (articles[indx].lines == NIL(char)) {
	    getlineslist(newsgroup,
			 MAX(newsgroup->first, NextPreviousArticle - SUBJECTS),
			 NextPreviousArticle, False);
	}

	if (articles[indx].subject) {
	    if (app_resources.displayLineCount) {
		(void) sprintf(buffer, "%*.*s %6.6s %-20.20s",
			       -subLength,subLength,
			       articles[indx].subject,
			       articles[indx].lines,
			       (articles[indx].author == NIL(char) ?
				 "(none)" : articles[indx].author));
	    } else {
		(void) sprintf(buffer, "%*.*s  %-24.24s",
			       -subLength,subLength,
			       articles[indx].subject,
			       (articles[indx].author == NIL(char) ?
				 "(none)" : articles[indx].author));
	    }

            NextPreviousArticle--;
	    return buffer;
	}
	/* continue on */
    }

    NextPreviousArticle--;
    return NIL(char);
}


static art_num justInCase;   /* old NextPreviousArticle, just in case the search fails */

/* the front-end is about to do an article search, save the starting point */
void startSearch()
{
    justInCase = NextPreviousArticle;
    return;
}


/* the article search failed, restore the original point */
void failedSearch()
{
    NextPreviousArticle = justInCase;
    return;
}


void fillUpArray(art)
    art_num art;
{
    struct newsgroup *newsgroup = CurrentGroup;

#ifdef DEBUG
    fprintf(stderr, "fillUpArray(%s)\n", CurrentGroup->name);
#endif

    getsubjectlist(newsgroup, art, newsgroup->last, False);
    getauthorlist(newsgroup, art, newsgroup->last, False);
    getlineslist(newsgroup, art, newsgroup->last, False);
    return;
}


/*
 * getinfofromfile	Get a string from a named file
 *			Handle white space and comments.
 *
 *	Parameters:	"file" is the name of the file to read.
 *
 *	Returns:	Pointer to static data area containing the
 *			first non-ws/comment line in the file.
 *			NULL on error (or lack of entry in file).
 *
 *	Side effects:	None.
 */

char * getinfofromfile(file)
    char *file;
{
	register FILE	*fp;
	register char	*cp;
	static char	buf[256];

	if (file == NULL)
		return (NULL);

	fp = fopen(file, "r");
	if (fp == NULL)
		return (NULL);

	while (fgets(buf, sizeof (buf), fp) != NULL) {
		if (*buf == '\n' || *buf == '#')
			continue;
		cp = index(buf, '\n');
		if (cp)
			*cp = '\0';
		(void) fclose(fp);
		return (buf);
	}

	(void) fclose(fp);
	return (NULL);			 /* No entry */
}


static Boolean articleIsAvailable _ARGUMENTS((struct newsgroup *, art_num));

static Boolean articleIsAvailable(newsgroup, i)
    struct newsgroup *newsgroup;
    art_num i;
{
    struct article *art = GETARTICLES(newsgroup);
    char *keep_subject;
    Boolean ret;

    if (! art) {
	return False;
    }

    art = &art[INDEX(i)];

    /* Determine if the article actually exists by */
    /* retrieving its Subject.		       */
    keep_subject = art->subject;
    art->subject = 0;
    getsubjectlist(newsgroup, i, i, False);
    if (art->subject) {
	XtFree(art->subject);
	ret = True;
    }
    else {
	SET_UNAVAIL(*art);
	ret = False;
    }

    art->subject = keep_subject;

    return ret;
}


/*
 * Two arguments:
 *
 * newsgroup		The newsgroup we're working in.
 * unread_only		True if we should only enter the newsgroup if
 * 			there are unread articles in it.
 *
 * Verifies that there is at least one article available in the
 * newsgroup.  If unread_only is false, then it will mark the last
 * available article unread if there are no unread articles.
 *
 * Return value:
 *
 * If unread_only is true, returns GOOD_GROUP if there's at least
 * one available unread article, XRN_NOUNREAD if there are no unread
 * articles, or XRN_NOMORE if there are no articles at all.
 *
 * If unread_only is false, returns GOOD_GROUP if there was at least one
 * available unread article to start out with, XRN_NOUNREAD if there's
 * an available article but it had to be marked unread inside
 * findUnreadArticle, or XRN_NOMORE if there are no articles at all.
 * 
 * NOTE CAREFULLY: This means that if unread_only is false,
 * XRN_NOUNREAD is not an error return value -- it's informational.
 */

static int findUnreadArticle _ARGUMENTS((struct newsgroup *,
				         /* Boolean */ int));

static int findUnreadArticle(newsgroup, unread_only)
    struct newsgroup *newsgroup;
    Boolean unread_only;
{
    art_num i;
    struct article *artarray = GETARTICLES(newsgroup), *art;

    if (! artarray) {
	return False;
    }

    /*
     * First, check if there is an unread article that is available.
     */
    
    for (i = newsgroup->last, art = &artarray[INDEX(newsgroup->last)];
	 i >= newsgroup->first; i--, art--) {
	 if (! IS_UNAVAIL(*art) && IS_UNREAD(*art) &&
	     articleIsAvailable(newsgroup, i)) {
	     return GOOD_GROUP;
	 }
     }

    /*
     * That didn't work, so check if there's an article that we can
     * mark unread (or not, if unread_only is True).
     */

    for (i = newsgroup->last, art = &artarray[INDEX(newsgroup->last)];
	 i >= newsgroup->first; i--, art--) {
	 if (! IS_UNAVAIL(*art) && articleIsAvailable(newsgroup, i)) {
	     if (! unread_only) {
		 SET_UNREAD(*art);
		 newsgroup->current = i;
	     }
	     return XRN_NOUNREAD;
	 }
     }

    return XRN_NOMORE;
}

/*
 * enterNewsgroup
 *
 * Arguments:
 *
 * name			The newsgroup to enter.
 * flags		Bit field specifying parameters.
 * 
 * The parameters:
 * 
 * ENTER_SETUP		If true, then try to set up the newsgroup for
 *			reading.  If false, then just set the current
 *			group to the indicated newsgroup.
 * ENTER_UNREAD		If true, only enter the newsgroup if there are
 * 			unread articles articles in it.  If false,
 * 			then try to find the last available article,
 * 			mark it unread, and enter the group.
 * ENTER_UNSUBBED	If true, then we're allowed to switch to an
 * 			unsubscribed newsgroup.
 * ENTER_JUMPING	If true, then we're jumping to this newsgroup
 * 			so we should disable prefetching, and if the
 * 			newsgroup is unsubscribed, then it should be
 * 			subscribed.
 * ENTER_REGEXP		If the group specified can't be found in the
 * 			table, try to do a regular expression match to
 * 			find it.
 *
 * Behavior:
 *
 * Side effects:
 * 
 * Changes CurrentGroup.  If set_up is true and jumping is false,
 * invalidates any prefetched group.  If set_up is true and jumping is
 * true, disables prefetching.  If jumping is true and the newsgroup
 * is unsubscribed, subscribes to the group.  If set_up is true,
 * updates the group's information.
 *
 * *** DOES NOT display error messages.  That's up to the caller.
 *
 * Return value:
 * 
 * GOOD_GROUP If the group was successfully entered.  BAD_GROUP if the
 * group is invalid or some other strange error occurs.  XRN_UNSUBBED
 * if the group is unsubscribed and ENTER_UNSUBBED isn't set.
 * XRN_NOMORE if ENTER_SETUP is set and there are no articles in the
 * group at all.  XRN_NOUNREAD if ENTER_SETUP is set and there are
 * only read articles in the group (if ENTER_UNREAD is set, this is an
 * error condition; otherwise, this return value is informational
 * rather than an error condition, because the newsgroup was still
 * entered even though there were no unread articles -- the last read
 * article was marked unread).
 */
int enterNewsgroup(name, flags)
    char *name;
    int flags;
{
    struct newsgroup *newsgroup;
    int ret = GOOD_GROUP;

#ifdef XRN_PREFETCH
    if (flags & ENTER_SETUP) {
	if (flags & ENTER_JUMPING) {
	    PrefetchGroupMode = DO_NOT_XRN_PREFETCH_GROUP;
	}
	else {
	    PrefetchGroupMode = OKAY_TO_XRN_PREFETCH_GROUP;
	}
    }
#endif

    if (!avl_lookup(NewsGroupTable, name, (char **) &newsgroup)) {
	if (flags & ENTER_REGEXP) {
	    if (! (newsgroup = findNewsGroupMatch(name))) {
		return BAD_GROUP;
	    }
	}
	else {
	    return BAD_GROUP;
	}
    }

    if (! IS_SUBSCRIBED(newsgroup)) {
	if (flags & ENTER_UNSUBBED) {
	    if (flags & ENTER_JUMPING) {
		SET_SUB(newsgroup);
		updateArticleArray(newsgroup);
	    }
	}
	else {
	    return XRN_UNSUBBED;
	}
    }

    if (flags & ENTER_SETUP) {
	if (EMPTY_GROUP(newsgroup)) {
	    return XRN_NOMORE;
	}

#define doFindUnread() \
	ret = findUnreadArticle(newsgroup, \
				(flags & ENTER_UNREAD) ? True : False); \
	if (ret == XRN_NOMORE) { \
	    return ret; \
	} \
	else if (ret == XRN_NOUNREAD) { \
	    if (flags & ENTER_UNREAD) { \
		return ret; \
	    } \
	} \
	else if (ret != GOOD_GROUP) { \
	    mesgPane(XRN_SERIOUS, UNKNOWN_FIND_UNRD_RESPONSE_MSG, ret, \
		     "enterNewsgroup"); \
	    return ret; \
	}

	doFindUnread();

	setUpGroup(newsgroup, 0);

	/*
	 * We may no longer have any unread articles, because KILL
	 * file processing may be killed them all.
	 */
	doFindUnread();

#undef doFindUnread
    }

    CurrentGroup = newsgroup;

    return ret;
}
