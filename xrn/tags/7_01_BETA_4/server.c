
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Header: /d/src/cvsroot/xrn/server.c,v 1.25 1994-11-18 14:34:07 jik Exp $";
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
 * server.c: routines for communicating with the NNTP remote news server
 *
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include "avl.h"
#include "news.h"
#include "mesg.h"
#include "error_hnds.h"
#include "resources.h"
#include "server.h"
#include "internals.h"
#include "mesg_strings.h"
#include "clientlib.h"
#include "xmisc.h"
#include "xthelper.h"

#if defined(sun) && defined(sparc) && !defined(SOLARIS)
#include <vfork.h>
#endif

#ifdef INEWS
#include <sys/stat.h>
#endif

extern int errno;

#define BUFFER_SIZE 1024
#define MESSAGE_SIZE 1024

int ServerDown = 0;
static char mybuf[MESSAGE_SIZE+100] = 
	"The news server is not responding correctly, aborting\n";

static struct newsgroup *currentNewsgroup = 0;
#define SETNEWSGROUP(n) (((n) == currentNewsgroup) ? 0 : getgroup((n), 0, 0, 0))

/*
 * get data from the server (active file, article)
 *
 *  on error, sets 'ServerDown'
 *
 *   returns: void
 */
static void get_data_from_server _ARGUMENTS((char *, int));

static void get_data_from_server(str, size)
    char *str;     /* string for message to be copied into */
    int size;      /* size of string                       */
{
    if (get_server(str, size) < 0) {
	ServerDown = 1;
    } else {
	ServerDown = 0;
    }
    return;
}

static void check_time_out _ARGUMENTS((char *, char *, int));

static void check_time_out(command, response, size)
    char *command;  /* command to resend           */
    char *response; /* response from the command   */
    int size;       /* size of the response buffer */
{
    /*
     * try to recover from a timeout
     *
     *   this assumes that the timeout message stays the same
     *   since the error number (503) is used for more than just
     *   timeout
     *
     *   Message is:
     *     503 Timeout ...
     */

    int old = ActiveGroupsCount;

    if (ServerDown || STREQN(response, "503 Timeout", 11)) {

	mesgPane(XRN_SERIOUS, LOST_CONNECT_ATTEMPT_RE_MSG);
	start_server(NIL(char));
	mesgPane(XRN_INFO, RECONNECTED_MSG);

	/*
	 * reissue the getactive command to update internal structures 
	 *   XXX what happens if a new group comes up???
	 *   XXX is the system in a state where resizing the article
	 *       arrays will be okay???
	 */

	getactive();
	
	if (ActiveGroupsCount > old) {
	    /* new newsgroups were found, allocate a bigger array */
	    Newsrc = (struct newsgroup **) XtRealloc((char *) Newsrc, (unsigned) (sizeof(struct newsgroup *) * ActiveGroupsCount));
	}

#ifndef FIXED_ACTIVE_FILE
	badActiveFileCheck();
#endif

	/*
	 * if it was an ARTICLE or XHDR or HEAD command, then you must get the
	 * server into the right state (GROUP mode), so resend the last
	 * group command
	 */
	if (currentNewsgroup) {
	    struct newsgroup *wanted = currentNewsgroup;
	    currentNewsgroup = 0;
	    if (SETNEWSGROUP(wanted)) {
		return;
	    }
	    currentNewsgroup = wanted;
	    /* XXX should do some processing of changed first/last numbers */
	}
	
	put_server(command);
	get_data_from_server(response, size);
    }
    
    return;
}

/*
 * retrieve article number 'artnumber' in the current group, update structure
 *
 *   returns:  filename that the article is stored in or NIL(char) if
 *             the article is not avaiable
 *
 */
