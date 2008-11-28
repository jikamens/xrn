#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: internals.c,v 1.215 1997-12-28 16:36:37 jik Exp $";
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
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include "avl.h"
#include "news.h"
#include "artstruct.h"
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
#include "xmisc.h"
#include "ngMode.h"
#include "artMode.h"
#include "snapshot.h"
#include "modes.h"
#include "cursor.h"
#include "varfile.h"
#include "dialogs.h"
#include "activecache.h"
#include "sort.h"
#include "killfile.h"
#include "file_cache.h"
#include "hash.h"

#ifndef R_OK
#define R_OK 4
#endif


#define BUFFER_SIZE 1024
#define FATAL 0
#define OKAY  1
/*
 * length of lines in the top text window
 */
#define LINE_LENGTH 200

/* set to the value of a group if that group has been prefetched */
static struct newsgroup *PrefetchedGroup = 0;
static struct newsgroup *PrefetchingGroup = 0;
static int PrefetchStage;
static XtWorkProc PrefetchingProc;
static XtWorkProcId prefetch_id;
static Boolean FinishingPrefetch = False;
static int InWorkProc = 0;

#define maybeInfoNow(mesg) \
	_info((mesg), !InWorkProc || FinishingPrefetch)

static void prefetchNextGroup _ARGUMENTS((struct newsgroup *));

#ifndef PREFETCH_CHUNK
#define PREFETCH_CHUNK 5
#endif
#ifndef PREFETCH_MIN_CHUNK
#define PREFETCH_MIN_CHUNK 5
#endif
#ifndef PREFETCH_CHUNK_TIME
#define PREFETCH_CHUNK_TIME 1
#endif

extern Boolean FastServer;

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

struct var_rec *cache_variables = 0;
char *cache_file;

/*
 * see if another xrn is running
 */
void checkLock()
{
    char *buffer = utTildeExpand(app_resources.lockFile);
    char host[64];
    char myhost[64];
    int pid;
    FILE *fp;
    extern int errno;

    if (!buffer) {
	/* silently ignore this condition */
	return;
    }

    if (gethostname(myhost, sizeof(myhost)) == -1) {
	(void) strcpy(myhost, "bogusHost");
    }

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
    (void) fprintf(stderr, ERR_XRN_RUN_MSG , host, pid, buffer);

    exit(-1);

    return;
}


void removeLock()
{
    char *buffer = utTildeExpand(app_resources.lockFile);

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
    char *err_buf;

    xrnBusyCursor();

    if (! (cache_file = findServerFile(app_resources.cacheFile, True))) {
	err_buf = XtMalloc(strlen(CANT_EXPAND_MSG)+MAXPATHLEN);
	(void) sprintf(err_buf, CANT_EXPAND_MSG, app_resources.cacheFile);
	ehErrorExitXRN(err_buf);
    }

    cache_variables = var_read_file(cache_file);

    start_server();

    do {
	 NewsGroupTable = avl_init_table(strcmp);
	 if (! NewsGroupTable) {
           ehErrorExitXRN(ERROR_OUT_OF_MEM_MSG);
	 }

	 if (app_resources.cacheActive) {
	   active_cache_read(cache_file);
	 }
	 else {
	   getactive(False);
	   if (! app_resources.fullNewsrc)
	     getactive(True);
	 }

	 if (readnewsrc(app_resources.newsrcFile,
			app_resources.saveNewsrcFile))
	      break;
	 
       ehErrorRetryXRN(ERROR_CANT_READ_NEWSRC_MSG, True);

	 avl_free_table(NewsGroupTable, XtFree,
			(void (*) _ARGUMENTS((void *))) XtFree);
    } while (1);

    if (app_resources.cacheActive)
      rescanBackground();

    xrnUnbusyCursor();

    return;
}

static XtWorkProcId rescan_id = 0;
static int active_serial;
static ng_num rescan_group_number;

/*
  A work procedure which fetches the statistics for a single
  newsgroup, using the getgroup() function.  Give up if the group
  number we're passed in is negative or greater than MaxGroupNumber.
  When we reach MaxGroupNumber, display a message indicating that
  we're done.
  */
static Boolean rescanWorkProc(data)
XtPointer data;
{
  art_num first, last;
  int number;

#ifdef DEBUG
  fprintf(stderr, "rescanWorkProc(%d)\n", rescan_group_number);
#endif

  if (active_read > active_serial) {
    rescan_id = 0;
    return True;
  }

  if (rescan_group_number == MaxGroupNumber) {
    char *buf = XtMalloc(strlen(RESCANNING_BACKGROUND_MSG) + strlen(DONE_MSG) +
			 2);
    (void) sprintf(buf, "%s %s", RESCANNING_BACKGROUND_MSG, DONE_MSG);
    INFO(buf);
    XtFree(buf);
    rescan_group_number++;
  }

  if (rescan_group_number > MaxGroupNumber) {
    rescan_id = 0;
    return True;
  }

  if (Newsrc[rescan_group_number] == CurrentGroup) {
    rescan_group_number++;
    return False;
  }

  if (! IS_SUBSCRIBED(Newsrc[rescan_group_number])) {
    rescan_group_number++;
    return False;
  }

  if (! getgroup(Newsrc[rescan_group_number], &first, &last, &number, False)) {
    articleArrayResync(Newsrc[rescan_group_number], first, last, number);
    redrawNewsgroupTextWidget(Newsrc[rescan_group_number]->name, False);
    resetArticleNewsgroupsList();
  }

  if ((Newsrc[rescan_group_number] == PrefetchingGroup) ||
      (Newsrc[rescan_group_number] == PrefetchedGroup))
    resetPrefetch();
  else
    doPrefetch(0, 0, 0, 0);
    
  rescan_group_number++;
  return False;
}
    

/*
  Do a NEWGROUPS command to check for new groups, then start a
  background procedure to rescan subscribed groups in the .newsrc.
  */
void rescanBackground()
{
#ifdef DEBUG
  fprintf(stderr, "rescanBackground()\n");
#endif

  getactive(True);

  infoNow(RESCANNING_BACKGROUND_MSG);

  if (rescan_id)
    XtRemoveWorkProc(rescan_id);
  active_serial = active_read;
  rescan_group_number = 0;
  rescan_id = XtAppAddWorkProc(TopContext, rescanWorkProc, 0);
}

void cancelRescanBackground()
{
#ifdef DEBUG
  fprintf(stderr, "cancelRescanBackground()\n");
#endif

  if (rescan_id) {
    XtRemoveWorkProc(rescan_id);
    rescan_id = 0;
  }
}

/*
 * get the active file again and grow the Newsrc array if necessary
 */
void rescanServer(
		  _ANSIDECL(Boolean,	force_list)
		  )
     _KNRDECL(Boolean,	force_list)
{
    cancelPrefetch();

    /* update the newsrc file */
    while (!updatenewsrc())
      ehErrorRetryXRN(ERROR_CANT_UPDATE_NEWSRC_MSG, True);

#if !defined(NNTP_REREADS_ACTIVE_FILE)
    stop_server();
    start_server();
#endif

    if (app_resources.cacheActive && !force_list) {
      rescanBackground();
      return;
    }

    getactive(False);
    if (! app_resources.fullNewsrc)
      getactive(True);

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
    struct article *art;
    art_num first, last;
    int count = 0;

    if (EMPTY_GROUP(newsgroup) ||
	((newsgroup->first == 0) && (newsgroup->last == 0)))
	return 0;

    for (art = artListFirst(newsgroup, &first, &last);
	 art; art = artStructNext(newsgroup, art, &first, &last)) {
	if (IS_UNREAD(art) && IS_AVAIL(art))
	    count += last - first + 1;
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
    struct article *art;
    art_num first, last;
    int count = 0;
  
    if (EMPTY_GROUP(newsgroup) ||
	((newsgroup->first == 0) && (newsgroup->last == 0)))
	return 0;
    
    for (art = artListFirst(newsgroup, &first, &last);
	 art; art = artStructNext(newsgroup, art, &first, &last)) {
	if (IS_AVAIL(art))
	    count += last - first + 1;
    }

    return count;
}


/*
  When this function returns false, it modifies the article list for
  the newsgroup as a side effect, so any outstanding article
  structures in the caller should be considered invalid.
  */
Boolean articleIsAvailable(newsgroup, i)
    struct newsgroup *newsgroup;
    art_num i;
{
    struct article *art;

    art = artStructGet(newsgroup, i, True);

    if (art->file && *art->file && !IS_ALL_HEADERS(art) && !IS_ROTATED(art) &&
	!IS_XLATED(art) &&
	(access(file_cache_file_name(FileCache, *art->file), R_OK) == 0)) {
      artStructSet(newsgroup, &art);
      return True;
    }

    CLEAR_SUBJECT(art);
    artStructSet(newsgroup, &art);

    (void) fillUpArray(newsgroup, i, i, False, False);

    art = artStructGet(newsgroup, i, False);
    if (art->subject)
      return True;
    else
      return False;
}


/*
  Find the first unread article, or the last article if unread_only is
  false and there are no unread articles, in a group and set the
  group's 'current' to it.  If "check_available" is true, actually
  contacts the NNTP server to verify that the article is available.
 
  Returns True if the group's current article was modified in a way
  which might require the group to be prefetched again, False
  otherwise.

  If the "finished" parameter is non-null, then this function will
  only make "max" attempts to find a current article by contacting the
  NNTP server.  If it succeeds, the parameter will be filled in True;
  otherwise, it'll be filled in False.
  */
static Boolean setCurrentArticle _ARGUMENTS((struct newsgroup *,
					     Boolean, Boolean,
					     Boolean *, int));

static Boolean setCurrentArticle(
				 _ANSIDECL(struct newsgroup *,	newsgroup),
				 _ANSIDECL(Boolean,		unread_only),
				 _ANSIDECL(Boolean,		check_available),
				 _ANSIDECL(Boolean *,		finished),
				 _ANSIDECL(int,			max)
				 )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(Boolean,			unread_only)
     _KNRDECL(Boolean,			check_available)
     _KNRDECL(Boolean *,		finished)
     _KNRDECL(int,			max)
{
    struct article *art;
    art_num i, start, avail = 0;
    art_num orig = newsgroup->current;

    if (app_resources.onlyShow > 0) {
	/* if the resource 'onlyShow' is > 0, then mark all but the last
	   'onlyShow' articles as read */
	for (start = newsgroup->last;
	     start >= newsgroup->first && (avail < app_resources.onlyShow);
	     start--) {
	    art = artStructGet(newsgroup, start, False);
	    if (IS_UNREAD(art) && IS_AVAIL(art))
		avail++;
	}
	start++;
    }
    else {
	start = newsgroup->first;
    }

    for (i = start; i<= newsgroup->last; i++) {
	art = artStructGet(newsgroup, i, False);
	if (IS_UNREAD(art) && IS_AVAIL(art)) {
	  if (!check_available || articleIsAvailable(newsgroup, i)) {
	    if (finished)
	      *finished = True;
	    newsgroup->current = i;
	    goto done;
	  }
	  else if (finished && ! --max) {
	    *finished = False;
	    goto done;
	  }
	}
    }

    if (unread_only) {
	newsgroup->current = newsgroup->last + 1;
    }
    else {
	for (i = newsgroup->last; i >= newsgroup->first; i--) {
	    art = artStructGet(newsgroup, i, False);
	    if (IS_AVAIL(art)) {
	      if (!check_available || articleIsAvailable(newsgroup, i)) {
		if (finished)
		  *finished = True;
		newsgroup->current = i;
		goto done;
	      }
	      else if (finished && ! --max) {
		*finished = False;
		goto done;
	      }
	    }
	}
	newsgroup->current = newsgroup->last + 1;
    }

  done:
    return ((orig != newsgroup->current) &&
	    (newsgroup->current <= newsgroup->last));
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
	if (IS_SUBSCRIBED(newsgroup) && watchingGroup(newsgroup->name))
	    count += unreadArticleCount(newsgroup);
    }
    return count;
}

/*
  Find the line in the specified newsgroup list (as returned by
  unreadGroups, below) closest in the newsrc file to the specified
  group, but not before it, and return the index of the first
  character on the line containing the found group.  If the specified
  group is actually in the newsgroup list, return the index of the
  first character on the line containing the specified group.

  When returning a line containing a newsgroup other than the one
  specified, copies the new group into group_name, after reallocating
  it so that it is just long enough to hold it.

  When returning 0 because no match could be found, makes group_name
  an empty string.
  */
