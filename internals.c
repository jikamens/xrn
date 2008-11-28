#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: internals.c,v 1.78 1995-02-01 15:33:57 jik Exp $";
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
#include "snapshot.h"

#ifndef R_OK
#define R_OK 4
#endif


static char *strip _ARGUMENTS((char *, /* Boolean */ int));

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
static Boolean FinishingPrefetch = False;
static Boolean FastServer = False;
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
#ifndef PREFECH_CHUNK_TIME
#define PREFETCH_CHUNK_TIME 1
#endif
#ifndef PREFETCH_FAST_CHUNK
#define PREFETCH_FAST_CHUNK 160
#endif

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
    busyCursor();

    start_server();

    do {
	 NewsGroupTable = avl_init_table(strcmp);
	 if (! NewsGroupTable) {
           ehErrorExitXRN(ERROR_OUT_OF_MEM_MSG);
	 }

	 getactive();
	 
	 if (readnewsrc(app_resources.newsrcFile,
			app_resources.saveNewsrcFile))
	      break;
	 
       ehErrorRetryXRN(ERROR_CANT_READ_NEWSRC_MSG, True);

	 avl_free_table(NewsGroupTable, XtFree,
			(void (*) _ARGUMENTS((void *))) XtFree);
    } while (1);
	
#ifndef FIXED_ACTIVE_FILE
    badActiveFileCheck();
#endif

    unbusyCursor();

    return;
}


/*
 * get the active file again and grow the Newsrc array if necessary
 */