char * getarticle(newsgroup, artnumber, position, header, rotation, xlation)
    struct newsgroup *newsgroup;
    art_num artnumber;  /* # of article in the current group to retrieve */
    long *position;     /* byte position of header/body seperation       */
    int header, rotation, xlation;
{
    char command[MESSAGE_SIZE], message[MESSAGE_SIZE], *msg;
#ifdef REALLY_USE_LOCALTIME
    char temp[MESSAGE_SIZE];
#endif
    FILE *articlefp;
    char *filename, *ptr;
#ifdef VMS
    char dummy[MAXPATHLEN];
#endif
    char field[BUFFER_SIZE];
    int byteCount = 0, lineCount = 0;
    int error = 0;
    int last_stripped = 0;

    *position = 0;

    if (SETNEWSGROUP(newsgroup)) {
	return 0;
    }

    /* send ARTICLE */
    (void) sprintf(command, "ARTICLE %ld", artnumber);
    put_server(command);
    get_data_from_server(message, sizeof(message));

    check_time_out(command, message, sizeof(message));

    if (*message != CHAR_OK) {
	/* can't get article */
	return(NIL(char));
    }

#ifndef VMS
    if ((filename = utTempnam(app_resources.tmpDir, "xrn")) == NIL(char)) {
	mesgPane(XRN_SERIOUS, CANT_TEMP_NAME_MSG);
	/* read till end of article */
	do {
	    get_data_from_server(message, sizeof(message));
	} while ((message[0] != '.') || (message[1] != '\0'));
	return(NIL(char));
    }
#else
    (void) sprintf(dummy, "%sxrn%ld-XXXXXX", app_resources.tmpDir, artnumber);
    if ((filename = mktemp(dummy)) == NIL(char)) {
	mesgPane(XRN_SERIOUS, CANT_TEMP_NAME_MSG);
	/* read till end of article */
	do {
	    get_data_from_server(message, sizeof(message));
	} while ((message[0] != '.') || (message[1] != '\0'));
	return(NIL(char));
    }
    filename = XtNewString(filename);
#endif

    if ((articlefp = fopen(filename, "w")) == NULL) {
	mesgPane(XRN_SERIOUS, CANT_CREATE_TEMP_MSG, filename, errmsg(errno));
	/* read till end of article */
	do {
	    get_data_from_server(message, sizeof(message));
	} while ((message[0] != '.') || (message[1] != '\0'));
	FREE(filename);
	return(NIL(char));
    }

    for (;;) {
	get_data_from_server(message, sizeof(message));

	/* the article is ended by a '.' on a line by itself */
	if ((message[0] == '.') && (message[1] == '\0')) {
	    /* check for a bogus message */
	    if (byteCount == 0) {
		(void) fclose(articlefp);
		(void) unlink(filename);
		FREE(filename);
		return(NIL(char));
	    }
	    break;
	}

	msg = &message[0];

	/* find header/body seperation */
	if (*position == 0) {
	    if (*msg == '\0') {
		*position = byteCount;
	    }
	}
	      
	if (*msg == '.') {
	    msg++;
	}

	if (*msg != '\0') {
	    /* strip leading ^H */
	    while (*msg == '\b') {
		msg++;
	    }
	    /* strip '<character>^H' */
	    for (ptr = index(msg + 1, '\b'); ptr != NIL(char); ptr = index(ptr, '\b')) {
		if (ptr - 1 < msg) {
		    /* too many backspaces, kill all leading back spaces */
		    while (*ptr == '\b') {
		        (void) strcpy(ptr, ptr + 1);
			ptr++;
		    }
		    break;
		}
		(void) strcpy(ptr - 1, ptr + 1);
		ptr--;
	    }

#ifdef REALLY_USE_LOCALTIME
  	    if (app_resources.displayLocalTime && !strncmp(msg, "Date: ", 6)) {
		  tconvert(temp, msg+6);
		  (void) strcpy(msg+6, temp);
	    }
#endif
	    /* strip the headers */
	    if ((*position == 0) && (header == NORMAL_HEADER)) {
		if ((*msg == ' ') || (*msg == '\t')) { /* continuation line */
		    if (last_stripped)
			continue;
		}
		else {
		    if ((ptr = index(msg, ':')) == NIL(char)) {
			continue; /* weird header line, skip */
		    }
		    if (*(ptr+1) == '\0') {
			continue; /* empty field, skip */
		    }
		    (void) strncpy(field, msg, (int) (ptr - msg));
		    field[(int) (ptr - msg)] = '\0';
		    utDowncase(field);
		    if (avl_lookup(app_resources.headerTree, field, &ptr)) {
			if (app_resources.headerMode == STRIP_HEADERS) {
			    last_stripped = 1;
			    continue;
			}
			else
			    last_stripped = 0;
		    } else {
			if (app_resources.headerMode == LEAVE_HEADERS) {
			    last_stripped = 1;
			    continue;
			}
			else
			    last_stripped = 0;
		    }
		}
	    }

	    /* handle rotation of the article body */
	    if ((rotation == ROTATED) && (*position != 0)) {
		for (ptr = msg; *ptr != '\0'; ptr++) {
		    if (isalpha(*ptr)) {
			if ((*ptr & 31) <= 13) {
			    *ptr = *ptr + 13;
			} else {
			    *ptr = *ptr - 13;
			}
		    }
		}
	    }

#ifdef XLATE
	    /* handle translation of the article body */
	    if ((xlation == XLATED) && (*position != 0))
		utXlate(msg);
#endif /* XLATE */

	    /* handle ^L (poorly?) */
	    if (*msg == '\014') {
		int i, lines;
		lines = articleLines();
		lines -= lineCount % lines;
		for (i = 0; i < lines; i++) {
		    if (putc('\n', articlefp) == EOF) {
			error++;
			break;
		    }
		}
		if (error) {
		    break;
		}
		byteCount += lines;
		lineCount += lines;
		msg++;
	    }
	    if (fputs(msg, articlefp) == EOF) {
		error++;
		break;
	    }
	}
	if (putc('\n', articlefp) == EOF) {
	    error++;
	    break;
	}
	byteCount += utStrlen(msg) + 1;
	lineCount++;
    }

    if (!error) {
	if (fclose(articlefp) == 0) {
	    return(filename);
	}
    } else {
	(void) fclose(articlefp);
	/* read till end of article */
	do {
	    get_data_from_server(message, sizeof(message));
	} while ((message[0] != '.') || (message[1] != '\0'));
    }
    mesgPane(XRN_SERIOUS, ERROR_WRITING_FILE_MSG, filename, errmsg(errno));
    (void) unlink(filename);
    FREE(filename);
    return(NIL(char));
}

/*
 * enter a new group and get its statistics (and update the structure)
 *   allocate an array for the articles and process the .newsrc article
 *   info for this group
 *
 *   returns: NO_GROUP on failure, 0 on success
 *
 */
int getgroup(newsgroup, first, last, number)
    struct newsgroup *newsgroup;     /* group name                 */
    art_num *first; /* first article in the group */
    art_num *last;  /* last article in the group  */
    int *number;    /* number of articles in the group, if 0, first
		       and last are bogus */
{
    char command[MESSAGE_SIZE], message[MESSAGE_SIZE];
    char group[GROUP_NAME_SIZE];
    static long code, num, count, frst, lst;

    if (! newsgroup) {
	newsgroup = currentNewsgroup;
    }

    if (! newsgroup) {
	/* this shouldn't ever happen, but let's be cautious */
	ehErrorExitXRN("getgroup(NIL) called with no currentNewsgroup");
    }

    if (newsgroup != currentNewsgroup) {
	(void) sprintf(command, "GROUP %s", newsgroup->name);
	put_server(command);
	get_data_from_server(message, sizeof(message));

	check_time_out(command, message, sizeof(message));
    
	if (*message != CHAR_OK) {
	    if (atoi(message) != ERR_NOGROUP) {

		(void) strcat(mybuf, "        Request was: ");
		(void) strcat(mybuf, command);
		(void) strcat(mybuf, "\n");
		(void) strcat(mybuf, "        Failing response was: ");
		(void) strcat(mybuf, message);
		ehErrorExitXRN(mybuf);
	    }
	    mesgPane(XRN_SERIOUS, NO_SUCH_NG_DELETED_MSG, newsgroup->name);
	
	    /* remove the group from active use ??? */
	
	    return(NO_GROUP);
	}

	currentNewsgroup = newsgroup;

	/* break up the message */
#if GROUP_NAME_SIZE <= 127
	"GROUP_NAME_SIZE is too small" /* this will produce a compilation */
	     /* error */
#endif
	count = sscanf(message, "%ld %ld %ld %ld %127s",
		       &code, &num, &frst, &lst, group);
	assert(count == 5);
    }
    
    if (number != NIL(int)) {
	*number = num;
    }
    if (first != NIL(art_num)) {
	*first = frst;
    }
    if (last != NIL(art_num)) {
	*last = lst;
    }

    return(0);
}