int getNearbyNewsgroup(list, group_name)
    char *list;
    char **group_name;
{
    struct newsgroup *group;
    char *ptr;
    char *match_group = 0;

    if (! group_name)
	return 0;

    if (! *group_name) {
	*group_name = XtRealloc(*group_name, 1);
	**group_name = '\0';
    }

    if (! **group_name) {
	currentGroup(NEWSGROUP_MODE, list, group_name, 0);
	return 0;
    }

    if (! avl_lookup(NewsGroupTable, *group_name, (char **) &group)) {
	**group_name = '\0';
	return 0;
    }

    /*
      First, check the easy case -- the group is in the string.
      */
    for (ptr = list;
	 ptr && (ptr = strstr(ptr, group->name));
	 ptr = index(ptr, '\n')) {
	while ((*ptr != '\n') && (ptr > list))
	    ptr--;
	if (ptr > list)
	    ptr++;
	currentGroup(NEWSGROUP_MODE, ptr, &match_group, 0);
	if (! strcmp(group->name, match_group))
	    break;
    }

    if (ptr) {
	XtFree(match_group);
	return ptr - list;
    }

    /*
      Well, OK, that didn't work.  Algorithm: Figure out the position
      in the Newsrc array of each newsgroup in the newsgroup list in
      order, until we find one whose position is greater than or equal
      to the position of the group we're looking for.  Return that
      group's position in the list.

      If all the newsgroups in the list are earlier in the Newsrc than
      the one we're looking for, return the index of the beginning of
      the end of the list.
      */

    for (ptr = list;
	 ptr && *ptr;
	 ptr = (ptr = index(ptr, '\n')) ? (ptr + 1) : 0) {
	char *gptr;

	currentGroup(NEWSGROUP_MODE, ptr, &match_group, 0);
	if (! avl_lookup(NewsGroupTable, match_group, &gptr))
	    continue;
	if (((struct newsgroup *)gptr)->newsrc >= group->newsrc) {
	    XtFree(*group_name);
	    *group_name = match_group;
	    return ptr - list;
	}
    }

    XtFree(match_group);

    *group_name = XtRealloc(*group_name, 1);
    **group_name = '\0';

    if (ptr && *ptr)
	return ptr - list;

    /*
      This shouldn't ever really happen, because the loop above should
      terminate when it gets to the last line and realizes that
      there's nothing on it, i.e., ptr is true but *ptr is false.
      Just in case, though, we'll put a return here.
      */
    return 0;
}


/*
 * build and return a string of information about groups that need to be read
 */