void rescanServer()
{
    int old = ActiveGroupsCount;

    cancelPrefetch();

    /* update the newsrc file */
    while (!updatenewsrc())
      ehErrorRetryXRN(ERROR_CANT_UPDATE_NEWSRC_MSG, True);

#if !defined(NNTP_REREADS_ACTIVE_FILE)
    stop_server();
    start_server();
#endif
    
    getactive();

    if (ActiveGroupsCount > old) {
	/* new newsgroups were found, allocate a bigger array */
	Newsrc = (struct newsgroup **) XtRealloc((char *) Newsrc, (unsigned) (sizeof(struct newsgroup *) * ActiveGroupsCount));
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
static Boolean articleIsAvailable _ARGUMENTS((struct newsgroup *, art_num));

static Boolean articleIsAvailable(newsgroup, i)
    struct newsgroup *newsgroup;
    art_num i;
{
    char *subject;
    Boolean ret;
    struct article *art;

    xhdr(newsgroup, i, "subject", &subject);
    if (subject) {
	ret = True;
	XtFree(subject);
    }
    else {
	ret = False;
	art = artStructGet(newsgroup, i, True);
	CLEAR_FILE(art);
	CLEAR_SUBJECT(art);
	CLEAR_AUTHOR(art);
	CLEAR_LINES(art);
	SET_UNAVAIL(art);
	artStructSet(newsgroup, &art);
    }

    return ret;
}


/*
 * find the first unread article in a group and set 'current' to it
 *
 * returns: void
 *
 */
static void setCurrentArticle _ARGUMENTS((struct newsgroup *, /* Boolean */ int));

static void setCurrentArticle(newsgroup, unread_only)
    struct newsgroup *newsgroup;
    Boolean unread_only;
{
    struct article *art;
    art_num i;
    art_num start;
    
    /* if the resource 'onlyShow' is > 0, then mark all but the last
     * 'onlyShow' articles as read */

    start = newsgroup->first;
    if (app_resources.onlyShow > 0) {
	start = MAX(start, newsgroup->last - app_resources.onlyShow);
    }

    for (i = start; i<= newsgroup->last; i++) {
	art = artStructGet(newsgroup, i, False);
	if (IS_UNREAD(art) && IS_AVAIL(art) &&
	    articleIsAvailable(newsgroup, i)) {
	    newsgroup->current = i;
	    return;
	}
    }

    if (unread_only) {
	newsgroup->current = newsgroup->last + 1;
    }
    else {
	for (i = newsgroup->last; i >= newsgroup->first; i--) {
	    art = artStructGet(newsgroup, i, False);
	    if (IS_AVAIL(art) && articleIsAvailable(newsgroup, i)) {
		newsgroup->current = i;
		return;
	    }
	}
	newsgroup->current = newsgroup->last + 1;
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
  specified, copies the new group into group_name.

  When returning 0 because no match could be found, makes group_name
  an empty string.
  */
int getNearbyNewsgroup(list, group_name)
    char *list;
    char *group_name;
{
    struct newsgroup *group;
    char *ptr;
    char match_group[GROUP_NAME_SIZE];

    if (! group_name)
	return 0;

    if (! *group_name) {
	if (sscanf(list, NEWS_GROUP_LINE, match_group) == 1)
	    (void) strcpy(group_name, match_group);
	return 0;
    }

    if (! avl_lookup(NewsGroupTable, group_name, (char **) &group)) {
	*group_name = '\0';
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
	assert(sscanf(ptr, NEWS_GROUP_LINE, match_group) == 1);
	if (! strcmp(group->name, match_group))
	    break;
    }

    if (ptr)
	return ptr - list;

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

	assert(sscanf(ptr, NEWS_GROUP_LINE, match_group) == 1);
	if (! avl_lookup(NewsGroupTable, match_group, &gptr))
	    continue;
	if (((struct newsgroup *)gptr)->newsrc >= group->newsrc) {
	    (void) strcpy(group_name, match_group);
	    return ptr - list;
	}
    }

    *group_name = '\0';

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
char *unreadGroups(mode)
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
	    (void) sprintf(dummy, NEWSGROUPS_INDEX_MSG,
	                   (unread > 0 ? UNREAD_MSG : ""),
	                   (total > 0 ? NEWS_IN_MSG : ""),
			   newsgroup->name, unread,
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

	if ((newsgroup->last == 0) || EMPTY_GROUP(newsgroup)) {
	    return;
	}

	for (artnum = newsgroup->first; artnum <= newsgroup->last; artnum++) {
	    original = artStructGet(newsgroup, artnum, False);
	    copy = *original;

	    copy.subject = 0;
	    copy.author = 0;
	    copy.lines = 0;
	    copy.filename = 0;

	    SET_UNMARKED(&copy);
	    SET_STRIPPED_HEADERS(&copy);
	    SET_UNROTATED(&copy);
	    SET_UNXLATED(&copy);

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
	copy.subject = 0;
	copy.author = 0;
	copy.lines = 0;
	copy.filename = 0;

	copy.status = ART_CLEAR_READ;
	SET_UNAVAIL(&copy);
	for (i = newsgroup->first; i < first; i++) {
	    art = artStructGet(newsgroup, i, False);
	    artStructReplace(newsgroup, &art, &copy, i);
	}

	copy.status = ART_CLEAR;
	for (i = newsgroup->last + 1; i <= last; i++) {
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
    (void) strcpy(&buffer[i], "/KILL");

    return buffer;
}

char * globalKillFile()
{
    static char buffer[BUFFER_SIZE];

    if (!createNewsDir()) {
	return NIL(char);
    }
    (void) strcpy(buffer, app_resources.expandedSaveDir);
    (void) strcat(buffer, "/KILL");
    return buffer;
}


/*
 * add a kill subject/author entry to a kill file
 */
void killItem(newsgroup, item, type)
    struct newsgroup *newsgroup;
    char *item;
    int type;
{
    char input[BUFFER_SIZE], *iptr;
    char output[BUFFER_SIZE], *optr;
    FILE *fp;
    char *file;

    if (type == KILL_LOCAL) {
	file = localKillFile(newsgroup, 1);
    } else {
	file = globalKillFile();
    }

    if ((fp = fopen(file, "a")) == NULL) {
	mesgPane(XRN_SERIOUS, 0, CANT_OPEN_KILL_MSG,
		 ((type == KILL_LOCAL) ? "local" : "global"),
		 file, newsgroup->name, errmsg(errno));
	return;
    }

    /* quote or get rid of all magic characters */
    (void) strncpy(input, item, sizeof(input)-1);
    input[sizeof(input)-1] = '\0';
    iptr = input;
    optr = output;
    while (*iptr && (optr - output < sizeof(output)-4)) {
	switch (*iptr) {
	case '/':
	case '\\':
	case '[':
	/* Theoretically, ']' shouldn't be special when it isn't
	   preceded by '[', but it doesn't hurt to be cautious. */
	case ']':
	case '+':
	case '.':
	case '{':
	case '}':
	case '^':
	case '*':
	case '$':
	    *optr++ = '\\';
	    *optr++ = *iptr++;
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
	case '(':
	/* On some systems, it's an error for there to be a ')' with
	   no matching '(' before it. */
	case ')':
	    *optr++ = '[';
	    *optr++ = *iptr++;
	    *optr++ = ']';
	    break;
	default:
	    *optr++ = *iptr++;
	    break;
	}
    }
    *optr = '\0';

    fprintf(fp, "/%.*s/:j\n", BUFFER_SIZE - 6, output);
    (void) fclose(fp);
    return;
}

/*
  Return true if the indicated character, which should be in the
  string pointed to by start, is backslash-quoted, or false otherwise.
  Deals correctly with quoted backslashes immediately following a
  non-quoted character.
  */
static Boolean isQuoted _ARGUMENTS((char *, char *));

static Boolean isQuoted(character, start)
    char *character, *start;
{
    /*
      Figure out how many backslashes there are between the indicated
      character and either the first non-backslash or the beginning of
      the string.

      If the number of backslashes is odd, then return True.
      Otherwise, return False.
      */
    Boolean quoted = False;

    for (character--;
	 (character >= start) && (*character == '\\');
	 character--)
	quoted = ! quoted;

    return quoted;
}


      
/*
 * kill all subjects in the newsgroup that match the kill
 * orders in fp.
 */
/* XXX  THIS ROUTINE REALLY NEEDS TO BE CLEANED UP */
static void killArticles _ARGUMENTS((struct newsgroup *, FILE *, int));

static void killArticles(newsgroup, fp, max)
    struct newsgroup *newsgroup;
    FILE *fp;
    int max;
{
    struct article *art, copy;
    char string[BUFFER_SIZE], pattern[BUFFER_SIZE], commands[BUFFER_SIZE];
    char dummy[BUFFER_SIZE];
    art_num i, start, last;
    char *subject, *author, *subj, *ptr, *pptr;
    int scount = 0, kcount = 0, ucount = 0, mcount = 0;
    char *reRet;
    char type;
    int mesg_name;

    newsgroup->max_killed = MAX(newsgroup->max_killed, newsgroup->first - 1);
    newsgroup->max_killed = MAX(newsgroup->max_killed, newsgroup->current - 1);
    
    start = newsgroup->max_killed + 1;
    if (max)
	last = MIN(newsgroup->last, start + max - 1);
    else
	last = newsgroup->last;

    /* XXX don't reprocess global each time, keep in persistent hash table */

    while (fgets(string, sizeof(string), fp) != NULL) {
	mesg_name = newMesgPaneName();

	/*
	 * see if there is a 'THRU artnum' line, if so,
	 * only compare subjects from that article on
	 * XXX need to update THRU
	 */
	if (STREQN(string, "THRU", 4)) {
	    i = atol(&string[5]);
	    /* if THRU is less than current, ignore it */
	    start = MAX(i + 1, start);
	    continue;
	}

	if (*string == '&') {
	    /* 'rn' switch setting, ignore */
	    continue;
	}

	if ((ptr = index(string, '\n')))
	    *ptr = '\0';

	/*
	 * process kill file request should be in the form
	 */
	ptr = string + 1;
	pptr = pattern;

	while (*ptr && ((*ptr != '/') || isQuoted(ptr, string + 1))) {
	   *pptr++ = *ptr;
	   ptr++;
	}
	*pptr = '\0';

	if (!*ptr) {
	    mesgPane(XRN_SERIOUS, mesg_name, MALFORMED_KILL_ENTRY_MSG, string,
		     newsgroup->name, ERROR_REGEX_NOSLASH_MSG);
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
	    mesgPane(XRN_SERIOUS, mesg_name, UNKNOWN_KILL_REGEXP_ERROR_MSG, string);
#else
	    mesgPane(XRN_SERIOUS, mesg_name, KNOWN_KILL_REGEXP_ERROR_MSG, string,
		     reRet);
#endif
	    continue;
	}

	ptr++;	/* skip past the slash */
	(void) strcpy(commands, ptr);
	if ((ptr = index(commands, ':')) == NIL(char)) {
	    mesgPane(XRN_SERIOUS, mesg_name, MALFORMED_KILL_ENTRY_MSG, string,
		     newsgroup->name, ERROR_REGEX_NOCOLON_MSG);
#ifdef SYSV_REGEX
	    FREE(reRet);
#endif
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
	        mesgPane(XRN_INFO, mesg_name, MALFORMED_KILL_ENTRY_MSG, string,
			 newsgroup->name,
                       ERROR_REGEX_UNKNOWN_COMMAND_MSG );
	        break;
	}

	mesg_name = newMesgPaneName();

	for (i = start; i <= last; i++) {
	    art = artStructGet(newsgroup, i, False);

	    /* short cut */
	    if (IS_UNAVAIL(art) ||
		((type == 'j') && IS_READ(art)) ||
		((type == 'm') && IS_UNREAD(art)))
		continue;

	    if (art->subject || art->author) {
		copy = *art;

		subject = art->subject;
		author = art->author;

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
			    SET_READ(&copy);
			    if (index(app_resources.verboseKill, 'j')) {
			        mesgPane(XRN_INFO, mesg_name, KILL_KILLED_MSG,
					 subject);
			    }
			    kcount++;
			    break;

			case 'm':
			    SET_UNREAD(&copy);
			    if (index(app_resources.verboseKill, 'm')) {
				mesgPane(XRN_INFO, mesg_name, KILL_UNREAD_MSG,
					 subject);
			    }
			    mcount++;
			    break;

			case 's':
			    (void) saveArticle(NIL(char), newsgroup, i, False,
					       False);
			    if (index(app_resources.verboseKill, 's')) {
				mesgPane(XRN_INFO, mesg_name, KILL_SAVED_MSG,
					 subject);
			    }
			    scount++;
			    break;

			default:
			    ucount++;
			    break;
		    }
		}

		artStructReplace(newsgroup, &art, &copy, i);
	    }
	}

#ifdef SYSV_REGEX
	FREE(reRet);
#endif
    }

    mesg_name = newMesgPaneName();

#define printcount(c,cmd,m) \
    if (c && ((! cmd) || index(app_resources.verboseKill, cmd))) { \
	mesgPane(XRN_INFO, mesg_name, m, c, ((c==1) ? "" : NOT_ONE_MSG), \
		 newsgroup->name); \
    }

    printcount(kcount, 'j', COUNT_KILLED_MSG);
    printcount(mcount, 'm', COUNT_UNREAD_MSG);
    printcount(scount, 's', COUNT_SAVED_MSG);
    printcount(ucount, '\0', COUNT_MATCHED_MSG);

#undef printcount

    return;
}

    
/*
 * mark articles as read if in the kill files
 */
static Boolean checkKillFiles _ARGUMENTS((struct newsgroup *,
					  /* Boolean */ int));
    
static Boolean checkKillFiles(newsgroup, prefetching)
    struct newsgroup *newsgroup;
    Boolean prefetching;
{
    FILE *gfp, *lfp;
    static int chunk_size;
    long start, end;
    static struct newsgroup *last_group = 0;
    char dummy[LABEL_SIZE];
    Boolean finished = True;

#ifdef DEBUG
    fprintf(stderr, "checkKillFiles(%s, %d)\n", newsgroup->name, prefetching);
#endif

    gfp = fopen(globalKillFile(), "r");
    lfp = fopen(localKillFile(newsgroup, 0), "r");

    if (! (gfp || lfp))
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
    
    if (gfp) {
	killArticles(newsgroup, gfp, prefetching ? chunk_size : 0);
	(void) fclose(gfp);
    }

    if (lfp) {
	killArticles(newsgroup, lfp, prefetching ? chunk_size : 0);
	(void) fclose(lfp);
    }

    if (prefetching) {
	end = time(0);
	newsgroup->max_killed =
	    MIN(newsgroup->max_killed + chunk_size, newsgroup->last);
	adjustPrefetchChunk(end - start, PREFETCH_CHUNK_TIME,
			    &chunk_size);
	if (newsgroup->max_killed < newsgroup->last)
	    finished = False;
    }
    else
	newsgroup->max_killed = newsgroup->last;

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
#ifdef DEBUG
    fprintf(stderr, "finishPrefetch()\n");
#endif

    FinishingPrefetch = True;
    while (PrefetchingGroup)
	(*PrefetchingProc)(PrefetchingGroup);
    FinishingPrefetch = False;

#ifdef DEBUG
    fprintf(stderr, "finishPrefetch() done\n");
#endif
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


/*
 * The reason this is divided into "stages" is so that it can be used
 * in a toolkit work procedure.  The stages allow the work to be
 * divided up so a single work procedure invocation doesn't take too long.
 */
static Boolean setUpGroupIncremental _ARGUMENTS((struct newsgroup *, int,
						 /* Boolean */ int,
						 /* Boolean */ int,
						 /* Boolean */ int));

static Boolean setUpGroupIncremental(newsgroup, stage, update_last,
				     kill_files, unread_only)
    struct newsgroup *newsgroup;
    int stage;
    Boolean update_last, kill_files, unread_only;
{
    Boolean finished = True;
    static int chunk_size = PREFETCH_CHUNK;

#ifdef DEBUG
    fprintf(stderr, "setUpGroupIncremental(%s, %d, %d, %d, %d)\n",
	    newsgroup->name, stage, update_last, kill_files, unread_only);
#endif

    if (PrefetchingGroup && (stage == 0)) {
	if ((PrefetchingGroup == newsgroup) && kill_files && unread_only)
	    finishPrefetch();
	else
	    cancelPrefetch();
    }

    if (PrefetchedGroup && (newsgroup != PrefetchedGroup)) {
	cancelPrefetch();
    }

    if (! PrefetchedGroup) {
	if (! stage || stage == 1) {
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
	    if (getgroup(newsgroup, &first, &last, &number))
		goto done;
	    articleArrayResync(newsgroup, first,
			       update_last ? last : newsgroup->last, number);

	    setCurrentArticle(newsgroup, unread_only);
	}
	if (! EMPTY_GROUP(newsgroup)) {
	    long start_time, end_time;

	    if ((stage > 1) && (stage < 5))
		start_time = time(0);

	    if (! stage || stage == 2)
		finished = getsubjectlist(newsgroup, newsgroup->current,
					  newsgroup->last, unread_only,
					  (stage && ! FinishingPrefetch)
					  ? chunk_size : 0);
	    if (! stage || stage == 3)
		finished = getauthorlist(newsgroup, newsgroup->current,
					 newsgroup->last, unread_only,
					 (stage && ! FinishingPrefetch)
					 ? chunk_size : 0);
	    if (! stage || stage == 4)
		finished = getlineslist(newsgroup, newsgroup->current,
					newsgroup->last, unread_only,
					(stage && ! FinishingPrefetch)
					? chunk_size : 0);
	    if (! finished) { /* that means it was a full chunk */
		end_time = time(0);
		adjustPrefetchChunk(end_time - start_time,
				    PREFETCH_CHUNK_TIME, &chunk_size);
		if (chunk_size >= PREFETCH_FAST_CHUNK)
		    FastServer = True;
		else
		    FastServer = False;
	    }
	    if (! stage || stage == 5) {
		if ((app_resources.killFiles == TRUE) && kill_files)
		    finished = checkKillFiles(newsgroup,
					      (stage && ! FinishingPrefetch)
					      ? True : False);
	    
		setCurrentArticle(newsgroup, unread_only);
	    }
	}
    }

    PrefetchedGroup = 0;

#ifndef NO_IMMEDIATE_WORK_PROC_PREFETCH
    if (stage == 0)
	prefetchNextGroup(newsgroup);
#endif

  done:
#ifdef DEBUG
    fprintf(stderr, "setUpGroupIncremental(%s, %d, %d, %d, %d) = %d\n",
	    newsgroup->name, stage, update_last, kill_files, unread_only
	    finished);
#endif
    return finished;
}

static void setUpGroup _ARGUMENTS((struct newsgroup *, /* Boolean */ int,
				   /* Boolean */ int, /* Boolean */ int));

static void setUpGroup(newsgroup, update_last, kill_files, unread_only)
    struct newsgroup *newsgroup;
    Boolean update_last, kill_files, unread_only;
{
#ifdef DEBUG
    fprintf(stderr, "setUpGroup(%s, %d, %d, %d)\n", newsgroup->name,
	    update_last, kill_files, unread_only);
#endif
    (void) setUpGroupIncremental(newsgroup, 0, update_last,
				 kill_files, unread_only);
#ifdef DEBUG
    fprintf(stderr, "setUpGroup(%s, %d, %d, %d) done\n", newsgroup->name,
	    update_last, kill_files, unread_only);
#endif
}


static struct newsgroup * findNewsGroupMatch _ARGUMENTS((char *));

static struct newsgroup * findNewsGroupMatch(name)
    char *name;
{
    static char *reRet;
    int i;

    /* no single character queries -- too confusing */
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
	    FREE(reRet);
	    return newsgroup;
	}
#else
	if (re_exec(newsgroup->name)) {
	    return newsgroup;
	}
#endif
	    
    }

#ifdef SYSV_REGEX
    FREE(reRet);
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
 * subscribe to a newsgroup
 *
 *   returns: void
 *
 */
void subscribe()
{
    struct newsgroup *newsgroup = CurrentGroup;

    if (!IS_SUBSCRIBED(newsgroup)) {
	art_num first, last;
	int number;

	SET_SUB(newsgroup);
	/*
	  Update the first and last article numbers for the newsgroup,
	  since "Rescan" doesn't update groups we're not subscribed to.
	  */
	if (! getgroup(newsgroup, &first, &last, &number))
	    articleArrayResync(newsgroup, first, last, number);
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
    long unread, next_unread;
    ng_num number;
    struct newsgroup *nextnewsgroup;
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
			 newsgroup->current, newsgroup->name,
			 nextnewsgroup->name, next_unread,
			 (next_unread == 1) ? "" : NOT_ONE_MSG);
	} else {
          (void) sprintf(dummy, QUEST_ART_UNREAD_NEXT_STRING,
			 newsgroup->current, newsgroup->name, unread,
			 nextnewsgroup->name, next_unread,
			 (next_unread == 1) ? "" : NOT_ONE_MSG);
	}
    } else {
	if (unread <= 0) {
          (void) sprintf(dummy, QUEST_ART_NOUNREAD_NONEXT_STRING,
			   newsgroup->current, newsgroup->name);
	} else {
          (void) sprintf(dummy, QUEST_ART_UNREAD_NONEXT_STRING,
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
	    struct article *art;

	    artListSet(newsgroup);
	    art = artStructGet(newsgroup, number, True);
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
int getArticle(filename, question)
    char **filename;
    char **question;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *art = artStructGet(newsgroup, newsgroup->current, True);
    int header, rotation;
    int	xlation = 0;

#ifdef DEBUG
    fprintf(stderr, "getArticle(%s)\n", newsgroup->name);
#endif

    if (IS_UNFETCHED(art)) {
	/* get the article and handle unavailable ones.... */
	header = (IS_ALL_HEADERS(art) ? FULL_HEADER : NORMAL_HEADER);
	rotation = (IS_ROTATED(art) ? ROTATED : NOT_ROTATED);
#ifdef XLATE
	xlation = (IS_XLATED(art) ? XLATED : NOT_XLATED);
#endif
	if (! (art->filename = getarticle(newsgroup, CurrentGroup->current, 0,
					  header, rotation, xlation))) {
	    CLEAR_SUBJECT(art);
	    CLEAR_AUTHOR(art);
	    CLEAR_LINES(art);
	    SET_UNAVAIL(art);
	    artStructSet(newsgroup, &art);
	    return XRN_ERROR;
	}
	SET_FETCHED(art);
    } else {
	/* verify that the file still exists */
	if (access(art->filename, R_OK) == -1) {
	    /* refetch the file */
	    SET_UNFETCHED(art);
	    artStructSet(newsgroup, &art);
	    return getArticle(filename, question);
	}
    }
    
    *filename = art->filename;
    if (IS_UNMARKED(art)) {
	SET_READ(art);
    }
    *question = buildQuestion(newsgroup);
    handleXref(newsgroup, newsgroup->current);

    artStructSet(newsgroup, &art);
    return XRN_OKAY;
}


static int toggleArtAttribute _ARGUMENTS((char **, char **, int));

static int toggleArtAttribute(filename, question, attribute)
    char **filename;
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

    ret = getArticle(filename, question);
    if (ret != XRN_OKAY) {
	mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, newsgroup->current);
    }
    return ret;
}

int toggleHeaders(filename, question)
    char **filename;
    char **question;
{
    return toggleArtAttribute(filename, question, ART_ALL_HEADERS);
}

int toggleRotation(filename, question)
    char **filename;
    char **question;
{
    return toggleArtAttribute(filename, question, ART_ROTATED);
}

#ifdef XLATE
int toggleXlation(filename, question)
    char **filename;
    char **question;
{
    return toggleArtAttribute(filename, question, ART_XLATED);
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
	    /* if the article can be fetched, mark it so */
	    if ((art->filename = getarticle(newsgroup, newsgroup->current, 0,
					    NORMAL_HEADER, NOT_ROTATED,
					    NOT_XLATED)))
		SET_FETCHED(art);
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

    (void) XtAppAddWorkProc(TopContext,
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

    if (PrefetchStage <= 5) {
#ifdef DEBUG
	fprintf(stderr,
		"prefetchSetupUnreadGroup[%d] running stage %d on %s\n",
		mesg_name, PrefetchStage, newsgroup->name);
#endif
	if (setUpGroupIncremental(newsgroup, PrefetchStage,
				  app_resources.rescanOnEnter, True, True))
	    PrefetchStage++;
	ret = False;
	goto done;
    }
    else {
	if (! FinishingPrefetch)
	    redrawNewsgroupTextWidget(newsgroup->name, False, False);
	(void) XtAppAddWorkProc(TopContext,
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
	    PrefetchStage = 1;
	    (void) sprintf(msg, PREFETCHING_MSG, newsgroup->name);
#ifdef DEBUG
	    fprintf(stderr, "prefetchNextGroup[%d] continuing with %s\n",
		    mesg_name, newsgroup->name);
#endif
	    (void) XtAppAddWorkProc(TopContext,
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

	if (IS_UNFETCHED(art) && 
	    (art->filename = getarticle(newsgroup, artnum, 0,
					NORMAL_HEADER, NOT_ROTATED,
					NOT_XLATED))) {
	    SET_FETCHED(art);
	    SET_STRIPPED_HEADERS(art);
	    SET_UNROTATED(art);
	    SET_UNXLATED(art);
	}

	artStructSet(newsgroup, &art);
    }
}



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
    struct article *art, copy;
    art_num artnum;
#ifndef FIXED_C_NEWS_ACTIVE_FILE
    int number;
#endif

    if (newsgroup->last == 0) {
	return;
    }

    if (EMPTY_GROUP(newsgroup)) {
	artListFree(newsgroup);
	return;
    }

    if (newsgroup->nglist == NIL(struct list)) {
	return;
    }

    if (!IS_SUBSCRIBED(newsgroup)) {
	artListFree(newsgroup);
	return;
    }

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

    artListSet(newsgroup);

    /* process the .newsrc line */

    for (item = newsgroup->nglist; item != NIL(struct list); item = item->next) {
	switch (item->type) {
	case SINGLE:
	    if (item->contents.single > newsgroup->last) {
		/* something really bad has happened, reset */
		mesgPane(XRN_SERIOUS, 0, ART_NUMBERING_PROBLEM_MSG,
			 newsgroup->name);
		artListFree(newsgroup);
		artListSet(newsgroup);
		lsDestroy(newsgroup->nglist);
		newsgroup->nglist = NIL(struct list);
		newsgroup->max_killed = 0;
		return;
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
		/* something really bad has happened, reset */
		mesgPane(XRN_SERIOUS, 0, ART_NUMBERING_PROBLEM_MSG,
			 newsgroup->name);
		artListFree(newsgroup);
		artListSet(newsgroup);
		lsDestroy(newsgroup->nglist);
		newsgroup->nglist = NIL(struct list);
		newsgroup->max_killed = 0;
		return;
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

    lsDestroy(newsgroup->nglist);
    newsgroup->nglist = NIL(struct list);
    
    return;
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
    char *ptr;
    struct newsgroup *newsgroup;
    ng_num i;
    
    if (!avl_lookup(NewsGroupTable, newGroup, &ptr)) {
	mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_MSG, newGroup);
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
	mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_MSG, newGroup);
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
	mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_MSG, newGroup);
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
	mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_MSG, afterGroup);
	return BAD_GROUP;
    }
    
    ng = (struct newsgroup *) ptr;
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
          ehErrorExitXRN(ERROR_OUT_OF_MEM_MSG);
	}

	while (avl_gen(gen, &key, &value)) {
	    struct newsgroup *newsgroup = (struct newsgroup *) value;
	    
	    (void) sprintf(buffer, "%-60s%s",
			   newsgroup->name,
                         (IS_SUBSCRIBED(newsgroup) ? SUBED_MSG : UNSUBED_MSG));
	    ar[count++] = XtNewString(buffer);
	    bytes += strlen(buffer);
	}
	avl_free_gen(gen);
	
    } else {
	for (i = 0; i < MaxGroupNumber; i++) {
	    struct newsgroup *newsgroup = (struct newsgroup *) Newsrc[i];
	    
	    (void) sprintf(buffer, "%-60s%s",
			   newsgroup->name,
                         (IS_SUBSCRIBED(newsgroup) ? SUBED_MSG : UNSUBED_MSG));
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
  If art_num is greater than 0, format a single line in the subject
  index, and return its length (including the final newline).  If
  art_num is less than or equal to 0, just return what the length
  would be if a subject line were formatted.
  */
static int subjectIndexLine _ARGUMENTS((char *, struct newsgroup *, art_num));

static int subjectIndexLine(out, newsgroup, artNum)
    char *out;
    struct newsgroup *newsgroup;
    art_num artNum;
{
    int subLength, lineLength;
    struct article *art;
    char *subject, *lines, *author;
    int lineWidth;

    lineLength = MAX(50, app_resources.indexLineLength);
    lineLength = MIN(LINE_LENGTH - 1, lineLength); /* for the NULL */

    if (artNum <= 0)
	return lineLength;

    artListSet(newsgroup);
    art = artStructGet(newsgroup, artNum, False);

    subLength = lineLength -
	2 /* spaces after subject, or spaces before and after line count */ -
	20 /* author */ -
	1 /* newline */;
    if (app_resources.displayLineCount) {
	subLength -= 6 /* number of lines */;
	lines = art->lines ? art->lines : "[?]";
	lineWidth = 6;
    }
    else {
	lines = "";
	lineWidth = 0;
    }

    subject = art->subject;

    author = art->author ? art->author : "(none)";

    (void) sprintf(out, "%*.*s %*.*s %-20.20s\n", -subLength, subLength,
		   subject, lineWidth, lineWidth, lines, author);
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



/*
 * build and return the subjects string
 */
static char * getUnSortedSubjects _ARGUMENTS((int, art_num));

static char * getUnSortedSubjects(mode, artNum)
    int mode;
    art_num artNum;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *art;
    int lineLength;
    art_num i;
    char *start, *end;

    if (EMPTY_GROUP(newsgroup)) {
	return XtNewString("");
    }

    artNum = MAX(newsgroup->first, artNum);

    lineLength = subjectIndexLine(0, newsgroup, 0);
    
    NextPreviousArticle = artNum - 1;

    if ((newsgroup->last - artNum + 1) < 0) {
	(void) sprintf(error_buffer, "Active File Error: last - artNum + 1 < 0 (%s)\n",
			       newsgroup->name);
	ehErrorExitXRN(error_buffer);
    }
	
    start = ARRAYALLOC(char, ((newsgroup->last - artNum + 1) * lineLength + 1));
    end = start;
    *start = '\0';

    for (i = artNum; i <= newsgroup->last; i++) {
	art = artStructGet(newsgroup, artNum, False);

	/* canceled and empty articles will not have a subject entry */
	if (art->subject && ((mode == ALL) || (IS_UNREAD(art) && IS_AVAIL(art)))) {
	    (void) subjectIndexLine(end, newsgroup, i);
	    end += lineLength;
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


#if defined(__osf__) || defined(_POSIX_SOURCE) || defined(SOLARIS)
static int pteCompare _ARGUMENTS((CONST void *, CONST void *));
#else
static int pteCompare _ARGUMENTS((void *, void *));
#endif

static int pteCompare(a, b)
#if defined(__osf__) || defined(_POSIX_SOURCE) || defined(SOLARIS)
    CONST void *a, *b;
#else
    void *a, *b;
#endif
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

static char * getSortedSubjects _ARGUMENTS((int, art_num));

static char * getSortedSubjects(mode, artNum)
    int mode;
    art_num artNum;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *art;
    int lineLength;
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

    artNum = MAX(newsgroup->first, artNum);

    lineLength = subjectIndexLine(0, newsgroup, 0);

    tree = avl_init_table(utSubjectCompare);
    if (! tree) {
      ehErrorExitXRN(ERROR_OUT_OF_MEM_MSG);
    }

    NextPreviousArticle = artNum - 1;

    if ((newsgroup->last - artNum + 1) < 0) {
	(void) sprintf(error_buffer, "Active File Error: last - artNum + 1 < 0 (%s)\n",
			       newsgroup->name);
	ehErrorExitXRN(error_buffer);
    }
	
    /* 
     * build the subject groups 
     */
    for (i = artNum; i <= newsgroup->last; i++) {
	char buffer[LINE_LENGTH];

	art = artStructGet(newsgroup, i, False);

	/* canceled and empty articles will not have a subject entry */
	if (art->subject && ((mode == ALL) || (IS_UNREAD(art) && IS_AVAIL(art)))) {
	    (void) subjectIndexLine(buffer, newsgroup, i);
	    (void) strncpy(curSub, getSubject(i), sizeof(curSub));

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
		    ehErrorExitXRN(ERROR_OUT_OF_MEM_MSG);
		}
	    }
	}
    }

    i = 0;
    treeSize = avl_count(tree);
    pteArray = ARRAYALLOC(struct entry *, treeSize);
    gen = avl_init_gen(tree, AVL_FORWARD);
    if (! gen) {
      ehErrorExitXRN(ERROR_OUT_OF_MEM_MSG);
    }
    while (avl_gen(gen, &key, &ptr)) {
	pteArray[i++] = (struct entry *) ptr;
    }
    avl_free_gen(gen);

    /* sort by article number */
    qsort((char *) pteArray, treeSize, sizeof(struct pte *), pteCompare);

    start = ARRAYALLOC(char, ((newsgroup->last - artNum + 1) * lineLength + 1));
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



char * getSubjects(mode, artNum)
    int mode;
    art_num artNum;
{
    if (app_resources.sortedSubjects) {
	return getSortedSubjects(mode, artNum);
    } else {
	return getUnSortedSubjects(mode, artNum);
    }
}


/*
 * set the internal pointers to a particular article
 */
void gotoArticle(article)
    art_num article;
{
    CurrentGroup->current = article;
}

/*
  Return the first article in the current newsgroup.
  */
art_num firstArticle()
{
    return CurrentGroup->first;
}

/*
  Return the current article in the current newsgroup.
  */
art_num currentArticle()
{
    return CurrentGroup->current;
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

    /* A space separates the article number from the subject line, but
       there also may be spaces in the first two columns, as well as
       more in subsequent columns if the number of digits in the
       unread article numbers are not all the same. */
    for (start += 2; *start == ' '; start++) /* empty */;
    start = index(start, ' ');
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
    art_num article;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *art;

    artListSet(newsgroup);

    art = artStructGet(newsgroup, article, False);
    return strip(art->subject, TRUE);
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
    int lineLength;
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *art;
    static char buffer[BUFFER_SIZE];

    lineLength = subjectIndexLine(0, newsgroup, 0);

#ifdef DEBUG
    fprintf(stderr, "getPrevSubject(%s)\n", newsgroup->name);
#endif

    /* search for the next available article in the reverse direction */
    for ( ; NextPreviousArticle >= newsgroup->first; NextPreviousArticle--) {
	art = artStructGet(newsgroup, NextPreviousArticle, False);

	if (IS_UNAVAIL(art))
	    continue;

	/* get the subject (and author) if it does not already exist */
	if (! art->subject) {
	    /* get the subject and a few more */
	    getsubjectlist(newsgroup,
			   MAX(newsgroup->first, NextPreviousArticle - SUBJECTS),
			   NextPreviousArticle, False, 0);
	    art = artStructGet(newsgroup, NextPreviousArticle, False);
	}

	if (! art->author) {
	    getauthorlist(newsgroup,
			  MAX(newsgroup->first, NextPreviousArticle - SUBJECTS),
			   NextPreviousArticle, False, 0);
	    art = artStructGet(newsgroup, NextPreviousArticle, False);
	}

	if (! art->lines) {
	    getlineslist(newsgroup,
			 MAX(newsgroup->first, NextPreviousArticle - SUBJECTS),
			 NextPreviousArticle, False, 0);
	    art = artStructGet(newsgroup, NextPreviousArticle, False);
	}

	if (art->subject) {
	    (void) subjectIndexLine(buffer, newsgroup, NextPreviousArticle);
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

    getsubjectlist(newsgroup, art, newsgroup->last, False, 0);
    getauthorlist(newsgroup, art, newsgroup->last, False, 0);
    getlineslist(newsgroup, art, newsgroup->last, False, 0);
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
 *			it's unsubscribed. 
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
	    if (flags & ENTER_SUBSCRIBE) {
		SET_SUB(newsgroup);
	    }
	    updateArticleArray(newsgroup);
	}
	else {
	    return XRN_UNSUBBED;
	}
    }

    if (flags & ENTER_SETUP) {
	if (app_resources.rescanOnEnter)
	    setUpGroup(newsgroup, True, False, flags & ENTER_UNREAD);

	if (EMPTY_GROUP(newsgroup)) {
	    return XRN_NOMORE;
	}

	setUpGroup(newsgroup, False, True, flags & ENTER_UNREAD);
	/*
	  We need to do this again here, even though setUpGroup() does
	  it, because if the group is already prefetched, and *then*
	  the first unread article in it is marked read while reading
	  another group, setUpGroup() won't reset the first unread
	  article (because it doesn't do anything if the group is
	  already prefetched).
	  */
	setCurrentArticle(newsgroup, flags & ENTER_UNREAD);

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

	groupSnapshotSave(newsgroup);
    }

    CurrentGroup = newsgroup;

    return ret;
}


char **parseRegexpList(list, list_name)
    char *list, *list_name;
{
    char *p, *q;
    int n;
    char **l;
    
    if (! list) {
	return 0;
    }

    /* Count the number of elements in the list, if there are no regexp errors */
    for (p = q = XtNewString(list), n = 0; strtok(p, ", \t\n"); p = 0) n++;

    FREE(q);

    if (! n) {
	return 0;
    }
    n++; /* to null-terminate */
    
    l = ARRAYALLOC(char *, n);

    /* Now build the list */
    for ((p = XtNewString(list)), n = 0; (q = strtok(p, ", \t\n")); p = 0) {
#ifdef SYSV_REGEX
	if (! (l[n] = regcmp(q, 0))) {
	    mesgPane(XRN_SERIOUS, 0, UNKNOWN_LIST_REGEXP_ERROR_MSG, list_name, q);
	    continue;
	}
#else
	l[n] = q;
	if ((q = re_comp(l[n]))) {
	    mesgPane(XRN_SERIOUS, 0, KNOWN_LIST_REGEXP_ERROR_MSG, list_name, l[n], q);
	    continue;
	}
#endif
	n++;
    }

    if (! n) {
	FREE(l);
	FREE(p);
	return 0;
    }

    l[n++] = 0;

    return l;
}