/*
 * get a list of all active newsgroups and create a structure for each one
 *
 *   returns: void
 */
void getactive()
{
    char command[MESSAGE_SIZE], message[MESSAGE_SIZE], group[GROUP_NAME_SIZE];
    char type[MESSAGE_SIZE];
    struct newsgroup *newsgroup;
    art_num first, last;
    char *ptr;
    static char **re_list;
    char **re_ptr;
    static int inited = 0;

    if (! inited) {
	re_list = parseRegexpList(app_resources.ignoreNewsgroups,
				  "ignoreNewsgroups");
	inited++;
    }

#ifdef XRN_PREFETCH
    cancelPrefetch();
#endif
    /*
     * It *is* necessary to reset currentNewsgroup to 0 when getactive
     * is called, even though the NNTP server does not forget its idea
     * of the current newsgroup when a LIST command is issued.  To
     * understand why this is so, imagine the following sequence of
     * events:
     *
     * 1) User reads newsgroup foo.bar.
     * 2) User exits from newsgroup, but currentNewsgroup remains
     *    foo.bar.
     * 3) User rescans.
     * 4) The active file that gets sent as a result of the LIST
     *    command says that there are new articles in foo.bar.
     * 5) HOWEVER, even though the active file says that, the NNTP
     *    server has not yet realized that, because it doesn't update
     *    its idea of what's in the current newsgroup unless a GROUP
     *    command is issued, i.e., it ignores the LIST data it sends
     *    over the wire.
     * 6) User tries to reenter newsgroup.
     * 7) Since currentNewsgroup is set to foo.bar, xrn doesn't send
     *    GROUP command.  Therefore, NNTP server does not update its
     *    idea of low and high articles for the newsgroup.
     * 8) Therefore, the XHDR commands to get header information from
     *    the new articles fail, because the server doesn't know that
     *    those articles exist.
     *
     * We solve this problem by always requiring a GROUP command when
     * first accessing a group after a LIST command, by setting
     * currentNewsgroup to 0.
     */
    currentNewsgroup = 0;
    (void) strcpy(command, "LIST");
    put_server(command);
    get_data_from_server(message, sizeof(message));

    check_time_out(command, message, sizeof(message));
    
    if (*message != CHAR_OK) {
	(void) strcat(mybuf, "        Request was: ");
	(void) strcat(mybuf, command);
	(void) strcat(mybuf, "\n");
	(void) strcat(mybuf, "        Failing response was: ");
	(void) strcat(mybuf, message);
	ehErrorExitXRN(mybuf);
    }

    for (;;) {
	get_data_from_server(message, sizeof(message));
	
	/* the list is ended by a '.' at the beginning of a line */
	if (*message == '.') {
	    break;
	}

	/* server returns: group last first y/m/x/=otherGroup */

	for ( ptr = message; *ptr == ' ';++ptr);	/* skip leading spaces */
#if GROUP_NAME_SIZE <= 127
	"GROUP_NAME_SIZE is too small" /* this will produce a compilation */
	     			       /* error */
#endif
	if (sscanf(message, "%127s %ld %ld %s", group, &last, &first, type) != 4) {
	    mesgPane(XRN_SERIOUS, BOGUS_ACTIVE_ENTRY_MSG, message);
	    continue;
	}

	if (type[0] == 'x') {
	    /* bogus newsgroup, pay no attention to it */
	    continue;
	}

	if (type[0] == '=') {
	    /* This newsgroup doesn't exist, it's just an alias */
	    continue;
	}

#ifndef NO_BOGUS_GROUP_HACK
	/* determine if the group name is screwed up - check for jerks who
	 * create group names like: alt.music.enya.puke.puke.pukeSender: 
	 * - note that there is a ':' in the name of the group... */

	if (strpbrk(group, ":!, \n\t")) {
	    continue;
	}

#endif /* NO_BOGUS_GROUP_HACK */

	for (re_ptr = re_list; re_ptr && *re_ptr; re_ptr++) {
#ifdef SYSV_REGEX
	    if (regex(*re_ptr, group))
#else
	    if ((! re_comp(*re_ptr)) && re_exec(group))
#endif
	    {
#ifdef DEBUG
		fprintf(stderr, "Ignoring %s.\n", group);
#endif
		break;
	    }
	}
	if (re_ptr && *re_ptr) {
	    continue;
	}

	if (first == 0) {
	    first = 1;
	}

	if (!avl_lookup(NewsGroupTable, group, &ptr)) {

	    /* no entry, create a new group */
	    newsgroup = ALLOC(struct newsgroup);
	    newsgroup->name = XtNewString(group);
	    newsgroup->newsrc = NOT_IN_NEWSRC;
	    newsgroup->status = NG_NOENTRY;
	    newsgroup->first = first;
	    newsgroup->last = last;
	    newsgroup->nglist = 0;
#ifdef notdef
	    if (last >= first) {
		    (void) fprintf(stderr,"allocate %d bytes for %d articles in %s\n", (newsgroup->last - newsgroup->first + 1) * sizeof (struct article),(newsgroup->last - newsgroup->first + 1), group);
		newsgroup->articles = ARRAYALLOC(struct article, newsgroup->last - newsgroup->first + 1);
		for (art = newsgroup->first; art <= newsgroup->last; art++) {
		    long indx = INDEX(art);
	
		    newsgroup->articles[indx].subject = NIL(char);
		    newsgroup->articles[indx].author = NIL(char);
		    newsgroup->articles[indx].lines = NIL(char);
		    newsgroup->articles[indx].filename = NIL(char);
		    newsgroup->articles[indx].status = ART_CLEAR;
		}
	    } else {
		newsgroup->articles = NIL(struct article);
	    }
#else
		newsgroup->articles = NIL(struct article);
#endif
	    
	    if (avl_insert(NewsGroupTable, newsgroup->name,
			   (char *) newsgroup) < 0) {
		 ehErrorExitXRN("out of memory");
	    }

	    ActiveGroupsCount++;
	    
	} else {
	    
	    /*
	     * entry exists, use it; must be a rescanning call
	     *
	     * just update the first and last values and adjust the
	     * articles array
	     */
	    
	    newsgroup = (struct newsgroup *) ptr;

	    /*
	     * only allow last to increase or stay the same
	     * - don't allow bogus last values to trash a group
	     */
	    if (IS_SUBSCRIBED(newsgroup) && last >= newsgroup->last) {
		/* XXX really should save up the resync and use the GROUP info also */
		articleArrayResync(newsgroup, first, last, 1);
	    }
	}
	switch (type[0]) {
	case 'y':
#ifndef INN
	case '=':
#endif
	    newsgroup->status |= NG_POSTABLE;
	    newsgroup->status &= ~(NG_MODERATED|NG_UNPOSTABLE);
	    break;

	case 'm':
	    newsgroup->status |= NG_MODERATED;
	    newsgroup->status &= ~(NG_POSTABLE|NG_UNPOSTABLE);
	    break;

	case 'n':
#ifdef INN
	case '=':
#endif
	    newsgroup->status |= NG_UNPOSTABLE;
	    newsgroup->status &= ~(NG_POSTABLE|NG_MODERATED);
	    break;

	default:
	    /*
	    fprintf(stderr, "unexpected type (%s) for newsgroup %s\n",
		    type, newsgroup->name);
	    */
	    break;
	}
    }

    return;
}