char *unreadGroups(line_length, mode)
    int line_length;
    int mode;	/* 0 for unread groups, 1 for all subscribed to groups */
{
    static char *dummy = NULL;
    static int dummy_size = 0, padding_size = 0;
    struct newsgroup *newsgroup;
    ng_num i;
    int unread, j;
    int bytes = 0, subscribedGroups = 0;
    char *string, **ar;
    int newsgroup_length = line_length - NEWS_GROUP_LINE_CHARS;

    if (newsgroup_length < 0)
	newsgroup_length = 0;

    ar = ARRAYALLOC(char *, MaxGroupNumber);

    if (! dummy_size) {
      dummy_size = LINE_LENGTH;
      dummy = XtMalloc(dummy_size);
      padding_size = strlen(NEWSGROUPS_INDEX_MSG) + strlen(UNREAD_MSG) +
	strlen(NEWS_IN_MSG) + 10 /* for unread count */ + strlen(NOT_ONE_MSG) +
	10 /* for old count */ + 1 /* for null */;
    }

    for (i = 0; i < MaxGroupNumber; i++) {
	newsgroup = Newsrc[i];

	if (IS_SUBSCRIBED(newsgroup) &&
	   (((unread = unreadArticleCount(newsgroup)) > 0) || mode)) {
	    int total = totalArticleCount(newsgroup);
	    while (dummy_size < padding_size + 
		   MAX(newsgroup_length, strlen(newsgroup->name))) {
	      dummy_size *= 2;
	      dummy = XtRealloc(dummy, dummy_size);
	    }
	    (void) sprintf(dummy, NEWSGROUPS_INDEX_MSG,
	                   (unread > 0 ? UNREAD_MSG : ""),
	                   (total > 0 ? NEWS_IN_MSG : ""),
			   -newsgroup_length, newsgroup->name, unread,
			   ((unread != 1) ? NOT_ONE_MSG : " "),
			   total - unread);

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
  Change a group's status so that it's no longer considered new.
  */
void clearNew(group)
char *group;
{
  struct newsgroup *newsgroup;
  
  if (! avl_lookup(NewsGroupTable, group, (char **) &newsgroup))
    return;

  CLEAR_NEW(newsgroup);
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
      ehErrorExitXRN(ERROR_OUT_OF_MEM_MSG);
    }

    while (avl_gen(gen, &key, &value)) {
	struct newsgroup *newsgroup = (struct newsgroup *) value;

	if (IS_NEW(newsgroup)) {
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

void freePrefetchedGroupArticle()
{
#ifdef DEBUG
    fprintf(stderr, "freePrefetchedGroupArticle()\n");
#endif

    if (PrefetchedGroup) {
	struct article *art = artStructGet(PrefetchedGroup,
					   PrefetchedGroup->current, True);
	if (IS_FETCHED(art))
	    CLEAR_FILE(art);
	artStructSet(PrefetchedGroup, &art);
    }

#ifdef DEBUG
    fprintf(stderr, "freePrefetchedGroupArticle() done\n");
#endif

    return;
}

static void adjustPrefetchChunk _ARGUMENTS((long, long, int *));

static void adjustPrefetchChunk(actual_time, desired_time, chunk_size)
    long actual_time, desired_time;
    int *chunk_size;
{
#ifdef DEBUG
    fprintf(stderr, "adjustPrefetchChunk(%ld, %ld, %d)\n", actual_time,
	    desired_time, *chunk_size);
#endif

    if (! desired_time)
	goto done;
    if (! actual_time) {
	*chunk_size *= 2;
    }
    else {
	*chunk_size = *chunk_size * desired_time / actual_time;
	if (*chunk_size < PREFETCH_MIN_CHUNK)
	    *chunk_size = PREFETCH_MIN_CHUNK;
    }

  done:
#ifdef DEBUG
    fprintf(stderr, "adjustPrefetchChunk(%ld, %ld, %d) done\n",
	    actual_time, desired_time, *chunk_size);
#endif
    return;
}


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
	newsgroup ? newsgroup->name : "NULL");
#endif /* DEBUG */

    if (newsgroup) {
	struct article *original, copy;
	art_num artnum;

	groupSnapshotFree(newsgroup);

	if (newsgroup->thread_table) {
	  hash_table_destroy(newsgroup->thread_table);
	  newsgroup->thread_table = 0;
	}

	if ((newsgroup->last == 0) || EMPTY_GROUP(newsgroup)) {
	    return;
	}

	for (artnum = newsgroup->first; artnum <= newsgroup->last; artnum++) {
	    original = artStructGet(newsgroup, artnum, False);
	    copy = *original;

	    CLEAR_ALL_NO_FREE(&copy);

	    SET_UNMARKED(&copy);
	    SET_STRIPPED_HEADERS(&copy);
	    SET_UNROTATED(&copy);
	    SET_UNXLATED(&copy);
	    SET_UNLISTED(&copy);

	    artStructReplace(newsgroup, &original, &copy, artnum);
	}
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
    struct article *art, copy;
    int i;

    /*
     * if there are actually no articles in the group, free up the
     * article array and set the first/last values
     */
       
    if (number == 0) {
	artListFree(newsgroup);
	newsgroup->first = newsgroup->last + 1;
	return;
    }
    
    /* refuse to resync if last == 0 */
    if (last == 0) {
	return;
    }

    if (first > last) {
	/* all articles have been removed */

	artListFree(newsgroup);
	newsgroup->first = newsgroup->last + 1;
	return;
    }

    /* don't allow first or last to go backwards!!! */
    if (first < newsgroup->first)
	first = newsgroup->first;
    if (last < newsgroup->last)
	last = newsgroup->last;

    if (! artListFirst(newsgroup, 0, 0)) {
	newsgroup->first = first;
	newsgroup->last = last;
	artListSet(newsgroup);
    }
    else {
	copy.status = ART_CLEAR_READ;

	CLEAR_ALL_NO_FREE(&copy);

	SET_UNAVAIL(&copy);
	for (i = newsgroup->first; i < first; i++) {
	    art = artStructGet(newsgroup, i, False);
	    artStructReplace(newsgroup, &art, &copy, i);
	}

	copy.status = ART_CLEAR;
	for (i = MAX(newsgroup->last + 1, first); i <= last; i++) {
	    art = artStructGet(newsgroup, i, False);
	    artStructReplace(newsgroup, &art, &copy, i);
	}

	newsgroup->first = first;
	newsgroup->last = last;
    }
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
    (void) sprintf(&buffer[i], "/%s", app_resources.killFileName);

    return buffer;
}

char * globalKillFile()
{
    static char buffer[BUFFER_SIZE];

    if (!createNewsDir()) {
	return NIL(char);
    }
    (void) sprintf(buffer, "%s/%s", app_resources.expandedSaveDir,
		   app_resources.killFileName);
    return buffer;
}


/*
  Convert a string to a regular expression matching that string.  The
  resulting regular expression is *not* anchored, i.e., it will match
  any string of which the string passed into it is a subsctring.

  The returned string is static memory which is not valid after the
  next call to this function.
  */
char *stringToRegexp(input, max_length)
    char *input;
{
    static char output[BUFFER_SIZE];
    char *inptr, *outptr;

    max_length -= 2; /* to make room for braces */
    
    /* quote or get rid of all magic characters */
    for (inptr = input, outptr = output;
	 (*inptr && (outptr - output < sizeof(output) - 4));
	 inptr++, outptr++) {
	switch (*inptr) {
	case '/':
	case '\\':
	case '[':
	/* Theoretically, ']' shouldn't be special when it isn't
	   preceded by '[', but it doesn't hurt to be cautious. */
	case ']':
	case '.':
	case '^':
	case '*':
	case '$':
	    *outptr++ = '\\';
	    *outptr = *inptr;
	    break;
	/*
	  Some characters can't be backslashed, because they are
	  literal when backslashed in some regular expression
	  libraries, and when not backslashed in others.  We could do
	  different things here based on whether SYSV_REGEX is
	  defined, but that seems like a bad idea, because it means
	  that the KILL file will have a different meaning based on
	  what kind of system it's read on.  We want KILL file regexps
	  to be portable.

	  Some day, we'll use the GNU regexp library or something on
	  all platforms, and we'll be able to do the right thing for
	  all characters.  In the meantime, we'll check for these
	  special characters by putting them inside square braces,
	  which means the same thing in all of the regular expression
	  packages.
	*/
	    /*
	      The POSIX regexp functions apparently treat \?, \+,
	      \{, and \} specially.
	      */
	case '{':
	case '}':
	case '?':
	case '+':
	case '(':
	/* On some systems, it's an error for there to be a ')' with
	   no matching '(' before it. */
	case ')':
	    *outptr++ = '[';
	    *outptr++ = *inptr;
	    *outptr = ']';
	    break;
	default:
	    *outptr = *inptr;
	    break;
	}
	if (outptr - output >= max_length)
	  break;
    }
    *outptr = '\0';
    return output;
}


/*
 * kill all subjects in the newsgroup that match the kill
 * orders in fp.
 *
 * Returns True if it gets to the end of the group, False otherwise.
 */
/* XXX  THIS ROUTINE REALLY NEEDS TO BE CLEANED UP */
static Boolean killArticles _ARGUMENTS((struct newsgroup *, int));

static Boolean killArticles(newsgroup, max)
    struct newsgroup *newsgroup;
    int max;
{
    struct article *art;
    art_num i, start, last;
    char *subject, *from, *newsgroups, *date, *id, *references, *xref;
    int scount = 0, kcount = 0, mcount = 0;
    int mesg_name;
    kill_entry *entry = 0;
    kill_file *kf;
    int cur_mode = KILL_LOCAL;

    kf = (kill_file *)newsgroup->kill_file;

    start = MAX(newsgroup->current, MIN(kf->thru + 1, newsgroup->last));
    do {
	art = artStructGet(newsgroup, start, False);
	if (IS_KILLED(art))
	    start++;
    } while (IS_KILLED(art) && (start <= newsgroup->last));

    if (max)
	last = MIN(newsgroup->last, start + max - 1);
    else
	last = newsgroup->last;

    for (i = start; i <= last; i++) {
	art = artStructGet(newsgroup, i, False);
	if (IS_KILLED(art) || IS_UNAVAIL(art))
	    last = MIN(newsgroup->last, last + 1);
    }

    if (kf->count)
      kf->flags |= KF_CHANGED;

    kf->thru = MAX(kf->thru, MIN(start, last));

    if (start > last)
	return True;

    kf->thru = last;

    while (1) {
      if (! (entry = kill_file_iter(newsgroup, cur_mode, entry))) {
	if (cur_mode != KILL_LOCAL)
	  break;
	cur_mode = KILL_GLOBAL;
	continue;
      }

      if (entry->type != KILL_ENTRY)
	continue;

      mesg_name = newMesgPaneName();

      for (i = start; i <= last; i++) {
	int field_count = 0;

	art = artStructGet(newsgroup, i, False);

	/* short cut */
	if (IS_KILLED(art) || IS_UNAVAIL(art) ||
	    ((entry->entry.action_flags & KILL_JUNK) && IS_READ(art)) ||
	    ((entry->entry.action_flags & KILL_MARK) && IS_UNREAD(art)))
	  continue;

#define CHECK_FIELD(flag, name) \
	if (entry->entry.check_flags & flag) { \
	  if ((name = art->name)) \
	    field_count++; \
	} \
	else \
	  name = 0;

	CHECK_FIELD(KILL_SUBJECT, subject);
	CHECK_FIELD(KILL_AUTHOR, from);
	CHECK_FIELD(KILL_NEWSGROUPS, newsgroups);
	CHECK_FIELD(KILL_DATE, date);
	CHECK_FIELD(KILL_ID, id);
	CHECK_FIELD(KILL_REFERENCES, references);
	CHECK_FIELD(KILL_XREF, xref);

	if (field_count) {
	  struct article copy;
	  unsigned char changed = FALSE;

	  copy = *art;

#ifdef POSIX_REGEX
# define KILL_MATCH(a)	((a) && (! regexec(&entry->entry.reStruct, (a), 0, 0, 0)))
#else
# ifdef SYSV_REGEX
#  define KILL_MATCH(a)	((a) && (regex(entry->entry.reStruct, (a)) != NULL))
# else
#  define KILL_MATCH(a)	((a) && re_exec(a))
# endif
#endif

	  if (KILL_MATCH(subject) || KILL_MATCH(from) ||
	      KILL_MATCH(date) || KILL_MATCH(newsgroups) ||
	      KILL_MATCH(id) || KILL_MATCH(references) ||
	      KILL_MATCH(xref)) {
	    kill_update_last_used(kf, entry);

	    switch (entry->entry.action_flags) {
	    case KILL_JUNK:
	      SET_READ(&copy);
	      if (index(app_resources.verboseKill, 'j')) {
		mesgPane(XRN_INFO, mesg_name, KILL_KILLED_MSG,
			 art->subject);
	      }
	      kcount++;
	      changed = TRUE;
	      break;

	    case KILL_MARK:
	      SET_UNREAD(&copy);
	      if (index(app_resources.verboseKill, 'm')) {
		mesgPane(XRN_INFO, mesg_name, KILL_UNREAD_MSG,
			 art->subject);
	      }
	      mcount++;
	      changed = TRUE;
	      break;

	    case KILL_SAVE:
	      (void) saveArticle(NIL(char), newsgroup, i, False,
				 False);
	      if (index(app_resources.verboseKill, 's')) {
		mesgPane(XRN_INFO, mesg_name, KILL_SAVED_MSG,
			 art->subject);
	      }
	      scount++;
	      changed = TRUE;
	      break;
	    default:
	      mesgPane(XRN_SERIOUS, mesg_name, UNKNOWN_FUNC_RESPONSE_MSG,
		       entry->entry.action_flags, "kill_file_iter(action_flags)",
		       "killArticles");
	      break;
	    }
	  }

	  if (changed)
	    artStructReplace(newsgroup, &art, &copy, i);
	}
      }
    }

    for (i = start; i <= last; i++) {
	art = artStructGet(newsgroup, i, True);
	SET_KILLED(art);
	artStructSet(newsgroup, &art);
    }

    mesg_name = newMesgPaneName();

#define printcount(c,cmd,m) \
    if (c && index(app_resources.verboseKill, cmd)) { \
	mesgPane(XRN_INFO, mesg_name, m, c, ((c==1) ? "" : NOT_ONE_MSG), \
		 newsgroup->name); \
    }

    printcount(kcount, 'j', COUNT_KILLED_MSG);
    printcount(mcount, 'm', COUNT_UNREAD_MSG);
    printcount(scount, 's', COUNT_SAVED_MSG);

#undef printcount

    if (last == newsgroup->last)
	return True;
    else
	return False;
}

    
/*
 * mark articles as read if in the kill files
 */
static Boolean checkKillFiles _ARGUMENTS((struct newsgroup *, Boolean));
    
static Boolean checkKillFiles(
			      _ANSIDECL(struct newsgroup *,	newsgroup),
			      _ANSIDECL(Boolean,		prefetching)
			      )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(Boolean,			prefetching)
{
    static int chunk_size;
    long start WALL(= 0), end;
    static struct newsgroup *last_group = 0;
    char dummy[LABEL_SIZE];
    Boolean finished = True;
    unsigned char fetched = newsgroup->fetch;

#ifdef DEBUG
    fprintf(stderr, "checkKillFiles(%s, %d)\n", newsgroup->name, prefetching);
#endif

    /*
      Make sure the local and global kill files for the group are read in.
      */
    read_local_kill_file(newsgroup);
    read_global_kill_file(newsgroup);

    if (fetched != newsgroup->fetch) {
      finished = False;
      goto done;
    }

    if (! has_kill_files(newsgroup))
      goto done;

    (void) sprintf(dummy, PROCESS_KILL_FOR_MSG, newsgroup->name);
    maybeInfoNow(dummy);

    /*
      Reset the chunk size for each newsgroup, because different
      newsgroups have different KILL files and therefore will take
      different amounts of time to kill.
      */
    if (last_group != newsgroup) {
	chunk_size = PREFETCH_CHUNK;
	last_group = newsgroup;
    }
    if (prefetching)
	start = time(0);
    
    finished = killArticles(newsgroup, prefetching ? chunk_size : 0);

    if (prefetching && !finished) {
	end = time(0);
	adjustPrefetchChunk(end - start, PREFETCH_CHUNK_TIME, &chunk_size);
    }

    if (finished) {
	(void) sprintf(dummy, PROCESS_KILL_FOR_MSG, newsgroup->name);
	(void) strcat(dummy, " ");
	(void) strcat(dummy, DONE_MSG);
	INFO(dummy);
    }

done:
#ifdef DEBUG
    fprintf(stderr, "checkKillFiles(%s, %d) = %d\n",
	    newsgroup->name, prefetching, finished);
#endif
    return finished;

}


void finishPrefetch()
{
    struct newsgroup *our_group = PrefetchingGroup;

#ifdef DEBUG
    fprintf(stderr, "finishPrefetch()\n");
#endif

    FinishingPrefetch = True;
    while (our_group == PrefetchingGroup)
	(*PrefetchingProc)(PrefetchingGroup);
    FinishingPrefetch = False;

#ifdef DEBUG
    fprintf(stderr, "finishPrefetch() done\n");
#endif
}

void resetPrefetch()
{
  struct newsgroup *newsgroup = 0;

  if (PrefetchedGroup) {
    newsgroup = PrefetchedGroup;
    PrefetchedGroup = 0;
  }
  else if (PrefetchingGroup) {
    newsgroup = PrefetchingGroup;
    PrefetchingGroup = 0;
    XtRemoveWorkProc(prefetch_id);
  }

  if (newsgroup)
    prefetchGroup(newsgroup->name);

}
    
void cancelPrefetch()
{
    char msg[LABEL_SIZE];

#ifdef DEBUG
    fprintf(stderr, "cancelPrefetch()\n");
#endif

    if (PrefetchedGroup) {
	freePrefetchedGroupArticle();
	releaseNewsgroupResources(PrefetchedGroup);
    }

    if (PrefetchingGroup) {
	releaseNewsgroupResources(PrefetchingGroup);
	(void) sprintf(msg, PREFETCHING_MSG, PrefetchingGroup->name);
	(void) strcat(msg, " ");
	(void) strcat(msg, ABORTED_MSG);
	INFO(msg);
    }

    PrefetchedGroup = 0;
    PrefetchingGroup = 0;

#ifdef DEBUG
    fprintf(stderr, "cancelPrefetch() done\n");
#endif
}

static int art_num_compare _ARGUMENTS((void *, void *));

static int art_num_compare(key1, key2)
     void *key1, *key2;
{
  return (art_num)key1 - (art_num)key2;
}

static char *backTo _ARGUMENTS((char *, char *, char));

static char *backTo(
		    _ANSIDECL(char *,	start),
		    _ANSIDECL(char *,	ptr),
		    _ANSIDECL(char,	character)
		    )
     _KNRDECL(char *,	start)
     _KNRDECL(char *,	ptr)
     _KNRDECL(char,	character)
{
  for (ptr--; ptr >= start; ptr--)
    if (*ptr == character)
      return ptr;
  return 0;
}

static Boolean threadIncremental _ARGUMENTS((struct newsgroup *,
					     int *,
					     art_num, art_num));

static Boolean threadIncremental(
				 _ANSIDECL(struct newsgroup *,	newsgroup),
				 _ANSIDECL(int *,		stage),
				 _ANSIDECL(art_num,		first),
				 _ANSIDECL(art_num,		last)
				 )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(int *,			stage)
     _KNRDECL(art_num,			first)
     _KNRDECL(art_num,			last)
{
  static int chunk_size = PREFETCH_CHUNK;
  time_t start_time WALL (= 0), end_time;
  art_num i, done_count = 0;
  char dummy[LABEL_SIZE];

  if (! (newsgroup->fetch & FETCH_THREADS))
    return True;

  if (stage) {
    if (*stage != PREFETCH_THREAD_STAGE)
      return True;
    start_time = time(0);
  }

  if (! newsgroup->thread_table) {
    (void) sprintf(dummy, THREADING_FOR_MSG, newsgroup->name);
    maybeInfoNow(dummy);

    newsgroup->thread_table = hash_table_create(last - first + 1,
						hash_string_calc,
						hash_string_compare,
						art_num_compare,
						0, 0);

    for (i = first; i <= last; i++) {
      struct article *art = artStructGet(newsgroup, i, False);
      if (art->id && IS_LISTED(art)) {
	int ret = hash_table_insert(newsgroup->thread_table,
				    (void *)art->id, (void *)i, 0);
	assert(ret);
      }
    }

    if (stage)
      return False;
  }

  
  for (i = first; (i <= last) && (! stage || (done_count < chunk_size)); i++) {
    struct article *art = artStructGet(newsgroup, i, True);
    char *references, *ptr;
    art_num parent;
    struct article *parent_struct;

    if (art->parent || !IS_LISTED(art))
      continue;

    if (! (art->references && *art->references)) {
      art->parent = (art_num)-1;
      continue;
    }

    done_count++;

    references = XtNewString(art->references);

    for (ptr = strrchr(references, '>'); ptr;
	 ptr = backTo(references, ptr, '>')) {
      *(ptr + 1) = '\0';
      ptr = backTo(references, ptr, '<');

      if (! ptr)
	break;

      if ((parent = (art_num)hash_table_retrieve(newsgroup->thread_table,
						 (void *)ptr, 0)) !=
	  (art_num)HASH_NO_VALUE) {
	art->parent = parent;
	parent_struct = artStructGet(newsgroup, parent, True);
	artStructAddChild(parent_struct, i);
	artStructSet(newsgroup, &parent_struct);
	break;
      }
    }

    XtFree(references);

    if (! art->parent)
      art->parent = (art_num)-1;
    artStructSet(newsgroup, &art);
  }

  if (i > last) {
    for (i = first; i <= last; i++) {
      struct article *art = artStructGet(newsgroup, i, False);
      if (art->parent == (art_num)-1) {
	struct article copy;
	copy = *art;
	CLEAR_PARENT(&copy);
	artStructReplace(newsgroup, &art, &copy, i);
      }
    }
    hash_table_destroy(newsgroup->thread_table);
    newsgroup->thread_table = 0;
    (void) sprintf(dummy, THREADING_FOR_MSG, newsgroup->name);
    (void) strcat(dummy, " ");
    (void) strcat(dummy, DONE_MSG);
    INFO(dummy);

    return True;
  }

  end_time = time(0);
  adjustPrefetchChunk(end_time - start_time,
		      PREFETCH_CHUNK_TIME, &chunk_size);
  return False;
}
  
static void checkThreading _ARGUMENTS((struct newsgroup *, art_num));

static void checkThreading(
			   _ANSIDECL(struct newsgroup *,	newsgroup),
			   _ANSIDECL(art_num,			first)
			   )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			first)
{
  art_num i;
  struct article *art, *parent, copy;

  for (i = first; i <= newsgroup->last; i++) {
    art = artStructGet(newsgroup, i, False);
    if (art->parent) {
      parent = artStructGet(newsgroup, art->parent, False);
      if ((art->parent < first) || !IS_LISTED(parent) || !IS_LISTED(art)) {
	copy = *parent;
	artStructRemoveChild(&copy, i);
	artStructReplace(newsgroup, &parent, &copy, art->parent);
	copy = *art;
	CLEAR_PARENT(&copy);
	artStructReplace(newsgroup, &art, &copy, i);
      }
    }
  }
}

  
static void rethreadGroup _ARGUMENTS((struct newsgroup *, art_num,
				      Boolean));

static void rethreadGroup(
			  _ANSIDECL(struct newsgroup *,	newsgroup),
			  _ANSIDECL(art_num,		art),
			  _ANSIDECL(Boolean,		rethread)
			  )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			art)
     _KNRDECL(Boolean,			rethread)
{
  art_num i;

  if (rethread) {
    /*
      We're generating a new list after "List Old" or "Goto article",
      which means that articles which weren't displayed before are
      displayed now, so we have to reset everything.

      Otherwise, the only thing that might have happened is some
      articles getting marked read as a result of cross-posts after
      the group was already prefetched, and those will be detected by
      checkThreading(), so we don't have to worry about them here.
      */
    for (i = art; i <= newsgroup->last; i++) {
      struct article *art = artStructGet(newsgroup, i, True);
      CLEAR_CHILDREN(art);
      CLEAR_PARENT(art);
      artStructSet(newsgroup, &art);
    }
  }
  else {
    checkThreading(newsgroup, art);
  }

  (void) threadIncremental(newsgroup, 0, art, newsgroup->last);
}
     
static Boolean fetchHeadersIncremental _ARGUMENTS((struct newsgroup *,
						   int *, art_num, art_num,
						   Boolean, Boolean));

static Boolean fetchHeadersIncremental(
				       _ANSIDECL(struct newsgroup *,	newsgroup),
				       _ANSIDECL(int *,			stage),
				       _ANSIDECL(art_num,		first),
				       _ANSIDECL(art_num,		last),
				       _ANSIDECL(Boolean,		unread_only),
				       _ANSIDECL(Boolean,		kill_files)
				       )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(int *,			stage)
     _KNRDECL(art_num,			first)
     _KNRDECL(art_num,			last)
     _KNRDECL(Boolean,			unread_only)
     _KNRDECL(Boolean,			kill_files)
{
    static int chunk_size = PREFETCH_CHUNK;
    time_t start_time WALL (= 0), end_time;
    Boolean finished = True;

    if (stage) {
      if ((*stage < PREFETCH_START_HEADERS_STAGE) ||
	  (*stage > PREFETCH_LAST_HEADERS_STAGE))
	return True;
      start_time = time(0);
    }

    if (! stage || *stage == PREFETCH_SUBJECT_STAGE)
      finished = getsubjectlist(newsgroup, first, last,
				unread_only,
				(stage && ! FinishingPrefetch)
				? chunk_size : 0);
    if (! stage || *stage == PREFETCH_AUTHOR_STAGE)
      finished = getauthorlist(newsgroup, first, last,
			       unread_only,
			       (stage && ! FinishingPrefetch)
			       ? chunk_size : 0);
    if (! stage || *stage == PREFETCH_LINES_STAGE)
      finished = getlineslist(newsgroup, first, last,
			      unread_only,
			      (stage && ! FinishingPrefetch)
			      ? chunk_size : 0);
    if ((newsgroup->fetch & FETCH_NEWSGROUPS) && kill_files &&
	(! stage || *stage == PREFETCH_NEWSGROUPS_STAGE))
      finished = getnewsgroupslist(newsgroup, first, last,
				   unread_only,
				   (stage && ! FinishingPrefetch)
				   ? chunk_size : 0);
    if ((newsgroup->fetch & FETCH_DATES) &&
	(! stage || *stage == PREFETCH_DATE_STAGE))
      finished = getdatelist(newsgroup, first, last,
			     unread_only,
			     (stage && ! FinishingPrefetch)
			     ? chunk_size : 0);
    if ((newsgroup->fetch & FETCH_IDS) &&
	(! stage || *stage == PREFETCH_IDS_STAGE))
      finished = getidlist(newsgroup, first, last,
			   unread_only,
			   (stage && ! FinishingPrefetch)
			   ? chunk_size : 0);
    if ((newsgroup->fetch & FETCH_XREF) &&
	(! stage || *stage == PREFETCH_XREF_STAGE))
      finished = getxreflist(newsgroup, first, last,
			     unread_only,
			     (stage && ! FinishingPrefetch)
			     ? chunk_size : 0);
    if ((newsgroup->fetch & FETCH_REFS) &&
	(! stage || *stage == PREFETCH_REFS_STAGE))
      finished = getreflist(newsgroup, first, last,
			    unread_only,
			    (stage && ! FinishingPrefetch)
			    ? chunk_size : 0);
    if (! finished) { /* that means it was a full chunk */
      end_time = time(0);
      adjustPrefetchChunk(end_time - start_time,
			  PREFETCH_CHUNK_TIME, &chunk_size);
    }

    return finished;
}

/*
 * The reason this is divided into "stages" is so that it can be used
 * in a toolkit work procedure.  The stages allow the work to be
 * divided up so a single work procedure invocation doesn't take too long.
 */
static Boolean setUpGroupIncremental _ARGUMENTS((struct newsgroup *, int *,
						 Boolean, Boolean,
						 Boolean, Boolean,
						 art_num, art_num));

static Boolean setUpGroupIncremental(
				     _ANSIDECL(struct newsgroup *,	newsgroup),
				     _ANSIDECL(int *,			stage),
				     _ANSIDECL(Boolean,			update_last),
				     _ANSIDECL(Boolean,			kill_files),
				     _ANSIDECL(Boolean,			unread_only),
				     _ANSIDECL(Boolean,			threading),
				     _ANSIDECL(art_num,			first),
				     _ANSIDECL(art_num,			last)
				     )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(int *,			stage)
     _KNRDECL(Boolean,			update_last)
     _KNRDECL(Boolean,			kill_files)
     _KNRDECL(Boolean,			unread_only)
     _KNRDECL(Boolean,			threading)
     _KNRDECL(art_num,			first)
     _KNRDECL(art_num,			last)
{
    Boolean finished = True;
    static int chunk_size = PREFETCH_CHUNK;
    struct article *art, copy;
    art_num i;

#ifdef DEBUG
    fprintf(stderr, "setUpGroupIncremental(%s, %d, %d, %d, %d)\n",
	    newsgroup->name, stage ? *stage : 0, update_last, kill_files,
	    unread_only);
#endif

    if (PrefetchingGroup && !stage) {
	if ((PrefetchingGroup == newsgroup) && kill_files && unread_only)
	    finishPrefetch();
	else
	    cancelPrefetch();
    }

    if (PrefetchedGroup && (newsgroup != PrefetchedGroup)) {
	cancelPrefetch();
    }

    if (! PrefetchedGroup) {
	if (! stage || *stage == PREFETCH_GETGROUP_STAGE) {
	    art_num first, last;
	    int number;
	   /*
	    * Update our idea of what the first article in the group
	    * is.  This is necessary because sites that don't
	    * regularly renumber their active files will report a
	    * different first article number for a group in the
	    * response to the LIST command than they will in response
	    * to the GROUP command, and the first article number in
	    * response to the latter command is, in fact, the correct
	    * one.
	    *
	    * If we don't use the first article number returned by the
	    * GROUP command when it is higher than the first article
	    * number returned by the LIST command, then we end up with
	    * two problems: (1) article arrays are much bigger than
	    * they need to be; (2) when the user clicks on "List Old"
	    * in a newsgroup, XRN will have to try to retrieve a large
	    * number of articles before actually finding the first
	    * article in the group.
	    *
	    * Also, update our idea of what the last article in the
	    * group is, if the update_last argument to setUpGroup is
	    * true.
	    */
	    (void) getgroup(newsgroup, &first, &last, &number,
			    stage ? False : True);
	    articleArrayResync(newsgroup, first,
			       update_last ? last : newsgroup->last, number);
	}
	if (! EMPTY_GROUP(newsgroup)) {
	    art_num my_first = first, my_last = last;

	    if (! stage || *stage == PREFETCH_SETCURRENT_STAGE)
	      (void) setCurrentArticle(newsgroup, unread_only, False,
				       (stage && ! FinishingPrefetch)
				       ? &finished : 0, chunk_size);

	    if (! my_first)
	      my_first = newsgroup->current;
	    if (! my_last)
	      my_last = newsgroup->last;

	    if ((! stage) ||
		((*stage >= PREFETCH_START_HEADERS_STAGE) &&
		 (*stage <= PREFETCH_LAST_HEADERS_STAGE)))
	      finished = fetchHeadersIncremental(newsgroup, stage,
						 my_first, my_last,
						 unread_only,
						 kill_files);

	    if (! stage || *stage == PREFETCH_KILL_STAGE) {
	        if ((app_resources.killFiles == TRUE) && kill_files) {
		  unsigned char fetched = newsgroup->fetch;

		  finished = checkKillFiles(newsgroup,
					    (stage && ! FinishingPrefetch)
					    ? True : False);

		  if (! finished || newsgroup->fetch != fetched) {
		    int i;

		    if (! stage)
		      return setUpGroupIncremental(newsgroup, stage,
						   update_last, kill_files,
						   unread_only, threading,
						   first, last);

		    for (i = PREFETCH_START_OPTIONAL_STAGE;
			 i <= PREFETCH_LAST_OPTIONAL_STAGE;
			 i++)
		      if ((PREFETCH_FIELD_BIT(i) & fetched) !=
			  (PREFETCH_FIELD_BIT(i) & newsgroup->fetch)) {
			*stage = i;
			finished = False;
			break;
		      }
		  }
		}
		if (finished) {
		  if (setCurrentArticle(newsgroup, unread_only, True, 0, 0)) {
		    if (! stage)
		      return setUpGroupIncremental(newsgroup, stage, update_last,
						   kill_files, unread_only,
						   threading,
						   first, last);
		    else {
		      *stage = PREFETCH_SETCURRENT_STAGE;
		      finished = False;
		    }
		  }
		  else if ((app_resources.onlyShow > 0) &&
			   app_resources.discardOld) {
		    for (i = newsgroup->first; i < newsgroup->current; i++) {
		      art = artStructGet(newsgroup, i, False);
		      if (IS_UNREAD(art)) {
			copy = *art;
			SET_READ(&copy);
			artStructReplace(newsgroup, &art, &copy, i);
		      }
		    }
		  }
		}
	    }
	    if ((! stage || *stage == PREFETCH_THREAD_STAGE) && threading)
	      finished = threadIncremental(newsgroup, stage, my_first, my_last);
	}
    }

    PrefetchedGroup = 0;

#ifndef NO_IMMEDIATE_WORK_PROC_PREFETCH
    if (! stage)
	prefetchNextGroup(newsgroup);
#endif

#ifdef DEBUG
    fprintf(stderr, "setUpGroupIncremental(%s, %d, %d, %d, %d) = %d\n",
	    newsgroup->name, stage ? *stage : 0, update_last, kill_files,
	    unread_only, finished);
#endif
    return finished;
}

static void setUpGroup _ARGUMENTS((struct newsgroup *, Boolean,
				   Boolean, Boolean, Boolean));

static void setUpGroup(
		       _ANSIDECL(struct newsgroup *,	newsgroup),
		       _ANSIDECL(Boolean,		update_last),
		       _ANSIDECL(Boolean,		kill_files),
		       _ANSIDECL(Boolean,		unread_only),
		       _ANSIDECL(Boolean,		threading)
		       )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(Boolean,			update_last)
     _KNRDECL(Boolean,			kill_files)
     _KNRDECL(Boolean,			unread_only)
     _KNRDECL(Boolean,			threading)
{
#ifdef DEBUG
    fprintf(stderr, "setUpGroup(%s, %d, %d, %d)\n", newsgroup->name,
	    update_last, kill_files, unread_only);
#endif
    (void) setUpGroupIncremental(newsgroup, 0, update_last,
				 kill_files, unread_only, threading, 0, 0);
#ifdef DEBUG
    fprintf(stderr, "setUpGroup(%s, %d, %d, %d) done\n", newsgroup->name,
	    update_last, kill_files, unread_only);
#endif
}


static struct newsgroup * findNewsGroupMatch _ARGUMENTS((char *));

static struct newsgroup * findNewsGroupMatch(name)
    char *name;
{
#ifdef POSIX_REGEX
    regex_t reStruct;
#else
    static char *reRet;
#endif
    int i;

    /* no single character queries -- too confusing */
    if (strlen(name) == 1) {
	return NIL(struct newsgroup);
    }

#ifdef POSIX_REGEX
    if (regcomp(&reStruct, name, REG_NOSUB))
	return 0;
#else
# ifdef SYSV_REGEX
    if ((reRet = regcmp(name, NULL)) == NULL) {
	return NIL(struct newsgroup);
    }
# else
    if ((reRet = re_comp(name)) != NULL) {
	return NIL(struct newsgroup);
    }
# endif
#endif

    for (i = 0; i < MaxGroupNumber; i++) {
	struct newsgroup *newsgroup = Newsrc[i];
#ifdef POSIX_REGEX
	if (! regexec(&reStruct, newsgroup->name, 0, 0, 0)) {
	    regfree(&reStruct);
	    return newsgroup;
	}
#else
# ifdef SYSV_REGEX
	if (regex(reRet, newsgroup->name) != NULL) {
	    FREE(reRet);
	    return newsgroup;
	}
# else
	if (re_exec(newsgroup->name)) {
	    return newsgroup;
	}
# endif
#endif
    }

#ifdef POSIX_REGEX
    regfree(&reStruct);
#else
# ifdef SYSV_REGEX
    free(reRet);
# endif
#endif

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
    struct article *art, copy;
    struct newsgroup *newsgroup = CurrentGroup;
    art_num artnum;

    artListSet(newsgroup);
    for (artnum = newsgroup->first; artnum <= newsgroup->last; artnum++) {
	art = artStructGet(newsgroup, artnum, False);
	copy = *art;
	if (IS_UNMARKED(art)) {
	    SET_READ(&copy);
	} else {
	    SET_UNREAD(&copy);
	    SET_UNMARKED(&copy);
	}
	artStructReplace(newsgroup, &art, &copy, artnum);
    }
    return;
}


/*
 * subscribe to a specified newsgroup
 *
 *   returns: void
 *
 */
static void subscribe_group _ARGUMENTS((struct newsgroup *));

static void subscribe_group(newsgroup)
    struct newsgroup *newsgroup;
{
    if (!IS_SUBSCRIBED(newsgroup)) {
	art_num first, last;
	int number;

	if (IS_NOENTRY(newsgroup))
	    addToNewsrcEnd(newsgroup->name, SUBSCRIBE);

	/*
	  Update the first and last article numbers for the newsgroup,
	  since "Rescan" doesn't update groups we're not subscribed to.
	  */
	if (! getgroup(newsgroup, &first, &last, &number, True)) {
	    SET_SUB(newsgroup);
	    articleArrayResync(newsgroup, first, last, number);
	    (void) updateArticleArray(newsgroup, False);
	}
    }
    return;
}


/*
  subscribe to the current newsgroup
  */
void subscribe()
{
    subscribe_group(CurrentGroup);
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

    if (IS_NOENTRY(newsgroup))
	addToNewsrcEnd(newsgroup->name, UNSUBSCRIBE);
    else
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
static char * buildQuestion _ARGUMENTS((struct newsgroup *, art_num));

static char * buildQuestion(newsgroup, article)
    struct newsgroup *newsgroup;
    art_num article;
{
    static char dummy[LABEL_SIZE];
    long unread, next_unread WALL(= 0);
    ng_num number;
    struct newsgroup *nextnewsgroup WALL(= 0);
    int found;

    unread = unreadArticleCount(newsgroup);

    found = 0;
    for (number = newsgroup->newsrc + 1; number < MaxGroupNumber;
	 number++) {
	nextnewsgroup = Newsrc[number];
	/* find a group that is subscribed to and has unread articles */
	if (IS_SUBSCRIBED(nextnewsgroup) &&
	    ((next_unread = unreadArticleCount(nextnewsgroup)) > 0)) {
	    found = 1;
	    break;
	}
    }
	    
    if (found) {
	if (unread <= 0) {
          (void) sprintf(dummy, QUEST_ART_NOUNREAD_NEXT_STRING,
			 article, newsgroup->name,
			 nextnewsgroup->name, next_unread,
			 (next_unread == 1) ? "" : NOT_ONE_MSG);
	} else {
          (void) sprintf(dummy, QUEST_ART_UNREAD_NEXT_STRING,
			 article, newsgroup->name, unread,
			 nextnewsgroup->name, next_unread,
			 (next_unread == 1) ? "" : NOT_ONE_MSG);
	}
    } else {
	if (unread <= 0) {
          (void) sprintf(dummy, QUEST_ART_NOUNREAD_NONEXT_STRING,
			   article, newsgroup->name);
	} else {
          (void) sprintf(dummy, QUEST_ART_UNREAD_NONEXT_STRING,
			   article, newsgroup->name, unread);
	}
    }
    return dummy;
}


static void handleXref _ARGUMENTS((struct newsgroup *, art_num));

static void handleXref(cur_newsgroup, article)
    struct newsgroup *cur_newsgroup;
    art_num article;
{
    char *string, *ptr, *num_ptr, *group, *gptr;
    art_num number;
    struct newsgroup *newsgroup;
    struct article *this_art;

    this_art = artStructGet(cur_newsgroup, article, False);

    if (IS_XREFED(this_art))
      return;

    if (this_art->xref)
      if (*this_art->xref)
	string = XtNewString(this_art->xref);
      else
	string = NULL;
    else
      xhdr(cur_newsgroup, article, "xref", &string);

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
    ptr++;

    while ((group = strtok(ptr, ":"))) {
	ptr = 0;

	if (! (num_ptr = strtok(ptr, " ")))
	    /* Something is confused.  Give up. */
	    break;

	if ((number = atol(num_ptr)) <= 0)
	    /* Something is confused differently.  Give up. */
	    break;
	
	if (!avl_lookup(NewsGroupTable, group, &gptr)) {
	    /* bogus group */
	    continue;
	}

	/* only Xref groups that are subscribed to */
	
	newsgroup = (struct newsgroup *) gptr;

	if (IS_SUBSCRIBED(newsgroup)) {
	  struct article *art;

	  if (number < newsgroup->first)
	    continue;
	  if (number > newsgroup->last)
	    if (app_resources.rescanOnEnter) {
	      art_num first, last;
	      int count;

	      if (getgroup(newsgroup, &first, &last, &count, False) != NO_GROUP)
		if ((number >= first) && (number <= last))
		  articleArrayResync(newsgroup, first, last, count);
		else
		  continue;
	      else
		continue;
	    }
	    else
	      continue;

	  artListSet(newsgroup);
	  art = artStructGet(newsgroup, number, True);
	  SET_XREFED(art);
	  /* Don't mark read in the current newsgroup; that's the caller's
	     responsibility. */
	  if ((newsgroup != cur_newsgroup) || (article != number))
	    SET_READ(art);
	  artStructSet(newsgroup, &art);
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
int getArticle(newsgroup, article, file, question)
    struct newsgroup *newsgroup;
    art_num article; 
    file_cache_file **file;
    char **question;
{
    struct article *art = artStructGet(newsgroup, article, True);
    file_cache_file *artfile;
    int header = 0, rotation = 0, xlation = 0;

#ifdef DEBUG
    fprintf(stderr, "getArticle(%s, %d)\n", newsgroup->name, article);
#endif

    if (IS_UNFETCHED(art)) {
	/* get the article and handle unavailable ones.... */
	if (IS_ALL_HEADERS(art))
	  header = FULL_HEADER;
	if (IS_ROTATED(art))
	  rotation = ROTATED;
#ifdef XLATE
	if (IS_XLATED(art))
	  xlation = XLATED;
#endif
	artfile = getarticle(newsgroup, article, 0, header | rotation |
			     xlation | PAGEBREAKS | BACKSPACES);
	/* need to refetch it since getarticle() modifies it */
	art = artStructGet(newsgroup, article, True);
	if (artfile)
	  art->file = artfile;
	else {
	    CLEAR_ALL(art);
	    SET_UNAVAIL(art);
	    artStructSet(newsgroup, &art);
	    return XRN_ERROR;
	}
	SET_FETCHED(art);
    } else {
	/* verify that the file still exists */
	if (!art->file || !*art->file ||
	    (access(file_cache_file_name(FileCache, *art->file), R_OK) == -1)) {
	    /* refetch the file */
	    CLEAR_FILE(art);
	    artStructSet(newsgroup, &art);
	    return getArticle(newsgroup, article, file, question);
	}
	file_cache_file_lock(FileCache, *art->file);
    }
    
    *file = art->file;
    if (IS_UNMARKED(art)) {
	SET_READ(art);
    }
    *question = buildQuestion(newsgroup, article);
    handleXref(newsgroup, article);

    artStructSet(newsgroup, &art);
    return XRN_OKAY;
}


static int toggleArtAttribute _ARGUMENTS((file_cache_file **, char **, int));

static int toggleArtAttribute(file, question, attribute)
    file_cache_file **file;
    char **question;
    int attribute;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *art = artStructGet(newsgroup, newsgroup->current, True);
    int ret;

    switch (attribute) {
    case ART_ALL_HEADERS:
	if (IS_ALL_HEADERS(art))
	    SET_STRIPPED_HEADERS(art);
	else
	    SET_ALL_HEADERS(art);
	break;
    case ART_ROTATED:
	if (IS_ROTATED(art))
	    SET_UNROTATED(art);
	else
	    SET_ROTATED(art);
	break;
#ifdef XLATE
    case ART_XLATED:
	if (IS_XLATED(art))
	    SET_UNXLATED(art);
	else
	    SET_XLATED(art);
	break;
#endif
    }

    CLEAR_FILE(art);

    artStructSet(newsgroup, &art);

    ret = getArticle(newsgroup, newsgroup->current, file, question);
    if (ret != XRN_OKAY) {
	mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, newsgroup->current);
    }
    return ret;
}

int toggleHeaders(file, question)
    file_cache_file **file;
    char **question;
{
    return toggleArtAttribute(file, question, ART_ALL_HEADERS);
}

int toggleRotation(file, question)
    file_cache_file **file;
    char **question;
{
    return toggleArtAttribute(file, question, ART_ROTATED);
}

#ifdef XLATE
int toggleXlation(file, question)
    file_cache_file **file;
    char **question;
{
    return toggleArtAttribute(file, question, ART_XLATED);
}
#endif /* XLATE */

static Boolean prefetchSetFetched _ARGUMENTS((XtPointer));

static Boolean prefetchSetFetched(pClient)
    XtPointer pClient;
{
    struct newsgroup *newsgroup = PrefetchingGroup;
    char msg[LABEL_SIZE];
#ifdef DEBUG
    int mesg_name = newMesgPaneName();
#endif

    if (rescan_id && !FinishingPrefetch) {
      (void) rescanWorkProc(0);
      return False;
    }

    InWorkProc++;

    if (! newsgroup) {
#ifdef DEBUG
	fprintf(stderr, "prefetchSetFetched[%d] returning (not in prefetch)\n",
		mesg_name);
#endif
	goto done;
    }
    if ((struct newsgroup *) pClient != newsgroup) {
#ifdef DEBUG
	fprintf(stderr, "prefetchSetFetched[%d] returning (wrong group)\n",
		mesg_name);
#endif
	goto done;
    }

    artListSet(newsgroup);

#ifdef DEBUG
    fprintf(stderr, "prefetchSetFetched[%d] fetching %s\n", mesg_name,
	    newsgroup->name);
#endif

    /* if the current article is unfetched, fetch it */
    if (FastServer || FinishingPrefetch) {
	struct article *art = artStructGet(newsgroup, newsgroup->current,
					   True);
	if (IS_UNFETCHED(art)) {
	    file_cache_file *artfile;

	    /* if the article can be fetched, mark it so */
	    artfile = getarticle(newsgroup, newsgroup->current, 0,
				 PAGEBREAKS | BACKSPACES);
	    /* need to refetch it since getarticle() modifies it */
	    art = artStructGet(newsgroup, newsgroup->current, True);

	    if (artfile) {
	      art->file = artfile;
	      SET_FETCHED(art);
	    }
	}

	artStructSet(newsgroup, &art);
    }

    PrefetchedGroup = newsgroup;
    PrefetchingGroup = 0;
    (void) sprintf(msg, PREFETCHING_MSG, newsgroup->name);
    (void) strcat(msg, " ");
    (void) strcat(msg, DONE_MSG);
    INFO(msg);

#ifdef DEBUG
    fprintf(stderr, "prefetchSetFetched[%d] fetched %s\n", mesg_name,
	    newsgroup->name);
#endif
  done:
    InWorkProc--;
    return True;
}


static Boolean prefetchGetArticles _ARGUMENTS((XtPointer));

static Boolean prefetchGetArticles(pClient)
    XtPointer pClient;
{
    register struct newsgroup *newsgroup = PrefetchingGroup;
#ifdef DEBUG
    int mesg_name = newMesgPaneName();
#endif

    if (rescan_id && !FinishingPrefetch) {
      (void) rescanWorkProc(0);
      return False;
    }

    InWorkProc++;

    if (! newsgroup) {
#ifdef DEBUG
	fprintf(stderr,
		"prefetchGetArticles[%d] returning (not in prefetch)\n",
		mesg_name);
#endif
	goto done;
    }

    if ((struct newsgroup *) pClient != newsgroup) {
#ifdef DEBUG
	fprintf(stderr, "prefetchGetArticles[%d] returning (wrong group)\n",
		mesg_name);
#endif
	goto done;
    }

    artListSet(newsgroup);

#ifdef DEBUG
    fprintf(stderr, "prefetchGetArticles[%d] fetching %s\n", mesg_name,
	    newsgroup->name);
#endif

    if ((newsgroup->current < newsgroup->first) ||
	(! unreadArticleCount(newsgroup))) {
	/* This will happen if setUpGroup couldn't find any unread
           articles in the newsgroup, in which case newsgroup->current
           will be 0, or if there are no unread articles in the group. */
	cancelPrefetch();
	prefetchNextGroup(newsgroup);
#ifdef DEBUG
	fprintf(stderr, "prefetchGetArticles[%d] fetching group after %s\n",
		mesg_name, newsgroup->name);
#endif
	goto done;
    }

    prefetch_id = XtAppAddWorkProc(TopContext,
				   prefetchSetFetched, newsgroup);
    PrefetchingProc = prefetchSetFetched;

#ifdef DEBUG
    fprintf(stderr, "prefetchGetArticles[%d] continuing with %s\n", mesg_name,
	    newsgroup->name);
#endif
  done:
    InWorkProc--;
    return True;    
}


static Boolean prefetchSetupUnreadGroup _ARGUMENTS((XtPointer));

static Boolean prefetchSetupUnreadGroup(pClient)
    XtPointer pClient;
{
    struct newsgroup *newsgroup = PrefetchingGroup;
#ifdef DEBUG
    int mesg_name = newMesgPaneName();
#endif
    Boolean ret;

    if (rescan_id && !FinishingPrefetch) {
      (void) rescanWorkProc(0);
      return False;
    }

    InWorkProc++;

    if (! newsgroup) {
#ifdef DEBUG
	fprintf(stderr,
		"prefetchSetupUnreadGroup[%d] returning (not in prefetch)\n",
		mesg_name);
#endif
	ret = True;
	goto done;
    }

    if ((struct newsgroup *) pClient != newsgroup) {
#ifdef DEBUG
	fprintf(stderr,
		"prefetchSetupUnreadGroup[%d] returning (wrong newsgroup)\n",
		mesg_name);
#endif
	ret = True;
	goto done;
    }

    if (PrefetchStage <= PREFETCH_LAST_STAGE) {
#ifdef DEBUG
	fprintf(stderr,
		"prefetchSetupUnreadGroup[%d] running stage %d on %s\n",
		mesg_name, PrefetchStage, newsgroup->name);
#endif
	if (setUpGroupIncremental(newsgroup, &PrefetchStage,
				  app_resources.rescanOnEnter,
				  True, True, True,
				  0, 0))
	    PrefetchStage++;
	ret = False;
	goto done;
    }
    else {
	if (! FinishingPrefetch)
	    redrawNewsgroupTextWidget(newsgroup->name, False);
	prefetch_id = XtAppAddWorkProc(TopContext,
				       prefetchGetArticles, newsgroup);
	PrefetchingProc = prefetchGetArticles;
#ifdef DEBUG
	fprintf(stderr, "prefetchSetupUnreadGroup[%d] continuing with %s\n",
		mesg_name, newsgroup->name);
#endif
	ret = True;
	goto done;
    }
  done:
    InWorkProc--;
    return ret;
}


static void prefetchNextGroup _ARGUMENTS((struct newsgroup *));

/*ARGSUSED*/
static void prefetchNextGroup(in_newsgroup)
    struct newsgroup *in_newsgroup;
{
    struct newsgroup *newsgroup;
    register ng_num number;
    char msg[LABEL_SIZE];
#ifdef DEBUG
    int mesg_name = newMesgPaneName();
#endif

#ifdef DEBUG
    fprintf(stderr, "prefetchNextGroup[%d](%s)\n", mesg_name, in_newsgroup ?
	    in_newsgroup->name : "NULL");
#endif

    if (app_resources.prefetchMax < 0)
	/* Don't prefetch at all if prefetchMax is negative. */
	goto done;

    /* find a group that is subscribed to and has unread articles */
    for (number = (in_newsgroup ? in_newsgroup->newsrc + 1 : 0);
	 number < MaxGroupNumber;
	 number++) {
	newsgroup = Newsrc[number];
	if (IS_SUBSCRIBED(newsgroup) && (unreadArticleCount(newsgroup) > 0))
	    break;
    }
    if (number < MaxGroupNumber) {
	if ((PrefetchingGroup == newsgroup) ||
	    (PrefetchedGroup == newsgroup))
	    goto done;
	cancelPrefetch();
	if ((! app_resources.prefetchMax) ||
	    (unreadArticleCount(newsgroup) <= app_resources.prefetchMax)) {
	    PrefetchingGroup = newsgroup;
	    PrefetchStage = PREFETCH_FIRST_STAGE;
	    (void) sprintf(msg, PREFETCHING_MSG, newsgroup->name);
#ifdef DEBUG
	    fprintf(stderr, "prefetchNextGroup[%d] continuing with %s\n",
		    mesg_name, newsgroup->name);
#endif
	    prefetch_id = XtAppAddWorkProc(TopContext,
					   prefetchSetupUnreadGroup, newsgroup);
	    PrefetchingProc = prefetchSetupUnreadGroup;
	    maybeInfoNow(msg);
	}
	
    }
    else if (PrefetchingGroup || PrefetchedGroup)
	cancelPrefetch();

  done:
#ifdef DEBUG
    fprintf(stderr, "prefetchNextGroup[%d](%s) done\n", mesg_name,
	    in_newsgroup ? in_newsgroup->name : "NULL");
#endif
    return;
}


/*
  Prefetch the specified group, if it has unread articles.
  */
void prefetchGroup(group)
    char *group;
{
    struct newsgroup *newsgroup;

#ifdef DEBUG
    fprintf(stderr, "prefetchGroup(%s)\n", group);
#endif

    if (! avl_lookup(NewsGroupTable, group, (char **)&newsgroup)) {
#ifdef DEBUG
	fprintf(stderr, "prefetchGroup(%s) done\n", group);
#endif
	return;
    }

    prefetchNextGroup(newsgroup->newsrc ? Newsrc[newsgroup->newsrc - 1] : 0);

#ifdef DEBUG
    fprintf(stderr, "prefetchGroup(%s) done\n", group);
#endif
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

#ifdef DEBUG
    fprintf(stderr, "prefetchNextArticle(%s)\n", newsgroup->name);
#endif

    artListSet(newsgroup);
    
    if (newsgroup->current == newsgroup->last) {
#ifdef NO_IMMEDIATE_WORK_PROC_PREFETCH
	/*
	 * if this is the last article, prefetch the next group, and the
	 * first article of the next group
	 */
	prefetchNextGroup(newsgroup);
#endif
	return;
    }

    if (FastServer) {
	art_num artnum = newsgroup->current + 1;
	struct article *art = artStructGet(newsgroup, artnum, True);
	file_cache_file *artfile;

	if (IS_UNFETCHED(art)) {
	  artfile = getarticle(newsgroup, artnum, 0, PAGEBREAKS | BACKSPACES);
	  /* need to refetch it since getarticle() modifies it */
	  art = artStructGet(newsgroup, artnum, True);

	  if (artfile) {
	    file_cache_file_unlock(FileCache, *artfile);
	    art->file = artfile;
	    SET_FETCHED(art);
	    SET_STRIPPED_HEADERS(art);
	    SET_UNROTATED(art);
	    SET_UNXLATED(art);
	  }
	}

	artStructSet(newsgroup, &art);
    }
}



/*
 * mark the articles in a group that have been read
 *
 * returns: True on success, False on failure indicating that the
 * group should not be entered.
 *
 */
Boolean updateArticleArray(
			   _ANSIDECL(struct newsgroup *,	newsgroup),
			   _ANSIDECL(Boolean,			do_unsubbed)
			   )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(Boolean,			do_unsubbed)
{
    struct list *item;
    struct article *art, copy;
    art_num artnum;
#ifndef FIXED_C_NEWS_ACTIVE_FILE
    int number;
#endif

    if (newsgroup->last == 0) {
	return True;
    }

#define CHECK_CACHED(label) \
    if (newsgroup->from_cache) { \
	art_num first, last; \
	int number; \
\
	if (getgroup(newsgroup, &first, &last, &number, False)) { \
	  return False; \
	} \
	else { \
	    articleArrayResync(newsgroup, first, last, number); \
	    newsgroup->from_cache = FALSE; \
	    goto label; \
	} \
    }

    if (!(do_unsubbed || IS_SUBSCRIBED(newsgroup))) {
	artListFree(newsgroup);
	return True;
    }

empty_retry:
    if (EMPTY_GROUP(newsgroup)) {
      CHECK_CACHED(empty_retry);
	artListFree(newsgroup);
	return True;
    }

    artListSet(newsgroup);

    if (newsgroup->nglist == NIL(struct list)) {
	return True;
    }

#ifndef FIXED_C_NEWS_ACTIVE_FILE
#ifdef DEBUG
    fprintf(stderr, "updateArticleArray(%s)\n", newsgroup->name);
#endif

    /* get the group range to fix c-news low number problem */
    if ((XRNState & XRN_NEWS_UP) == XRN_NEWS_UP) {
	(void) getgroup(newsgroup, &newsgroup->first, &newsgroup->last,
			&number, True);
	if ((newsgroup->first == 0 && newsgroup->last == 0) || number == 0) {
	    lsDestroy(newsgroup->nglist);
	    newsgroup->nglist = NIL(struct list);
	    return True;
	}
    }
#endif

    /* process the .newsrc line */

retry:
    for (item = newsgroup->nglist; item != NIL(struct list); item = item->next) {
	switch (item->type) {
	case SINGLE:
	    if (item->contents.single > newsgroup->last) {
		CHECK_CACHED(retry);
		/* something really bad has happened, reset */
		mesgPane(XRN_SERIOUS, 0, ART_NUMBERING_PROBLEM_MSG,
			 newsgroup->name);
		artListFree(newsgroup);
		artListSet(newsgroup);
		lsDestroy(newsgroup->nglist);
		newsgroup->nglist = NIL(struct list);
		return True;
	    }
	    if (item->contents.single >= newsgroup->first) {
		struct article *art = artStructGet(newsgroup,
						   item->contents.single,
						   True);
		art->status = ART_CLEAR_READ;
		artStructSet(newsgroup, &art);
	    }
	    break;

	case RANGE:
	    if ((item->contents.range.start > newsgroup->last) ||
		(item->contents.range.end > newsgroup->last)) {
		CHECK_CACHED(retry);
		/* something really bad has happened, reset */
		mesgPane(XRN_SERIOUS, 0, ART_NUMBERING_PROBLEM_MSG,
			 newsgroup->name);
		artListFree(newsgroup);
		artListSet(newsgroup);
		lsDestroy(newsgroup->nglist);
		newsgroup->nglist = NIL(struct list);
		return True;
	    }
	    if (item->contents.range.start < newsgroup->first) {
		item->contents.range.start = newsgroup->first;
	    }
	    
	    if (item->contents.range.end < newsgroup->first) {
		break;
	    }
	    for (artnum = item->contents.range.start;
		 artnum <= item->contents.range.end; artnum++) {
		art = artStructGet(newsgroup, artnum, False);
		copy = *art;
		copy.status = ART_CLEAR_READ;
		artStructReplace(newsgroup, &art, &copy, artnum);
	    }
	}
    }

#undef CHECK_CACHED

    lsDestroy(newsgroup->nglist);
    newsgroup->nglist = NIL(struct list);
    
    return True;
}


/*
 * mark an article as read
 */
void markArticleAsRead(article)
    art_num article;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *art;

    artListSet(newsgroup);

    art = artStructGet(newsgroup, article, True);
    SET_READ(art);
    SET_UNMARKED(art);
    artStructSet(newsgroup, &art);
}


/*
 * mark an article as unread
 */
void markArticleAsUnread(article)
    art_num article;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *art;

    artListSet(newsgroup);

    art = artStructGet(newsgroup, article, True);
    SET_UNREAD(art);
    SET_MARKED(art);
    artStructSet(newsgroup, &art);
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
  struct newsgroup *newsgroup;
  ng_num i;

  if (! verifyGroup(newGroup, &newsgroup, False)) {
    mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_MSG, newGroup);
    return BAD_GROUP;
  }
    
  CLEAR_NOENTRY(newsgroup);
  if (status == SUBSCRIBE) {
    subscribe_group(newsgroup);
  } else {
    SET_UNSUB(newsgroup);
  }
  if (newsgroup->newsrc == NOT_IN_NEWSRC) {
    for (i = MaxGroupNumber - 1; i != NOT_IN_NEWSRC; i--) {
      Newsrc[i + 1] = Newsrc[i];
      Newsrc[i + 1]->newsrc = i + 1;
    }

    INC_MAXGROUPNUMBER();
  } else {
    for (i = newsgroup->newsrc - 1; i != NOT_IN_NEWSRC; i--) {
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
    struct newsgroup *newsgroup;
    ng_num i;

    if (! verifyGroup(newGroup, &newsgroup, False)) {
      mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_MSG, newGroup);
      return BAD_GROUP;
    }
    
    CLEAR_NOENTRY(newsgroup);
    if (status == SUBSCRIBE) {
	subscribe_group(newsgroup);
    } else {
	SET_UNSUB(newsgroup);
    }
    if (newsgroup->newsrc == NOT_IN_NEWSRC) {
      INC_MAXGROUPNUMBER();
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
    struct newsgroup *newsgroup, *ng;
    ng_num newloc, i;

    if (! verifyGroup(newGroup, &newsgroup, False)) {
      mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_MSG, newGroup);
      return BAD_GROUP;
    }
    
    CLEAR_NOENTRY(newsgroup);
    if (status == SUBSCRIBE) {
	subscribe_group(newsgroup);
    } else {
	SET_UNSUB(newsgroup);
    }

    if (! avl_lookup(NewsGroupTable, afterGroup, (char **) &ng)) {
      Boolean no_group = True;

      if (! active_read) {
	char *err_buf;

	err_buf = XtMalloc(strlen(MAYBE_LIST_MSG)+strlen(afterGroup));
	(void) sprintf(err_buf, MAYBE_LIST_MSG, afterGroup);

	no_group =
	  (ConfirmationBox(TopLevel, err_buf, "yes", "no", True) ==
	   XRN_CB_ABORT) ||
	  (! verifyGroup(afterGroup, &ng, False));
	
	XtFree(err_buf);
      }

      if (no_group) {
	mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_MSG, afterGroup);
	return BAD_GROUP;
      }
    }

    newloc = ng->newsrc;

    if (newloc == NOT_IN_NEWSRC) {
	mesgPane(XRN_SERIOUS, 0, NOT_IN_NEWSRC_MSG, afterGroup);
	return BAD_GROUP;
    }

    if (newsgroup->newsrc == NOT_IN_NEWSRC) {
	for (i = MaxGroupNumber - 1; i >= newloc + 1; i--) {
	    Newsrc[i + 1] = Newsrc[i];
	    Newsrc[i + 1]->newsrc = i + 1;
	}

	INC_MAXGROUPNUMBER();
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


int ignoreGroup(group)
char *group;
{
  struct newsgroup *newsgroup;

  if (! verifyGroup(group, &newsgroup, False)) {
    mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_MSG, group);
    return BAD_GROUP;
  }

  if (newsgroup->newsrc != NOT_IN_NEWSRC)
      return removeFromNewsrc(group);

  SET_NOENTRY(newsgroup);
  SET_UNSUB(newsgroup);

  return GOOD_GROUP;
}

  
int removeFromNewsrc(group)
char *group;
{
  struct newsgroup *newsgroup;
  int i;

  if (! verifyGroup(group, &newsgroup, False)) {
    mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_MSG, group);
    return BAD_GROUP;
  }

  if (newsgroup->newsrc != NOT_IN_NEWSRC) {
    MaxGroupNumber--;

    for (i = newsgroup->newsrc; i < MaxGroupNumber; i++) {
      Newsrc[i] = Newsrc[i + 1];
      Newsrc[i]->newsrc = i;
    }

    newsgroup->newsrc = NOT_IN_NEWSRC;
  }

  SET_NOENTRY(newsgroup);
  SET_UNSUB(newsgroup);
  CLEAR_NEW(newsgroup);

  return GOOD_GROUP;
}

  
/*
 * build and return a string that shows the subscription status
 * of all newsgroups; assumes all groups have been subscribed or
 * unsubscribed to by this time.
 *
 *   if sorted is non-zero, the list is sorted alphabetically, if
 *    zero, the list is returned as it exists in the newsrc file
 *
 * The specified regular expression limits the list to the newsgroups
 * whose names match it.  May return NULL if there's an error in the
 * regular expression; otherwise, will never return NULL.
 */
char * getStatusString(line_width, sorted, regexp)
    int line_width;
    int sorted;
    char *regexp;
{
    int i, count = 0, bytes = 0;
    char buffer[1024];
    avl_generator *gen;
    char *key, *value;
    char **ar;
    char *string;
    static int status_width = 0;
    int newsgroup_width;
#ifdef POSIX_REGEX
    regex_t reStruct;
    int reRet;
#else
# ifdef SYSV_REGEX
    char *reRet = 0;
# else
    char *reRet;
# endif
#endif

    if (regexp) {
      if (
#ifdef POSIX_REGEX
	  (reRet = regcomp(&reStruct, regexp, REG_NOSUB))
#else
# ifdef SYSV_REGEX
	  ! (reRet = regcmp(regexp, NULL))
# else
	  (reRet = re_comp(regexp))
# endif
#endif
	  ) {
#ifdef SYSV_REGEX
	mesgPane(XRN_SERIOUS, 0, UNKNOWN_REGEXP_ERROR_MSG, regexp);
#else
# ifdef POSIX_REGEX
	regerror(reRet, &reStruct, error_buffer, sizeof(error_buffer));
# endif
	mesgPane(XRN_SERIOUS, 0, KNOWN_REGEXP_ERROR_MSG, regexp,
# ifdef POSIX_REGEX
		 error_buffer
# else
		 reRet
# endif /* POSIX_REGEX */
		 );
#endif /* SYSV_REGEX */
	return NULL;
      }
    }

    if (! status_width)
	status_width = MAX(utStrlen(IGNORED_MSG),
			   MAX(utStrlen(SUBED_MSG), utStrlen(UNSUBED_MSG)));

    newsgroup_width = line_width - status_width - 1
	/* the 1 is for the space between the newsgroup name and status */;

    ar = ARRAYALLOC(char *, ActiveGroupsCount);

    if (sorted) {
	gen = avl_init_gen(NewsGroupTable, AVL_FORWARD);
	if (! gen) {
          ehErrorExitXRN(ERROR_OUT_OF_MEM_MSG);
	}

	while (avl_gen(gen, &key, &value)) {
	    struct newsgroup *newsgroup = (struct newsgroup *) value;

	    if (regexp &&
#ifdef POSIX_REGEX
		regexec(&reStruct, newsgroup->name, 0, 0, 0)
#else
# ifdef SYSV_REGEX
		! regex(reRet, newsgroup->name)
# else
		! re_exec(newsgroup->name)
# endif
#endif
		)
	      continue;

	    (void) sprintf(buffer, "%*s %s",
			   -newsgroup_width, newsgroup->name,
			   (IS_NOENTRY(newsgroup) ? IGNORED_MSG :
			    (IS_SUBSCRIBED(newsgroup) ? SUBED_MSG : UNSUBED_MSG)));
	    ar[count++] = XtNewString(buffer);
	    bytes += strlen(buffer);
	}
	avl_free_gen(gen);
	
    } else {
	for (i = 0; i < MaxGroupNumber; i++) {
	    struct newsgroup *newsgroup = (struct newsgroup *) Newsrc[i];
	    
	    if (regexp &&
#ifdef POSIX_REGEX
		regexec(&reStruct, newsgroup->name, 0, 0, 0)
#else
# ifdef SYSV_REGEX
		! regex(reRet, newsgroup->name)
# else
		! re_exec(newsgroup->name)
# endif
#endif
		)
	      continue;

	    (void) sprintf(buffer, "%*s %s",
			   -newsgroup_width, newsgroup->name,
			   (IS_SUBSCRIBED(newsgroup) ? SUBED_MSG : UNSUBED_MSG));
	    ar[count++] = XtNewString(buffer);
	    bytes += strlen(buffer);
	}

	gen = avl_init_gen(NewsGroupTable, AVL_FORWARD);
	if (! gen)
	    ehErrorExitXRN(ERROR_OUT_OF_MEM_MSG);
	while (avl_gen(gen, &key, &value)) {
	    struct newsgroup *newsgroup = (struct newsgroup *) value;

	    if (regexp &&
#ifdef POSIX_REGEX
		regexec(&reStruct, newsgroup->name, 0, 0, 0)
#else
# ifdef SYSV_REGEX
		! regex(reRet, newsgroup->name)
# else
		! re_exec(newsgroup->name)
# endif
#endif
		)
	      continue;

	    if (IS_NOENTRY(newsgroup)) {
		(void) sprintf(buffer, "%*s %s",
			       -newsgroup_width, newsgroup->name,
			       IGNORED_MSG);
		ar[count++] = XtNewString(buffer);
		bytes += strlen(buffer);
	    }
	}
	avl_free_gen(gen);
    }	    
    
    string = arrayToString(ar, count, bytes);
    for (i = 0; i < count; i++) {
	FREE(ar[i]);
    }
    FREE(ar);

    if (regexp) {
#ifdef POSIX_REGEX
      regfree(&reStruct);
#else
# ifdef SYSV_REGEX
      free(reRet);
# endif
#endif
    }

    return string;
}



/*
  If art_num is greater than 0, format a single line in the subject
  index, and return its length (including the final newline).  If
  art_num is less than or equal to 0, just return what the length
  would be if a subject line were formatted, and reset our idea of the
  current width of the index window.  Beware that this should be
  called with art_num <= 0 before starting any series of consecutive
  calls to it.
  */
int subjectIndexLine(
		     _ANSIDECL(int,			line_length),
		     _ANSIDECL(char *,			out),
		     _ANSIDECL(struct newsgroup *,	newsgroup),
		     _ANSIDECL(art_num,			artNum),
		     _ANSIDECL(Boolean,			threaded)
		     )
     _KNRDECL(int,			line_length)
     _KNRDECL(char *,			out)
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			artNum)
     _KNRDECL(Boolean,			threaded)
{
    static int lineLength, subLength, authorLength, lineWidth;
    static int num_width;
    struct article *art, *parent;
    char *subject, *lines, *author;
    int thread_depth, ret;
    
    if (artNum <= 0) {
      num_width = utDigits(newsgroup->last);
	lineLength = line_length;
	lineLength = MIN(LINE_LENGTH - 1, lineLength); /* for the NULL */
	subLength = lineLength -
	  3 		/* spaces before and after article number */ -
	  num_width 	/* width of article number */ -
	  2		/* spaces after subject, or spaces before and
			   after line count */ -
	  1 /* newline */;
	if (app_resources.displayLineCount) {
	    lineWidth = 6;
	    subLength -= lineWidth;
	}
	else {
	    lineWidth = 0;
	}

	if (subLength < 0) {
	    lineLength -= subLength;
	    subLength = 0;
	}

	authorLength = 0.25 * subLength;
	subLength -= authorLength;

	return lineLength;
    }

    artListSet(newsgroup);
    art = artStructGet(newsgroup, artNum, False);

    if (app_resources.displayLineCount) {
	lines = (art->lines && *art->lines) ? art->lines : "[?]";
    }
    else {
	lines = "";
    }

    if (threaded) {
      hash_table_object loop_table = 0;

    recheck:
      thread_depth = 0;
      parent = art;
      if (loop_table) {
	ret = hash_table_insert(loop_table, (void *)artNum, (void *)1, 1);
	assert(ret);
      }
      while (parent->parent) {
	if (loop_table &&
	    ! hash_table_insert(loop_table, (void *)parent->parent,
				(void *)1, 1)) {
	  hash_table_destroy(loop_table);
	  break;
	}
	if (++thread_depth == subLength) {
	  if (loop_table) {
	    hash_table_destroy(loop_table);
	    break;
	  }
	  else {
	    /* Heavily nested threading could really be authentic, or
	       it could be because of a loop in "References"
	       dependencies.  To find out which it is, we create a
	       hash table to keep track of which articles we've seen,
	       and then redo the thread-depth check.

	       We only create this hash table when we reach the
	       boundary condition, rather than for every article,
	       because creating the hash table and inserting the
	       articles number into it is expensive (well, at least,
	       more expensive than not doing it), and we want this
	       code to be as fast as possible. */
	    loop_table = hash_table_create(subLength + 1,
					   hash_int_calc, hash_int_compare,
					   hash_int_compare, 0, 0);
	    goto recheck;
	  }
	}

	parent = artStructGet(newsgroup, parent->parent, False);
      }
    }
    else {
      thread_depth = 0;
    }

    subject = art->subject;

    author = art->author ? art->author : "(none)";

    (void) sprintf(out, "  %*ld %*.*s%*.*s %*.*s %*.*s\n", num_width,
		   artNum, thread_depth, thread_depth, "",
		   -(subLength-thread_depth), subLength-thread_depth,
		   subject, lineWidth,
		   lineWidth, lines, -authorLength, authorLength,
		   author);
    if (IS_READ(art))
	*out = READ_MARKER;
    else if (IS_MARKED(art))
	*out = UNREAD_MARKER;
    if (IS_SAVED(art))
	*(out + 1) = SAVED_MARKER;
    else if (IS_PRINTED(art))
	*(out + 1) = PRINTED_MARKER;

    return lineLength;
}