/*
 * check the case where the first and last article numbers are equal
 * - unfortunately, this means two different things:
 *   1) there are no articles in the group
 *   2) there is one article in the group
 *
 * - so, to get rid of the ambiguity, we make a GROUP call
 *   and look at the 'number' of articles field to determine
 *   whether there are 0 or 1 articles
 */
void badActiveFileCheck()
{
    avl_generator *gen;
    char *key, *value;
    int number;

    /* check out first == last groups */
    gen = avl_init_gen(NewsGroupTable, AVL_FORWARD);
    if (! gen) {
	 ehErrorExitXRN("out of memory");
    }

    while (avl_gen(gen, &key, &value)) {
	struct newsgroup *newsgroup = (struct newsgroup *) value;

	if (IS_SUBSCRIBED(newsgroup) &&
	    (newsgroup->first == newsgroup->last) &&
	    (newsgroup->first != 0)) {

	    if (! (getgroup(newsgroup, 0, 0, &number) || number)) {
		articleArrayResync(newsgroup, newsgroup->first, newsgroup->last, number);
	    }
	}
    }
    avl_free_gen(gen);

    return;
}

/*
 * initiate a connection to the news server
 *
 * nntpserver is the name of an alternate server (use the default if NULL)
 *
 * the server eventually used is remembered, so if this function is called
 * again (for restarting after a timeout), it will use it.
 *
 *   returns: void
 *
 */
void start_server(nntpserver)
    char *nntpserver;
{
    static char *server = NIL(char);   /* for restarting */
    int response, connected;

    if (! server)
	server = nntpserver;

    if (! server)
	server = getenv("NNTPSERVER");

#ifdef INN
    if (! server)
	/* INN ignores the argument */
	server = getserverbyfile("");
#else
# ifdef SERVER_FILE
    if (! server)
	server = getserverbyfile(SERVER_FILE);
# endif
#endif

    if (! server)
	ehErrorExitXRN(NO_SERVER_MSG);

    do {
	if ((response = server_init(server)) < 0) {
	    connected = 0;
	    mesgPane(XRN_SERIOUS, FAILED_RECONNECT_MSG, "server_init");
 	    xthHandleAllPendingEvents();
	    sleep(60);
	    continue;
	}
	if (handle_server_response(response, server) < 0) {
	    connected = 0;
	    stop_server();
	    mesgPane(XRN_SERIOUS, FAILED_RECONNECT_MSG, "handle_response");
 	    xthHandleAllPendingEvents();
	    sleep(60);
	    continue;
	}
	connected = 1;
    } while (!connected);
    
    return;
}


/*
 * close an outstanding connection to the NNTP server
 */
void stop_server()
{
    currentNewsgroup = 0;
    close_server();
}


/*
 * Calculate the number of digits in an integer.  Sure, I could use a
 * logarithm function, but that would require relying on a sane math
 * library on all systems.  The technique used in this function is
 * gross, but what the heck, it works.
 */
static int digits(num)
    long int num;
{
    char int_buf[20]; /* An article number longer than twenty digits?
			 I'll be dead by then! */

    (void) sprintf(int_buf, "%ld", num);
    return(strlen(int_buf));
}


/*
 * get a list of subject lines for the current group in the range
 *  'first' to 'last'
 *
 *   returns: void
 *
 * Note that XHDR is not part of the rfc977 standard, but is implemented
 * by the Berkeley NNTP server
 *
 */
void getsubjectlist(newsgroup, artfirst, artlast, unreadonly)
    struct newsgroup *newsgroup;
    art_num artfirst;
    art_num artlast;
    Boolean unreadonly;
{
    char command[MESSAGE_SIZE], message[MESSAGE_SIZE], buffer[MESSAGE_SIZE];
    char *subjectline;
    long number;
    long first, last;
    int num_column;

    if (SETNEWSGROUP(newsgroup)) {
	return;
    }

    first = artfirst;
    num_column = digits(artlast);
    while (first <= artlast) {
	if (newsgroup->articles[INDEX(first)].subject != NIL(char) ||
	    (unreadonly && IS_READ(newsgroup->articles[INDEX(first)]))) {
	     first++;
	     continue;
	}

	for (last = first + 1; last <= artlast; last++) {
	    if (newsgroup->articles[INDEX(last)].subject != NIL(char) ||
		(unreadonly && IS_READ(newsgroup->articles[INDEX(last)]))) {
		break;
	    }
	}
	last--;

	(void) sprintf(command, "XHDR subject %ld-%ld", first, last);
	put_server(command);
	get_data_from_server(message, sizeof(message));

	check_time_out(command, message, sizeof(message));

	/* check for errors */
	if (*message != CHAR_OK) {
	    mesgPane(XRN_SERIOUS, XHDR_ERROR_MSG);
	    return;
	}

	for(;;) {

	    get_data_from_server(message, sizeof(message));
	    
	    if (*message == '.') {
		break;
	    }

	    /*
	     * message is of the form:
	     *
	     *    Number SubjectLine
	     *
	     *    203 Re: Gnumacs Bindings
	     *
	     * must get the number since not all subjects will be returned
	     */

	    number = atol(message);
	    subjectline = index(message, ' ');
	    (void) sprintf(buffer, "  %*ld %s", num_column,
			   number, ++subjectline);

	    newsgroup->articles[INDEX(number)].subject = XtNewString(buffer);
	}
	first = last + 1;
    }
    return;
}

/*
 * get a list of author lines for the current group in the range
 *  'first' to 'last'
 *
 *   returns: void
 *
 * Note that XHDR is not part of the rfc977 standard, but is implemented
 * by the Berkeley NNTP server
 *
 */
void getauthorlist(newsgroup, artfirst, artlast, unreadonly)
    struct newsgroup *newsgroup;
    art_num artfirst;
    art_num artlast;
    Boolean unreadonly;
{
    char command[MESSAGE_SIZE], message[MESSAGE_SIZE];
    char *author, *end, *brackbeg, *brackend;
    long number;
    long first, last;

    if (SETNEWSGROUP(newsgroup)) {
	return;
    }

    first = artfirst;
    while (first <= artlast) {
	if (newsgroup->articles[INDEX(first)].author != NIL(char) ||
	    (unreadonly && IS_READ(newsgroup->articles[INDEX(first)]))) {
	    first++;
	    continue;
	}

	for (last = first + 1; last <= artlast; last++) {
	    if (newsgroup->articles[INDEX(last)].author != NIL(char) ||
		(unreadonly && IS_READ(newsgroup->articles[INDEX(last)]))) {
		break;
	    }
	}
	last--;

	(void) sprintf(command, "XHDR from %ld-%ld", first, last);
	put_server(command);
	get_data_from_server(message, sizeof(message));

	check_time_out(command, message, sizeof(message));

	/* check for errors */
	if (*message != CHAR_OK) {
	    mesgPane(XRN_SERIOUS, XHDR_ERROR_MSG);
	    return;
	}
	
	for(;;) {

	    get_data_from_server(message, sizeof(message));
	    
	    if (*message == '.') {
		break;
	    }

	    /*
	     * message is of the form:
	     *
	     *    Number Author
	     *
	     *    201 ricks@shambhala (Rick L. Spickelmier)
	     *    202 Jens Thommasen <jens@ifi.uio.no>
	     *    203 <oea@ifi.uio.no>
	     *    302 "Rein Tollevik" <rein@ifi.uio.no>
	     *
	     * must get the number since not all authors will be returned
	     */

	    number = atol(message);
	    if (app_resources.authorFullName) {
		/* Can be made fancyer at the expence of extra cpu time */
		author = index(message, ' ');
		assert(author != NIL(char));
		author++;

		/* First check for case 1, user@domain ("name") -> name */

		brackbeg = index(message, '(');
		brackend = index(message, '\0') - sizeof(char);
		/* brackend now points at the last ')' if this is case 1 */
		if (brackbeg != NIL(char) && (brackend > brackbeg) &&
		    (*brackend == ')')) {
		    author = brackbeg + sizeof(char);

		    /* Remove surrounding quotes ? */
		    if ((*author == '"') && (*(brackend - sizeof(char)) == '"')) {
		      author++;
		      brackend--;
		    }

		    /* Rather strip trailing spaces here */

		    *brackend = '\0';
		} else {
		    /* Check for case 2, "name" <user@domain> -> name */
		    brackbeg = index(message, '<');
		    if (brackbeg != NIL(char) && (index(brackbeg, '>') != NIL(char))
			&& (brackbeg > message)) {
			while (*--brackbeg == ' ')
			  ;

			/* Remove surrounding quotes ? */
			if ((*brackbeg == '"') && (*author ==  '"')) {
			    *brackbeg = '\0';
			    author++;

			    /* Rather strip trailing spaces here */

			} else {
			    *++brackbeg = '\0';
			}
		    } else {

			/* 
			 * Check for case 3, <user@domain> -> usr@domain
			 *
			 * Don't need to do this again:
			 * brackbeg = index(message, '<');
			 */

			brackend = index(message, '>');
			if ((author == brackbeg) && (brackend != NIL(char))) {
			    author++;
			    *brackend = '\0';
			} else {
			    if ((end = index(author, ' ')) != NIL(char)) {
				*end = '\0';
			    }
			}
		    }
		}
	    } else {
		if ((author = index(message, '<')) == NIL(char)) {
		    /* first form */
		    author = index(message, ' ');
		    assert(author != NIL(char));
		    author++;
		    if ((end = index(author, ' ')) != NIL(char)) {
			*end = '\0';
		    }
		} else {
		    /* second form */
		    author++;
		    if ((end = index(author, '>')) != NIL(char)) {
			*end = '\0';
		    }
		}
	    }
	    /*
	     * do a final trimming - just in case the authors name ends
	     * in spaces or tabs - it does happen
	     */
	    end = author + utStrlen(author) - 1;
	    while ((end > author) && ((*end == ' ') || (*end == '\t'))) {
		*end = '\0';
		end--;
	    }
	    newsgroup->articles[INDEX(number)].author = XtNewString(author);
	}
	first = last + 1;
    }
    return;
}

/*
 * get a list of number of lines per message for the current group in the
 *  range 'first' to 'last'
 *
 *   returns: void
 *
 * Note that XHDR is not part of the rfc977 standard, but is implemented
 * by the Berkeley NNTP server
 *
 */