char * getSubjects(
		   _ANSIDECL(int,	line_length),
		   _ANSIDECL(art_num,	artNum),
		   _ANSIDECL(Boolean,	rethread)
		   )
     _KNRDECL(int,	line_length)
     _KNRDECL(art_num,	artNum)
     _KNRDECL(Boolean,	rethread)
{
  NextPreviousArticle = artNum - 1;

  rethreadGroup(CurrentGroup, artNum, rethread);

  return art_sort_doit(CurrentGroup, artNum, CurrentGroup->last, line_length);
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


#define STRIPLEADINGSPACES   for (; *start && isspace(*start); start++);
#define STRIPENDINGSPACES  for (; (end >= start) && isspace(*end); *end-- = '\0');

char * subjectStrip(str)
    char *str;
{
    register char *start, *end, *ptr;
    static char work[BUFFER_SIZE];

    (void) strncpy(work, str, BUFFER_SIZE);
    start = work;
    work[BUFFER_SIZE - 1] = '\0';

    /*
     * strip leading '[rR][eE]: ' and 'Re^N: ' -
     * only if striprefs is TRUE (want to be able to kill follow-ups)
     */
    while (! strncasecmp(start, "re: ", 4)) {
      start += 4;
      STRIPLEADINGSPACES;
    }

    while (! strncasecmp(start, "re^", 3)) {
      start += 3;
      if ((ptr = index(start, ':')))
	start = ptr + 1;
      STRIPLEADINGSPACES;
    }

    for (end = start + 1; (end = index(end, '(')); end++)
      if (! strncasecmp(end, "(was", 4) &&
	  ((end[4] == ':') || (end[4] == ' '))) {
	*end = '\0';
	break;
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
    art_num article;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *art;

    artListSet(newsgroup);

    art = artStructGet(newsgroup, article, False);
    return art->subject ? subjectStrip(art->subject) : 0;
}


/*
 * return the author of an article
 */
char * getAuthor(article)
    art_num article;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *art;

    art = artStructGet(newsgroup, article, False);
    return art->author;
}


art_num getPrevNumber()
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *art;

    /* search for the next available article in the reverse direction */
    for ( ; NextPreviousArticle >= newsgroup->first; NextPreviousArticle--) {
	art = artStructGet(newsgroup, NextPreviousArticle, False);

	if (IS_UNAVAIL(art))
	    continue;

	if (! art->subject) {
	  (void) fillUpArray(newsgroup,
			     MAX(newsgroup->first, NextPreviousArticle - SUBJECTS),
			     NextPreviousArticle, False, False);
	  NextPreviousArticle++;
	  continue;
	}

	if (art->subject) {
	    return NextPreviousArticle--;
	}
    }

    return 0;
}