void getlineslist(newsgroup, artfirst, artlast, unreadonly)
    struct newsgroup *newsgroup;
    art_num artfirst;
    art_num artlast;
    Boolean unreadonly;
{
    char command[MESSAGE_SIZE], message[MESSAGE_SIZE];
    char *numoflines, *end;
    long number;
    int lcv;
    long first, last;

    if (SETNEWSGROUP(newsgroup)) {
	return;
    }

    if (!app_resources.displayLineCount) {
	return;
    }

    first = artfirst;
    while (first <= artlast) {
	if (newsgroup->articles[INDEX(first)].lines != NIL(char) ||
	    (unreadonly && IS_READ(newsgroup->articles[INDEX(first)]))) {
	    first++;
	    continue;
	}

	for (last = first + 1; last <= artlast; last++) {
	    if (newsgroup->articles[INDEX(last)].lines != NIL(char) ||
		(unreadonly && IS_READ(newsgroup->articles[INDEX(last)]))) {
		break;
	    }
	}
	last--;

	(void) sprintf(command, "XHDR lines %ld-%ld", first, last);
	put_server(command);
	get_data_from_server(message, sizeof(message));

	check_time_out(command, message, sizeof(message));

	/* check for errors */
	if (*message != CHAR_OK) {
	    mesgPane(XRN_SERIOUS, XHDR_ERROR_MSG);
	    return;
	}

	for(;;) {

	    get_data_from_server(message, sizeof(message));

	    if (*message == '.') {
		break;
	    }

	    /*
	     * message is of the form:
	     *
	     *    Number NumberOfLines
	     *
	     *    203 ##
	     *
	     * must get the number since not all subjects will be returned
	     */

	    number = atol(message);
	    numoflines = index(message, ' ');
	    assert(numoflines != NIL(char));
	    numoflines++;

	    /* Ignore extra whitespace at the beginning of the field */
	    for ( ; (*numoflines == ' ') || (*numoflines == '\t'); numoflines++)
		/* empty */;
	    
	    if ((end = index(numoflines, ' ')) != NIL(char)) {
		*end = '\0';
	    }

	    if (numoflines[0] != '(') {
		numoflines[utStrlen(numoflines)+1] = '\0';
		numoflines[utStrlen(numoflines)] = ']';
		for (lcv = utStrlen(numoflines); lcv >= 0; lcv--) {
		    numoflines[lcv+1] = numoflines[lcv];
		}
		numoflines[0] = '[';
	    } else {
		numoflines[0] = '[';
		numoflines[utStrlen(numoflines)-1] = ']';
	    }
	    if (strcmp(numoflines, "[none]") == 0) {
		(void) strcpy(numoflines, "[?]");
	    }
	    newsgroup->articles[INDEX(number)].lines = XtNewString(numoflines);
	}
	first = last + 1;
    }
    return;
}

#ifndef INEWS
static void sendLine _ARGUMENTS((char *));

static void sendLine(str)
    char *str;
{
    if (*str == '.') {
	char *str2 = XtMalloc(utStrlen(str) + 2); /* one for the extra period,
						   * and one for the null at
						   * the end */
	str2[0] = '.';
	strcpy(str2 + 1, str);
	put_server(str2);
	XtFree(str2);
    }
    else {
	put_server(str);
    }
    return;
}
#endif

static char * getLine _ARGUMENTS((char **));

static char * getLine(ptr)
    char **ptr;
{
    static char line[512];
    char *end = index(*ptr, '\n');

    if (end) {
	(void) strncpy(line, *ptr, end - *ptr);
	line[end - *ptr] = '\0';
	*ptr = end + 1;
    } else {
	(void) strcpy(line, *ptr);
	*ptr = 0;
    }
    return line;
}

/*
 * Takes a block of text, wraps the text based on lineLength and
 * breakLength resources, and returns a NULL-terminated allocated
 * array of allocated strings representing the wrapped lines.  The
 * procedure which calls wrapText should use the wrapped Text and then
 * free each string and free the array.
 */
static char ** wrapText _ARGUMENTS((char *));

static char ** wrapText(ptr)
    char *ptr;
{
     int c = 0;		/* current line length */
     char **lines, *this_line;
     unsigned int num_lines = 0;
     int breakAt = app_resources.breakLength;
     int maxLength;

     maxLength = app_resources.lineLength;
     if (app_resources.breakLength > maxLength) {
       maxLength = app_resources.breakLength;
     }

     lines = (char **) XtMalloc((Cardinal) 0);

     if (app_resources.breakLength && app_resources.lineLength) {
	 /*
	  * Do text wrapping.
	  */
	 this_line = XtMalloc((Cardinal) (maxLength + 1));

	 while (*ptr != '\0') {
	     if (c >= breakAt) {
		 /*
		  * Everything after the first line in a paragraph
		  * should be wrapped at lineLength, not breakLength.
		  * This prevents the last line of a paragraph from
		  * ending up a little bit longer than all the other
		  * lines (and extending into the margin), but not quite
		  * breakLength characters lines long.
		  */
		 breakAt = app_resources.lineLength;
		 /* backoff to app_resources.lineLength */
		 ptr -= c - app_resources.lineLength;
		 c = app_resources.lineLength;
		 for (; c > 0 && *ptr != ' ' && *ptr != '\t'; ptr--) {
#ifdef notdef
		     if (*ptr == '\t') {
		     }
#endif
		     c--;
		 }

		 if (c == 0) {
		     /* pathological, cut to app_resources.lineLength */
		     c = app_resources.lineLength;
		     ptr += app_resources.lineLength - 1;
		 }

		 /* output */
		 this_line[c] = '\0';
		 lines = (char **) XtRealloc((char *) lines, (Cardinal)
					     (sizeof(char *) * ++num_lines));
		 lines[num_lines-1] = this_line;
		 this_line = XtMalloc((Cardinal) (maxLength + 1));
		 c = 0;
		 if (strncmp(lines[num_lines-1],
			    app_resources.includePrefix,
			    utStrlen(app_resources.includePrefix)) == 0) {
		     strcpy(this_line, app_resources.includePrefix);
		     c += utStrlen(app_resources.includePrefix);
		 }

		 /*
		  * Delete any extra spaces, tabs or carriage returns at 
		  * the beginning of the next line.  This is necessary
		  * because we may break a line in the middle of a
		  * multi-space word break (e.g. the end of a sentence),
		  * or right before the paragraph-ending carriage
		  * return, which we've already printed as part of the
		  * line above.
		  */
		 while ((*ptr == ' ') || (*ptr == '\t') || (*ptr == '\n')) {
		     ptr++;
		     if (*(ptr-1) == '\n')
			  /* We only one to get rid of one carriage return */
			  break;
		 }
	       
		 continue;
	     }

	     if (*ptr == '\n') {
		 this_line[c] = '\0';
		 lines = (char **) XtRealloc((char *) lines, (Cardinal)
					     (sizeof(char *) * ++num_lines));
		 lines[num_lines-1] = this_line;
		 this_line = XtMalloc((Cardinal) (maxLength + 1));
		 if (c == 0)
		     breakAt = app_resources.breakLength;
		 c = 0, ptr++;
		 continue;
	     }

#ifdef notdef
	     if (*ptr == '\t') {
		 c += c % 8;
		 continue;
	     }
#endif
	  
	     this_line[c++] = *ptr++;
	 }

	 if (c != 0) {
	     this_line[c] = '\0';
	     lines = (char **) XtRealloc((char *) lines, (Cardinal)
					 (sizeof(char *) * ++num_lines));
	     lines[num_lines-1] = this_line;
	 }
     }
     else {
	 /*
	  * Don't do text wrapping, just break the text at linefeeds.
	  */
	 while (*ptr) {
	     c = 0;
	     for (; *ptr && (*ptr != '\n'); ptr++, c++) ;
	     if (c || *ptr) {
		 this_line = XtMalloc((Cardinal) (c + 1));
		 lines = (char **) XtRealloc((char *) lines,
					     (Cardinal) (sizeof(char *) *
							 ++num_lines));
		 strncpy(this_line, &ptr[-c], c);
		 this_line[c] = '\0';
		 lines[num_lines-1] = this_line;
		 if (*ptr)
		     ptr++;
	     }
	 }
     }
     
     lines = (char **) XtRealloc((char *) lines,
				 (Cardinal) (sizeof(char *) * ++num_lines));
     lines[num_lines-1] = NULL;

     return(lines);
}