/*
 * get the previous subject index line (article number is NextPreviousArticle).
 * only called when going off the top of the subject string
 *
 *   returns a point to a static area
 *
 *  NextPreviousArticle is set to current-1 on building the subject string.
 *  NextPreviousArticle is decremented by this routine.
 */
char *getPrevSubject(line_length)
     int line_length;
{
  static char buffer[BUFFER_SIZE];
  struct newsgroup *newsgroup = CurrentGroup;

  art_num art = getPrevNumber();

  if (art) {
    (void) subjectIndexLine(line_length, buffer, newsgroup, art, False);
    return buffer;
  }

  return 0;
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


int fillUpArray(
		_ANSIDECL(struct newsgroup *,	newsgroup),
		_ANSIDECL(art_num,		art),
		_ANSIDECL(art_num,		last),
		_ANSIDECL(Boolean,		check_abort),
		_ANSIDECL(Boolean,		kill_files)
		)
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			art)
     _KNRDECL(art_num,			last)
     _KNRDECL(Boolean,			check_abort)
     _KNRDECL(Boolean,			kill_files)
{
    int i;

    if (! last)
      last = newsgroup->last;

    for (i = PREFETCH_START_HEADERS_STAGE; i <= PREFETCH_LAST_HEADERS_STAGE; ) {
      if (fetchHeadersIncremental(newsgroup, &i, art, last,
				  False, kill_files))
	i++;
      if (check_abort && abortP())
	return ABORT;
    }

    return(! ABORT);
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
 * 			then enter the newsgroup if any article, read
 *			or not, is available.
 * ENTER_UNSUBBED	If true, then we're allowed to switch to an
 * 			unsubscribed newsgroup.
 * ENTER_SUBSCRIBE	If true, then subscribe to the newsgroup if
 *			it's unsubscribed, and add it to the end of
 *			the .newsrc if it isn't already in it.
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
 * article is now current and should be displayed).
 */
int enterNewsgroup(name, flags)
    char *name;
    int flags;
{
    int ret = GOOD_GROUP;
    struct newsgroup *newsgroup;
    Boolean unsubbed = False;

    if (!avl_lookup(NewsGroupTable, name, (char **) &newsgroup)) {
      newsgroup = 0;
      if (flags & ENTER_REGEXP)
	newsgroup = findNewsGroupMatch(name);
      if (! (newsgroup || active_read)) {
	char *err_buf;

	err_buf = XtMalloc(strlen(MAYBE_LIST_MSG)+strlen(name));
	(void) sprintf(err_buf, MAYBE_LIST_MSG, name);
	if ((ConfirmationBox(TopLevel, err_buf, "yes", "no", True)
	     == XRN_CB_ABORT) || (! verifyGroup(name, &newsgroup, False))) {
	  XtFree(err_buf);
	  return BAD_GROUP;
	}
      }
      if (! newsgroup)
	return BAD_GROUP;
    }

    if (! IS_SUBSCRIBED(newsgroup)) {
	if (flags & ENTER_UNSUBBED) {
	    if (flags & ENTER_SUBSCRIBE) {
		if (IS_NOENTRY(newsgroup)) {
		    if (addToNewsrcEnd(name, SUBSCRIBE) != GOOD_GROUP)
			return BAD_GROUP;
		}
		else {
		    SET_SUB(newsgroup);
		}
	    }
	    if (! updateArticleArray(newsgroup, flags & ENTER_SETUP)) {
	      return BAD_GROUP;
	    }
	    unsubbed = True;
	}
	else {
	    return XRN_UNSUBBED;
	}
    }

    if (flags & ENTER_SETUP) {
	if (app_resources.rescanOnEnter || unsubbed)
	    setUpGroup(newsgroup, True, False, flags & ENTER_UNREAD,
		       False);

	if (EMPTY_GROUP(newsgroup)) {
	    return XRN_NOMORE;
	}

	do {
	    setUpGroup(newsgroup, False, True, flags & ENTER_UNREAD,
		       True);
	/*
	  We need to do setCurrentArticle again here, even though
	  setUpGroup() does it, because if the group is already
	  prefetched, and *then* the first unread article in it is
	  marked read while reading another group, setUpGroup() won't
	  reset the first unread article (because it doesn't do
	  anything if the group is already prefetched).
	  */
	} while (setCurrentArticle(newsgroup, flags & ENTER_UNREAD,
				   True, 0, 0));

	/*
	  Need to check if the group is empty again, because
	  setUpGroup and/or setCurrentArticle may have discovered that
	  in fact, there aren't any articles at all available in the
	  group (e.g., our cached information about the group is
	  incorrect).
	  */
	if (EMPTY_GROUP(newsgroup)) {
	  return XRN_NOMORE;
	}

	if (newsgroup->current > newsgroup->last)
	    ret = XRN_NOUNREAD;
	else {
	    struct article *art =
		artStructGet(newsgroup, newsgroup->current, False);
	    if (IS_READ(art))
		ret = XRN_NOUNREAD;
	    else
		ret = GOOD_GROUP;
	}

	if ((ret == XRN_NOUNREAD) && (flags & ENTER_UNREAD))
	  return ret;

	groupSnapshotSave(newsgroup);
    }

    CurrentGroup = newsgroup;

    return ret;
}