static int mailArticle _ARGUMENTS((char *));

static int mailArticle(article)
    char *article;
{
     FILE *fp;
     char **lines_ptr, **lines;
     char *ptr;

     if ((fp = xrn_popen(app_resources.mailer, "w")) == NULL)
	  return POST_FAILED;

     /* First, send everything up to the first blank line without any */
     /* wrapping. 						      */
     while (1) {
	  ptr = index(article, '\n');
	  if ((ptr == article) || (ptr == NULL))
	       /* line has nothing but newline or end of article */
	       break;
	  (void) fwrite(article, sizeof(char), (unsigned) (ptr - article + 1), fp);
	  article = ptr + 1;
     }
     
     lines_ptr = lines = wrapText(article);
     while (*lines) {
	  (void) fwrite(*lines, sizeof(char), utStrlen(*lines), fp);
	  (void) fwrite("\n", sizeof(char), 1, fp); /* wrapText deletes newlines */
	  FREE(*lines);
	  lines++;
     }
     FREE(lines_ptr);

     return xrn_pclose(fp) ? POST_FAILED : POST_OKAY;
}




int postArticle(article, mode, ErrMsg)
    char *article;
    int mode;   /* XRN_NEWS or XRN_MAIL */
    char **ErrMsg;
/*
 * post an article
 *
 *   returns 1 for success, 0 for failure
 */
{
    char command[MESSAGE_SIZE], message[MESSAGE_SIZE];
    char *ptr, *saveptr;
    char **lines, **lines_ptr;

#ifdef INEWS
    char *tempfile;
    int exitstatus;
    char buffer[1024];
    FILE *inews;
#endif

    *ErrMsg = 0;

    if (mode == XRN_MAIL) {
	return mailArticle(article);
    }

#ifdef INEWS
    tempfile = XtNewString(utTempnam(app_resources.tmpDir, "xrn"));
    (void) sprintf(buffer, "%s -h > %s 2>&1",INEWS, tempfile);
    if ((inews = xrn_popen(buffer, "w")) == NULL) {
	mesgPane(XRN_SERIOUS, CANT_EXECUTE_CMD_POPEN_MSG, buffer);
	(void) unlink(tempfile);
	FREE(tempfile);
	return POST_FAILED;
    }
#else

    (void) strcpy(command, "POST");
    put_server(command);
    get_data_from_server(message, sizeof(message));

    check_time_out(command, message, sizeof(message));

    if (*message != CHAR_CONT) {
	mesgPane(XRN_SERIOUS, NNTP_ERROR_MSG, message);
	if (atoi(message) == ERR_NOPOST) {
	    return(POST_NOTALLOWED);
	} else {
	    return(POST_FAILED);
	}
    }
#endif

    ptr = article;

    while (1) {
	char *line;

	saveptr = ptr;

	line = getLine(&ptr);
	if (index(line, ':') || (*line == ' ') || (*line == '\t')) {
#ifdef INEWS
	    fputs(line, inews);
	    fputc('\n', inews);
#else
	    sendLine(line);
#endif
	    continue;
	}
	break;
    }

#ifdef INEWS
    fputs("\n\n", inews);
#else
    sendLine("");		/* send a blank line */
#endif

    if (*saveptr != '\n') {
	 /* if the skipped line was not blank, point back to it */
	 ptr = saveptr;
    }

    lines_ptr = lines = wrapText(ptr);
    while (*lines) {
#ifdef INEWS
         fputs(*lines, inews);
	 fputc('\n', inews);
#else
	 sendLine(*lines);
#endif
	 XtFree(*lines);
	 lines++;
    }
    FREE(lines_ptr);
    
#ifdef INEWS
    if (exitstatus = xrn_pclose(inews)) {
	FILE *filefp;
	char *p;
	struct stat buf;
	char temp[1024];

#ifndef INN	
	(void) sprintf(temp, "\n\ninews exit value: %d\n", exitstatus);
#else
	temp[0] = '\0';
#endif /* INN */	
	if ((filefp = fopen(tempfile, "r")) != NULL) {
	    if (fstat(fileno(filefp), &buf) != -1) {
		p = XtMalloc(buf.st_size + utStrlen(temp) + 10);
		(void) fread(p, sizeof(char), buf.st_size, filefp);
		p[buf.st_size] = '\0';
		(void) strcat(p, temp);
		(void) fclose(filefp);
		*ErrMsg = p;
	    }
	}
	(void) unlink(tempfile);
	FREE(tempfile);
	return(POST_FAILED);
    }
#else
    put_server(".");

    get_data_from_server(message, sizeof(message));

    if (*message != CHAR_OK) {
	*ErrMsg = XtNewString(message);
	return(POST_FAILED);
    }
#endif

    return(POST_OKAY);
}