void exitNewsgroup()
{
  CurrentGroup = 0;
}


#ifdef POSIX_REGEX
regex_t *
#else
char **
#endif
parseRegexpList(list, list_name, count)
    char *list, *list_name;
    int *count;
{
    char *p, *q, *r;
    int n;
#ifdef POSIX_REGEX
    regex_t *l;
    int reRet;
#else
    char **l;
#endif

    if (! list) {
	return 0;
    }

    /* Count the number of elements in the list, if there are no regexp errors */
    for (p = q = XtNewString(list), n = 0; strtok(p, ", \t\n"); p = 0) n++;

    FREE(q);

    if (! n) {
	return 0;
    }

#ifdef POSIX_REGEX
    l = ARRAYALLOC(regex_t, n);
#else
    l = ARRAYALLOC(char *, n);
#endif

    /* Now build the list */
    for ((p = r = XtNewString(list)), n = 0; (q = strtok(p, ", \t\n")); p = 0) {
#ifdef POSIX_REGEX
	if ((reRet = regcomp(&l[n], q, REG_NOSUB))) {
	    regerror(reRet, &l[n], error_buffer,
		     sizeof(error_buffer));
	    mesgPane(XRN_SERIOUS, 0, KNOWN_LIST_REGEXP_ERROR_MSG,
		     list_name, q, error_buffer);
	    continue;
	}
#else
# ifdef SYSV_REGEX
	if (! (l[n] = regcmp(q, 0))) {
	    mesgPane(XRN_SERIOUS, 0, UNKNOWN_LIST_REGEXP_ERROR_MSG, list_name, q);
	    continue;
	}
# else
	l[n] = q;
	if ((q = re_comp(l[n]))) {
	    mesgPane(XRN_SERIOUS, 0, KNOWN_LIST_REGEXP_ERROR_MSG, list_name, l[n], q);
	    continue;
	}
# endif
#endif
	n++;
    }

#if defined(POSIX_REGEX) || defined(SYSV_REGEX)
    FREE(r);
#endif

    if (! n) {
	FREE(r);
	FREE(l);
	return 0;
    }

    *count = n;

    return l;
}

/*
  Returns -2 if the article doesn't exist, -1 if it exists but is not
  in the specified  newsgroup (or is not in the currently available
  range in the newsgroup), 0 if the article has no xref header, or
  the article number of the article.
  */
art_num getArticleNumberFromIdXref(
			       _ANSIDECL(struct newsgroup *,	newsgroup),
			       _ANSIDECL(char *,		id)
			       )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(char *,			id)
{
  char *xref, *ptr;
  int len;
  art_num artNum;
  long error_code;
  
  /* First, the easy way, using Xref */
  xref = xhdr_id(newsgroup, id, "xref", &error_code);
  if (xref) {
    for (ptr = strstr(xref, newsgroup->name); ptr;
	 ptr = strstr(ptr + 1, newsgroup->name)) {
      if (((ptr == xref) || isspace(ptr[-1])) &&
	  (len = strlen(newsgroup->name)) && ptr[len] == ':') {
	artNum = atol(&ptr[len+1]);
	XtFree(xref);
	if ((artNum > newsgroup->last) || (artNum < newsgroup->first))
	  return -1;
	else
	  return artNum;
      }
    }
    XtFree(xref);
    return -1;
  }

  if (error_code == ERR_NOART)
    return -2;

  return 0;
}