#ifdef DONT_USE_XHDR_FOR_A_SINGLE_ITEM

/*
 * get header information about 'article'
 *
 *   the results are stored in 'string'
 */
void xhdr(newsgroup, article, field, string)
    struct newsgroup *newsgroup;
    art_num article;
    char *field;
    char **string;
{
    char buffer[BUFFER_SIZE], message[MESSAGE_SIZE], *ptr, *cmp, *found = 0;

    if (SETNEWSGROUP(newsgroup)) {
	*string = 0;
	return;
    }

    /*
     * In some implementations of NNTP, the XHDR request on a
     * single article can be *very* slow, so we do a HEAD request
     * instead and just search for the appropriate field.
     */
    (void) sprintf(buffer, "HEAD %ld", article);
    put_server(buffer);
    get_data_from_server(message, sizeof(message));

    check_time_out(buffer, message, sizeof(message));

    if (*message != CHAR_OK) {
	/* can't get header */
	*string = NIL(char);
	return;
    }

    for (;;) {
	get_data_from_server(message, sizeof(message));

	/* the header information is ended by a '.' on a line by itself */
	if (message[0] == '.')
	    break;

	if (!found) {
	    for (ptr = message, cmp = field; *ptr; ptr++, cmp++) {
		/* used to be 'mklower' */
		if (tolower(*cmp) != tolower(*ptr))
		    break;
	    }
	    if (*cmp == 0 && *ptr == ':') {
		while (*++ptr == ' ')
		    ;
		found = XtNewString(ptr);
	    }
	}
    }

    if (found)
	*string = found;
    else
	*string = NIL(char);

    return;
}

#else

/*
 * get header information about 'article'
 *
 *   the results are stored in 'string'
 */
void xhdr(newsgroup, article, field, string)
    struct newsgroup *newsgroup;
    art_num article;
    char *field;
    char **string;
{
    char buffer[BUFFER_SIZE], message[MESSAGE_SIZE], *ptr;

    if (SETNEWSGROUP(newsgroup)) {
	*string = 0;
	return;
    }
    (void) sprintf(buffer, "XHDR %s %ld", field, article);
    put_server(buffer);
    get_data_from_server(message, sizeof(message));
    
    check_time_out(buffer, message, sizeof(message));
    
    /* check for errors */
    if (*message != CHAR_OK) {
	fprintf(stderr, "NNTP error: %s\n", message);
	*string = NIL(char);
	mesgPane(XRN_SERIOUS, XHDR_ERROR_MSG);
	return;
    }
    
    get_data_from_server(message, sizeof(message));

    /* no information */
    if (*message == '.') {
	*string = NIL(char);
	return;
    }

    ptr = index(message, ' ');

    /* malformed entry */
    if (ptr == NIL(char)) {
	mesgPane(XRN_SERIOUS, MALFORMED_XHDR_RESPONSE_MSG, buffer, message);
	get_data_from_server(message, sizeof(message));
	return;
    }

    ptr++;

    /* no information */
    if (STREQ(ptr, "(none)")) {
	*string = NIL(char);
	/* ending '.' */
	do {
	    get_data_from_server(message, sizeof(message));
	} while (*message != '.');
	return;
    }

    *string = XtNewString(ptr);

    /* ending '.' */
    do {
	get_data_from_server(message, sizeof(message));
    } while (*message != '.');

    return;
}
#endif

struct article * getarticles(newsgroup)
    struct newsgroup *newsgroup;
{
    register art_num first = newsgroup->first, last = newsgroup->last, art;

    if (last >= first && last != 0) {
	register struct article	*ap;
	newsgroup->articles = ARRAYALLOC(struct article, last - first + 1);

	ap = &newsgroup->articles[INDEX(first)];
    
	for (art = first; art <= last; art++) {
	    ap->subject = NIL(char);
	    ap->author = NIL(char);
	    ap->lines = NIL(char);
	    ap->filename = NIL(char);
	    ap->status = ART_CLEAR;
	    ap++;
	}
    }
#ifdef STUPIDMMU
    cornered(newsgroup);
#endif
    return(newsgroup->articles);
}


#ifndef POPEN_USES_INEXPENSIVE_FORK

static int popen_pid = 0;

FILE *xrn_popen(command, type)
    CONST char *command;
    CONST char *type;
{
    int pipes[2];
    int itype = (strcmp(type, "w") == 0 ? 1 : 0);

    if (pipe(pipes) == -1)
	return NULL;

    switch (popen_pid = vfork()) {
    case -1:
	(void)close(pipes[0]);
	(void)close(pipes[1]);
	return NULL;

    case 0:
	if (itype) {
	    dup2(pipes[0], fileno(stdin));
	    close(pipes[1]);
	} else {
	    dup2(pipes[1], fileno(stdout));
	    close(pipes[0]);
	}
	execl("/bin/sh", "/bin/sh", "-c", command, 0);
	fprintf(stderr, "XRN Error: failed the execlp\n");
	_exit(-1);
	/* NOTREACHED */

    default:
	    if (itype) {
		close(pipes[0]);
		return fdopen(pipes[1], "w");
	    } else {
		close(pipes[1]);
		return fdopen(pipes[0], "r");
	    }
    }
}

int xrn_pclose(str)
    FILE *str;
{
    int pd = 0;
    int	status;
    int	err;

    err = fclose(str);

    do {
	if ((pd = wait(&status)) == -1)
	{
		err = EOF;
		break;
	}
    } while (pd !=  popen_pid);

    if (err == EOF)
	return  -1;
	
    if (status)
	status >>= 8;	/* exit status in high byte */

    return status;
}

#endif
