#if !defined(lint) && !defined(SABER)
static char XRNrcsid[] = "$Header: /d/src/cvsroot/xrn/compose.c,v 1.20 1994-10-20 15:10:13 jik Exp $";
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
 * compose.c: routines for composing messages and articles
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <signal.h>
#include "error_hnds.h"
#ifdef RESOLVER
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif
#ifndef VMS
#include <pwd.h>
#endif

#if defined(aiws) || defined(DGUX)
struct passwd *getpwuid();
struct passwd *getpwnam();
#endif

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>

#ifndef MOTIF
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Dialog.h>
#else
#include <Xm/PanedW.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/MessageB.h>
#endif

#if defined(__hpux)
#include <unistd.h>
#endif

#if defined(sun) && defined(sparc) && !defined(SOLARIS)
#include <vfork.h>
/*
 * Unfortunately, it's necessary to declare fclose on the Sun because
 * stdio.h in the standard Sun C library doesn't, and we need to
 * assign fclose to a function pointer variable below.
 */
extern int fclose _ARGUMENTS((FILE *));
#endif

#include "avl.h"
#include "xthelper.h"
#include "resources.h"
#include "dialogs.h"
#include "news.h"
#include "internals.h"
#include "server.h"
#include "mesg.h"
#include "xrn.h"
#include "xmisc.h"
#include "buttons.h"
#include "butdefs.h"
#include "mesg_strings.h"

/*
  XawFmt8Bit only available starting in X11R6.
  */
#ifndef XawFmt8Bit
#define XawFmt8Bit FMT8BIT
#endif

#ifndef MOTIF
#define XAWSEARCH(widget, string, dir, pos) \
    {\
	XawTextBlock block; \
	block.firstPos = 0; \
	block.ptr = string; \
	block.length = utStrlen(block.ptr); \
	block.format = XawFmt8Bit; \
	pos = XawTextSearch(widget, dir, &block); \
    }

#define XAWTEXTINSERT(widget, loc, string) \
    { \
	XawTextBlock block; \
	block.firstPos = 0; \
	block.ptr = string; \
	block.length = utStrlen(block.ptr); \
	block.format = XawFmt8Bit; \
	(void) XawTextReplace(widget, loc, loc, &block); \
    }
#else

/**********************************************************************
Yuck, yuck, yuck.  Provide Xaw-like routines to manipulate the Motif
text widget instead of the Xaw one.  We can't use the stuff from
MotifXawHack.h, because it manipulates a Motif list widget.
**********************************************************************/

#define MOTIF_FORWARD 1
#define MOTIF_BACKWARD 2
#define XawsdRight MOTIF_FORWARD
#define XawsdLeft MOTIF_BACKWARD
#define XawTextSearchError (-1)
#define XawTextPosition XmTextPosition

static int MotifSearch _ARGUMENTS((Widget, char *, int));

static int MotifSearch(w, str, dir)
    Widget w;
    char *str;
    int dir;
{
  char *data, *area, *p, *q;
  int result;

  data = XmTextGetString(w);
  if (dir == MOTIF_FORWARD) {
    area = data;
    area += XmTextGetCursorPosition(w);
  } else {
    area = data;
    data[XmTextGetCursorPosition(w)] = '\0';
  }
  p = strstr(area, str);
  if (p && (dir == MOTIF_BACKWARD)) {
    q = p+1;
    while (q) {
      q = strstr(q, str);
      if (q) {
	p = q;
	q++;
      }
    }
  }
  if (p) {
    result = p-data;
  } else {
    result = XawTextSearchError;
  }
  XtFree(data);
  return result;
}

static void MotifInsert _ARGUMENTS((Widget, XmTextPosition, char *));

static void MotifInsert(w, loc, str)
    Widget w;
    XmTextPosition loc;
    char *str;
{
  XmTextInsert(w, (XmTextPosition) loc, str);
}

#define XAWSEARCH(widget, string, dir, pos) \
	{ pos = MotifSearch(widget, string, dir); }

#define XAWTEXTINSERT(widget, loc, string) \
	{ MotifInsert(widget, loc, string); }

#endif

/* entire pane */
static Widget ComposeTopLevel = (Widget) 0;
/* text window */
static Widget ComposeText = (Widget) 0;

/*
  This stuff is used by the external editorCommand code.  See
  Call_Editor and processMessage below.  See also some hooks in
  composePane, compSendFunction and destroyCompositionTopLevel (and
  possibly in a couple others I've forgotten).
  */

static char *EditorFileName = 0;
static struct stat originalBuf;

typedef enum {
    NoEdit, InProgress, Completed
} EditStatus;

EditStatus editing_status = NoEdit;
extern int inchannel;

static int Call_Editor _ARGUMENTS((char * header));

/*
  End stuff used for editorCommand support.
  */

#define BUFFER_SIZE 1024

#define POST     	0
#define FOLLOWUP 	1
#define REPLY    	2
#define FORWARD  	3
#define GRIPE    	4
#define FOLLOWUP_REPLY	5
static int PostingMode = POST;
static char *PostingModeStrings[] =
    { "post", "followup", "reply", "forward", "gripe", "followup" };

struct header {
    art_num article;
    char *artFile;
    char *newsgroups;
    char *subject;
    char *messageId;
    char *followupTo;
    char *references;
    char *from;
    char *replyTo;
    char *distribution;
    char *keywords;

    char *user;
    char *real_user;
    char *fullname;
    char *host;
    char *real_host;
    char *path;		/* note: this is not used with InterNetNews */
    char *organization;

    char  *date; /* added to header structure....... */
};

/*
 * storage for the header information needed for building, checking,
 * and repairing the header.  Once this is created, the user can go
 * to another article or another group and still use the composition
 * window
 */

static struct header Header = {0, NIL(char), NIL(char), NIL(char), NIL(char), NIL(char),
                               NIL(char), NIL(char), NIL(char), NIL(char), NIL(char),
 NIL(char),NIL(char)};


#define HOST_NAME_SIZE 1024

/*
 * get the header and other important information for an article
 *
 * if called with article equal to zero, will only set up the non-article
 * specific entries
 *
 */
static void getHeader _ARGUMENTS((art_num));

static void getHeader(article)
    art_num article;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct passwd *pwd;
    char host[HOST_NAME_SIZE], buffer[BUFFER_SIZE], *ptr;
    char real_host[HOST_NAME_SIZE];
#ifdef INN
    char *inn_domain = (char *) GetFileConfigValue ("domain");
    char *inn_org;
#else
    char path[HOST_NAME_SIZE];
#endif
#if defined(RESOLVER) && !defined(RETURN_HOST)
    struct hostent *hent;
#endif
    
    if (article > 0) {
	Header.article = article;
	xhdr(newsgroup, article, "newsgroups", &Header.newsgroups);
	xhdr(newsgroup, article, "subject", &Header.subject);
	xhdr(newsgroup, article, "message-id", &Header.messageId);
	xhdr(newsgroup, article, "followup-to", &Header.followupTo);
	xhdr(newsgroup, article, "references", &Header.references);
	xhdr(newsgroup, article, "from", &Header.from);
	xhdr(newsgroup, article, "reply-to", &Header.replyTo);
	xhdr(newsgroup, article, "distribution", &Header.distribution);
	xhdr(newsgroup, article, "keywords", &Header.keywords);
    } else {
	/* information for 'post' */
	if (newsgroup) {
	    Header.newsgroups = XtNewString(newsgroup->name);
	} else {
	    Header.newsgroups = XtNewString("");
	}
    }

    /*
     * since I'm lazy down below, I'm going to replace NIL pointers with ""
     */
    if (Header.newsgroups == NIL(char)) {
	Header.newsgroups = XtNewString("");
    }
    if (Header.subject == NIL(char)) {
	Header.subject = XtNewString("");
    }
    if (Header.messageId == NIL(char)) {
	Header.messageId = XtNewString("");
    }
    if (Header.followupTo == NIL(char)) {
	Header.followupTo = XtNewString("");
    }
    if (Header.references == NIL(char)) {
	Header.references = XtNewString("");
    }
    if (Header.from == NIL(char)) {
	Header.from = XtNewString("");
    }
    if (Header.replyTo == NIL(char)) {
	Header.replyTo = XtNewString("");
    }
    if (Header.distribution == NIL(char)) {
	Header.distribution = XtNewString("");
    }
    if (Header.keywords == NIL(char)) {
	Header.keywords = XtNewString("");
    }

    real_host[0] = 0;
    host[0] = 0;
#ifdef	UUCPNAME
    {
        FILE * uup;
    
        if ((uup = fopen(UUCPNAME, "r")) != NULL) {
    		char   *p;
    		char    xbuf[BUFSIZ];
    
    		while (fgets(xbuf, sizeof(xbuf), uup) != NULL) {
    		    if (*xbuf <= ' ' || *xbuf == '#') {
    			continue;
		    }
    		    break;
    		}
    		if (p = index(xbuf, '\n')) {
    		    *p = 0;
		}
    		(void) strncpy(real_host, xbuf, sizeof(real_host) - 1);
		(void) strcpy(host, real_host);
    		(void) fclose(uup);
        }
    }
#endif
    if (real_host[0] == 0) {
#ifdef INN
	/* always let values in inn.cofnf take precedence */
	char *inn_fromhost = (char *) GetConfigValue("fromhost");
	if (inn_fromhost != NIL(char)) {
	    (void) strcpy(real_host, inn_fromhost);
	} else if (inn_domain != NIL(char)) {
	    (void) strcpy(real_host, inn_domain);
	} else {
#endif /* INN */
	    if ((ptr = getinfofromfile(HIDDEN_FILE)) != NULL) {
		(void) strncpy(real_host, ptr, sizeof(real_host) - 1);
	    } else {
#ifdef RETURN_HOST
		(void) strcpy(real_host, RETURN_HOST);
#else /* Not RETURN_HOST */
#ifndef VMS
		(void) gethostname(real_host, sizeof(real_host));
#ifdef RESOLVER
		hent = gethostbyname(real_host);
		if (hent != NULL) {
		    (void) strcpy(real_host,hent->h_name);
		}
#endif /* RESOLVER */
#else /* VMS */
		ptr = getenv("SYS$NODE");
		(void) strcpy(real_host, ptr);
		ptr = index(real_host, ':');
		if (ptr != NIL(char)) {
		    *ptr = '\0';
		}
		(void) strcat(real_host, DOMAIN_NAME);
		XmuCopyISOLatin1Lowered(real_host, real_host);
#endif
#endif /* RETURN_HOST */
	}
#ifdef INN
	}
#endif	/* INN */
        if ((ptr = getenv("HIDDENHOST")) != NIL(char)) {
	    (void) strncpy(host, ptr, sizeof(host)-1);
        } else {
	    (void) strcpy(host, real_host);
	}	    
    }

#ifndef INN    
    if ((ptr = getenv("HIDDENPATH")) != NIL(char)) {
	(void) strncpy(path, ptr, sizeof(path)-1);
    } else if ((ptr = getinfofromfile(PATH_FILE)) != NULL) {
	(void) strncpy(path, ptr, sizeof(path)-1);
    } else {
	(void) strncpy(path, host, sizeof(path)-1);
    }
#endif /* INN */
    
    /* If the host name is not a full domain name, put in the domain */
    if (index (host, '.') == NIL(char)) {
        char   *domain;
    
        if ((domain = getenv ("DOMAIN")) != NIL (char)) {
	    (void) strcat(host, domain);
#ifdef INN
	} else if (inn_domain != NIL(char)) {
	    (void) strcat(host, inn_domain);
#endif /* INN */	    
	} else if ((domain = getinfofromfile(DOMAIN_FILE)) != NULL) {
	    (void) strcat(host, domain);
        } else {
	    (void) strcat (host, DOMAIN_NAME);
	}
    }

    Header.host = XtNewString(host);
    Header.real_host = XtNewString(real_host);
#ifndef INN    
    Header.path = XtNewString(path);
#endif /* INN */

    if (app_resources.organization != NIL(char)) {
	Header.organization = XtNewString(app_resources.organization);
#ifndef apollo
    } else if ((ptr = getenv ("ORGANIZATION")) != NIL(char)) {
#else
    } else if ((ptr = getenv ("NEWSORG")) != NIL(char)) {
#endif
	Header.organization = XtNewString(ptr);
#ifdef INN
    } else if ((inn_org = (char *) GetConfigValue("organization")) != NIL(char)) {
	Header.organization = XtNewString(inn_org);
#endif /* INN */    
#ifdef ORG_FILE
    } else if ((ptr = getinfofromfile(ORG_FILE)) != NULL) {
	Header.organization = XtNewString(ptr);
#endif /* ORG_FILE */
    } else {
	Header.organization = XtNewString(ORG_NAME);
    }

#ifndef VMS
    pwd = getpwuid(getuid());
    if (Header.user = getenv("USER")) {
	Header.user = XtNewString(Header.user);
    } else if (Header.user = pwd->pw_name) {
	Header.user = XtNewString(Header.user);
    } else {
	Header.user = XtNewString("");
    }

    if (Header.real_user = pwd->pw_name) {
	Header.real_user = XtNewString(Header.real_user);
    } else {
	Header.real_user = XtNewString("");
    }

    if (Header.fullname = getenv("FULLNAME")) {
	Header.fullname = XtNewString(Header.fullname);
    } else if (Header.fullname = pwd->pw_gecos) {
	Header.fullname = XtNewString(Header.fullname);
    } else {
	Header.fullname = XtNewString("");
    }
#else
    if (Header.user = getenv("USER")) {
	Header.user = XtNewString(Header.user);
    } else {
	Header.user = XtNewString("");
    }
    XmuCopyISOLatin1Lowered(Header.user, Header.user);

    if (Header.fullname = getenv("FULLNAME")) {
	Header.fullname = XtNewString(Header.fullname);
    } else {
	Header.fullname = XtNewString("");
    }

#endif
    ptr = index(Header.fullname, ',');
    if (ptr != NIL(char)) {
	*ptr = '\0';
    }
    
    /* & expansion */
    ptr = index(Header.fullname, '&');
    if (ptr != NIL(char)) {
	char *p = buffer + (ptr - Header.fullname);

	buffer[0] = '\0';
	*ptr = '\0';
	(void) strcpy(buffer, Header.fullname);
	(void) strcat(buffer, Header.user);
	if (isascii(*p)) {
	    *p = toupper(*p);
	}
	ptr++;
	(void) strcat(buffer, ptr);
	FREE(Header.fullname);
	Header.fullname = XtNewString(buffer);
    }
    
    return;
}

/*
 * see if a field exists in the header that begins with `fieldName'
 */
static int fieldExists _ARGUMENTS((char *));

static int fieldExists(fieldName)
    char *fieldName;
{
    XawTextPosition pos, eoh;
    char buf[128];
    
#ifndef MOTIF
    XawTextSetInsertionPoint(ComposeText, 0);
#else
    XmTextSetCursorPosition(ComposeText, 0);
#endif
    XAWSEARCH(ComposeText, "\n\n", XawsdRight, eoh);
    if (eoh == XawTextSearchError) {
	eoh = 300; /* XXX XawTextGetLastPosition(ComposeText); */
    }
#ifndef MOTIF
    XawTextSetInsertionPoint(ComposeText, eoh);
#else
    XmTextSetCursorPosition(ComposeText, eoh);
#endif

    /*
     * The field is found if it either has a newline right before it,
     * or if it's the first thing in the posting.  We do the search
     * with the newline first, because that's the more common case,
     * and then, if it fails, try the same search without the new line
     * and see if we end up at position 0.
     */

    (void) strcpy(buf, "\n");
    (void) strcat(buf, fieldName);
    
    XAWSEARCH(ComposeText, buf, XawsdLeft, pos);
    if ((pos < eoh) && (pos >= 0)) {
	 return pos + 1;
    }

#ifndef MOTIF
    XawTextSetInsertionPoint(ComposeText, strlen(fieldName));
#else
    XmTextSetCursorPosition(ComposeText, strlen(fieldName));
#endif

    XAWSEARCH(ComposeText, fieldName, XawsdLeft, pos);
    if (pos == 0) {
	 return pos;
    }

    return XawTextSearchError;
}

/*
 * add a header field to a message.
 * this is a destructive operation.
 */
static void addField _ARGUMENTS((char *));

static void addField(field)
    char *field;
{
    XAWTEXTINSERT(ComposeText, 0, field);
    return;
}    

/*
 * remove all fields from a header that begin with `fieldName'.
 * this is a destructive operation.
 */
static void stripField _ARGUMENTS((char *));

static void stripField(fieldName)
    char *fieldName;
{
    XawTextPosition pos;
    XawTextPosition end;

    while ((pos = fieldExists(fieldName)) != XawTextSearchError) {
#ifndef MOTIF
	 XawTextBlock block;

	 XawTextSetInsertionPoint(ComposeText, pos);
	 block.firstPos = 0;
	 block.ptr = "\n";
	 block.length = 1;
	 block.format = XawFmt8Bit;
	 end = XawTextSearch(ComposeText, XawsdRight, &block);
#else
	 XmTextSetCursorPosition(ComposeText, pos);
	 end = MotifSearch(ComposeText, "\n", MOTIF_FORWARD);
#endif
	 if (end == XawTextSearchError) {
	      fprintf(stderr,
		      "ouch!  can't find newline in stripField\n");
	      return;
	 }
	 /* delete the line */
#ifndef MOTIF
	 block.ptr = NIL(char);
	 block.length = 0;
	 XawTextReplace(ComposeText, pos, end + 1, &block);
#else
	 XmTextReplace(ComposeText, pos, end + 1, "");
#endif
    }
    return;
}

/*
 * remove all fields from a header that begin with `fieldName'
 * and return the removed characters from any one of the fields
 * in `removed'.
 * this is (usually) a destructive operation.
 */

#ifdef INN
static void _returnField _ARGUMENTS((char *, char *, int));

static void _returnField(fieldName, removed, destroy)
    char *fieldName;
    char *removed;
    int destroy;

#define returnField(x,y) _returnField((x),(y),1)

#else /* INN */

static void returnField _ARGUMENTS((char *, char *));

static void returnField(fieldName, removed)
    char *fieldName;
    char *removed;

#endif /* INN */
{
     int pos;
     char *ptr;
#ifndef MOTIF
     Arg args[1];
#endif

#ifndef MOTIF
     XtSetArg(args[0], XtNstring, &ptr);
     XtGetValues(ComposeText, args, 1);
#else
     ptr = XmTextGetString(ComposeText);
#endif

     pos = fieldExists(fieldName);
     if (pos == XawTextSearchError) {
#ifdef MOTIF
	  FREE(ptr);
#endif
	  *removed = '\0';
	  return;
     }

     while (ptr[pos] && ptr[pos] != '\n')
	  *removed++ = ptr[pos++];
     *removed++ = ptr[pos++];
     *removed++ = '\0';

#ifdef INN
     if (destroy)
#endif /* INN */	 
       stripField(fieldName);

#ifdef MOTIF
     FREE(ptr);
#endif

     return;
}

static void destroyCompositionTopLevel()
{
    if (app_resources.editorCommand == NIL(char)) {
	XtPopdown(ComposeTopLevel);
    }
    XtDestroyWidget(ComposeTopLevel);
    ComposeTopLevel = (Widget) 0;
    return;
}

static void freeHeader()
{
    if (PostingMode != GRIPE) {
	FREE(Header.artFile);
	FREE(Header.newsgroups);
	FREE(Header.subject);
	FREE(Header.messageId);
	FREE(Header.followupTo);
	FREE(Header.references);
	FREE(Header.from);
	FREE(Header.replyTo);
	FREE(Header.distribution);
	FREE(Header.keywords);
	FREE(Header.user);
	FREE(Header.real_user);
	FREE(Header.fullname);
	FREE(Header.host);
	FREE(Header.real_host);
#ifndef INN	
	FREE(Header.path);
#endif /* INN */	
	FREE(Header.organization);
    }
    return;
}

#define CHECK_SIZE(s) \
     message_size += (s); \
     while (message_size >= message_total_size) { \
	  message_total_size += BUFFER_SIZE; \
	  message = XtRealloc(message, message_total_size); \
     }
			    
/*
 * add a subject field to the header of a message.
 * deal with supressing multiple '[rR][eE]: ' strings
 */
static void buildSubject _ARGUMENTS((char **, int *, int *));

static void buildSubject(msg, msg_size, msg_total_size)
    char **msg;
    int *msg_size, *msg_total_size;
{
    char *message = *msg;
    int message_size;
    int message_total_size;

    if (msg_size) {
	message_size = *msg_size;
	message_total_size = *msg_total_size;
    }

    if (STREQN(Header.subject, "Re: ", 4) ||
	STREQN(Header.subject, "RE: ", 4) ||
	STREQN(Header.subject, "re: ", 4)) {
	if (msg_size) {
	    CHECK_SIZE(strlen("Subject: "));
	}
	(void) strcat(message, "Subject: ");
    } else {
	if (msg_size) {
	    CHECK_SIZE(strlen("Subject: Re: "));
	}
	(void) strcat(message, "Subject: Re: ");
    }
    CHECK_SIZE(strlen(Header.subject) + 1);
    (void) strcat(message, Header.subject);
    (void) strcat(message, "\n");

    if (msg_size) {
	*msg_size = message_size;
	*msg_total_size = message_total_size;
    }
    *msg = message;

    return;
}

static void buildFrom _ARGUMENTS((char **, int *, int *));

static void buildFrom(msg, msg_size, msg_total_size)
    char **msg;
    int *msg_size, *msg_total_size;
{
    char *message = *msg;
    int message_size = *msg_size;
    int message_total_size = *msg_total_size;

    CHECK_SIZE(strlen("From: @ ()\n") + strlen(Header.user) +
	       strlen(Header.host) + strlen(Header.fullname));
    (void) strcat(message, "From: ");
    (void) strcat(message, Header.user);
    (void) strcat(message, "@");
    (void) strcat(message, Header.host);
    (void) strcat(message, " (");
    (void) strcat(message, Header.fullname);
    (void) strcat(message, ")\n");

    *msg = message;
    *msg_size = message_size;
    *msg_total_size = message_total_size;

    return;
}

BUTTON(compAbort,abort);

/*ARGSUSED*/
static void compAbortFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *ptr;
    int append = 0;

    destroyCompositionTopLevel();
    freeHeader();

    switch (PostingMode) {
    case POST:
    case FOLLOWUP:
	ptr = "article";
	break;
    case FOLLOWUP_REPLY:
	ptr = "article and mail message";
	break;
    default:
	ptr = "mail message";
	break;
    }

    if (count && (*count > 0) && (strcasecmp(string[0], "append") == 0))
	append = XRN_APPEND;

    mesgPane(XRN_INFO | append, MSG_ABORTED_MSG, ptr);

    return;
}

static void saveMessage _ARGUMENTS((char *, char *));

static void saveMessage(fname, msg)
    char *fname;
    char *msg;
{
    FILE *fp;
    char *file = utTildeExpand(fname);
#ifdef __osf__
    time_t clock;
#else
    long clock;
#endif

    if (file == NIL(char)) {
	mesgPane(XRN_SERIOUS, NO_FILE_NOT_SAVED_MSG);
	return;
    }
    
    (void) sprintf(error_buffer, "Saving in %s", file);
    infoNow(error_buffer);
    
    if ((fp = fopen(file, "a")) != NULL) {

	/* Provide initial 'From' line (note ctime() provides a newline) */

	if (Header.user == NULL) getHeader((art_num) 0);
	(void) time(&clock);
	if (fprintf(fp, "From %s %s", Header.user, ctime(&clock)) == EOF) {
	    goto finished;
	}

	/* copy body of message, protecting any embedded 'From' lines */

	while (*msg) {
	    if (STREQN(msg, "From ", 5)) {
		if (fputc('>', fp) == EOF) {
		    goto finished;
		}
	    }
	    while (*msg) {
		if (fputc(*msg, fp) == EOF) {
		    goto finished;
		}
		++msg;
		if (*(msg-1) == '\n') break;
	    }
	}
	
	/* ensure there is an empty line at the end */

	if (fputs("\n\n", fp) == EOF) {
	    goto finished;
	}

finished:
	(void) fclose(fp);

    }
    
    return;
}

BUTTON(compSave,save);

/*ARGSUSED*/
static void compSaveFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    Arg args[1];
    char *ptr;
    
#ifndef MOTIF
    XtSetArg(args[0], XtNstring, &ptr);
    XtGetValues(ComposeText, args, XtNumber(args));
    saveMessage(app_resources.savePostings, ptr);
#else
    ptr = XmTextGetString(ComposeText);
    saveMessage(app_resources.savePostings, ptr);
    XtFree(ptr);
#endif
    return;
}

static void saveDeadLetter _ARGUMENTS((char *));

static void saveDeadLetter(msg)
    char *msg;
{
    saveMessage(app_resources.deadLetters, msg);
    return;
}

#ifdef VMS
static Widget mailErrorDialog = NULL;

static void popDownMail _ARGUMENTS((Widget, XtPointer, XtPointer));

static void popDownMail(widget, client_data, call_data)
    Widget widget;
    XtPointer client_data;
    XtPointer call_data;
{
    PopDownDialog(mailErrorDialog);
    mailErrorDialog = NULL;
    return;
}

static void mailError _ARGUMENTS((int));

static void mailError(status)
    int status;
{
#include <descrip.h>
    static struct DialogArg args[] = {
	{"Click to continue", popDownMail, (XtPointer) -1},
    };
    char	VMSmessage[255];
    char	message[255];
    struct dsc$descriptor_s messageText;

    destroyCompositionTopLevel();
    freeHeader();

    messageText.dsc$b_class = DSC$K_CLASS_S;
    messageText.dsc$b_dtype = DSC$K_DTYPE_T;
    messageText.dsc$w_length = 255;
    messageText.dsc$a_pointer = &VMSmessage;

    sys$getmsg(status, &messageText, &messageText, 0, 0);
    VMSmessage[messageText.dsc$w_length] = '\0';
    (void) strcpy(message,"Error sending mail:");
    (void) strcat(message,VMSmessage);
    mailErrorDialog = CreateDialog(TopLevel, message, DIALOG_NOTEXT,
				   args, XtNumber(args));
    PopUpDialog(mailErrorDialog);
    return;
}
#endif /* VMS */

/*
 * WARNING: This function DESTROYS the newsgroups string passed into
 * it.  USE WITH CAUTION.  Caveat Emptor.  Look before crossing.
 * Carpe Diem.  And all that.
 */

static unsigned long newsgroupsStatusUnion _ARGUMENTS((char *));

static unsigned long newsgroupsStatusUnion(newsgroups)
    char *newsgroups;
{
    char *newsgroup;
    struct newsgroup *ngptr;
    unsigned long status = 0;

    for (newsgroup = strtok(newsgroups, ",\n"); newsgroup;
	 newsgroup = strtok(0, ",")) {
	if (avl_lookup(NewsGroupTable, newsgroup, (char **)&ngptr)) {
	    status |= ngptr->status;
	}
    }

    return(status);
}

BUTTON(compSend,send);

/*ARGSUSED*/
static void compSendFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char buffer[BUFFER_SIZE], *bufp = buffer;
    char *ErrMessage;
#ifdef VMS    
#include <maildef.h>
    struct _itemList {
	short item_size;
	short item_code;
	int   item_ptr;
	int   item_rtn_size;
	int   end_list;
    } itemList;
	 
    char toString[BUFFER_SIZE];
    int	context, status;
    char *textPtr, *nl;
#endif
#if defined(INN) || defined(VMS)
    char subjString[BUFFER_SIZE];
#endif
    /*
     * I am loathe to use buffers that are 1024 bytes long for the old
     * from line, new from line and sender line, since they are almost
     * certainly going to be much shorter than this, but this seems to
     * be the way things are done elsewhere in this file, so I'll
     * stick with it.
     */
    char *ngPtr;
    char oldFromString[BUFFER_SIZE];
    char newFromString[BUFFER_SIZE];
#ifndef INEWS
    /* INEWS puts on its own Sender field */
    char senderString[BUFFER_SIZE];
#endif /* INEWS */    
    Arg args[1];
    char *ptr;
    int mode, i, j, len, comma;
    unsigned long newsgroups_status;
    int tries = 1;
    int retry_editing = 0;
    int saved_dead = 0;
    int append = 0;

    if ((PostingMode == POST) || (PostingMode == FOLLOWUP) ||
	(PostingMode == FOLLOWUP_REPLY)) {
	mode = XRN_NEWS;
	if (fieldExists("Subject:") == XawTextSearchError) {
	    if (PostingMode == POST) {
		XBell(XtDisplay(TopLevel), 0);
		mesgPane(XRN_SERIOUS, NO_SUBJECT_MSG);
		if (app_resources.editorCommand)
		    Call_Editor(0);
		return;
	    }
	    bufp[0] = '\0';
	    buildSubject(&bufp, 0, 0);
	    addField(buffer);
	}

#ifdef INN
 	/* Let's be more strict, since inews doesn't see empty headers.  */
 	_returnField("Subject:", subjString, 0);
 	if (subjString[9] == '\n') {
	    XBell(XtDisplay(TopLevel), 0);
	    mesgPane(XRN_SERIOUS, EMPTY_SUBJECT_MSG);
	    if (app_resources.editorCommand)
		Call_Editor(0);
	    return;
 	}
#endif /* INN */	

	if ((PostingMode == FOLLOWUP) || (PostingMode == FOLLOWUP_REPLY)) {
	    if (fieldExists("References:") == XawTextSearchError) {
		(void) sprintf(buffer, "References: %s\n", Header.messageId);
		addField(buffer);
	    }
	}

#ifdef INEWS
	/*
	 * Extract any From: field currently in the message, and store it
	 * in the newFromString.
	 */
	returnField("From:", newFromString);
	/*
	 * This is the default that was displayed in the Composition pane
	 */
	(void) sprintf(oldFromString, "From: %s@%s (%s)\n",
		       Header.user, Header.host, Header.fullname);
	/*
	 * If we're using INEWS we pass on any From: header the user
	 * may have set explicitly.
	 */
	if (strcmp(oldFromString, newFromString)) {
	    addField(newFromString);
	}
	/*
	 * Get rid of any Sender: field currently in the mesage --
	 * the Sender: field should not ever be inserted by the user.
	 */
	stripField("Sender:");
#else
	/*
	 * Strip any From: field currently in the message, and store
	 * it in oldFromString.
	 */
	returnField("From:", oldFromString);
	/*
	 * If there was no From: field in the message, create a
	 * template one.
	 */
	if (*oldFromString == '\0')
	     (void) sprintf(oldFromString, "From: %s@%s (%s)\n",
			    Header.user, Header.host, Header.fullname);
	/*
	 * Now add into the message either the From: field that was
	 * pulled out or the one that was just created.
	 */
	addField(oldFromString);
	/*
	 * Now figure out what the default From: field should look
	 * like, with the real username in it (in case the user has
	 * specified a different username in the USER environment
	 * variable).
	 */
	(void) sprintf(newFromString, "From: %s@%s (%s)\n",
		       Header.real_user, Header.real_host, Header.fullname);
	/*
	 * Get rid of any Sender: field currently in the message --
	 * the Sender: field should not ever be inserted by the user.
	 */
	returnField("Sender:", senderString);
	/*
	 * If the default From: field is different from what's
	 * actually in the message, then insert a Sender: field with
	 * the default information in it.
	 */
	if (strcmp(oldFromString, newFromString)) {
	     (void) sprintf(senderString, "Sender: %s@%s (%s)\n",
			    Header.real_user, Header.real_host,
			    Header.fullname);
	     addField(senderString);
	}
#endif /* INEWS */	

	if (fieldExists("Newsgroups:") == XawTextSearchError) {
	    (void) sprintf(ngPtr = buffer, "Newsgroups: %s\n",
			   Header.newsgroups);
	    addField(buffer);
	}
	else {
	    returnField("Newsgroups:", ngPtr = buffer);
	}

	/*
	 * fix up the Newsgroups: field - inews can not handle spaces
	 * between group names
	 */
	len = strlen(buffer);
	j = 0;
	comma = 0;
	for (i = 0; i < len; i++) {
	    if (comma && (buffer[i] == ' ')) continue;
	    comma = 0;
	    ngPtr[j++] = buffer[i];
	    if (buffer[i] == ',') {
		comma = 1;
	    }
	}
	ngPtr[j] = '\0';
	addField(buffer);
	for (ngPtr = buffer + 11 /* skip "Newsgroups:" */; *ngPtr == ' ';
	     ngPtr++) /* empty */;
	newsgroups_status = newsgroupsStatusUnion(ngPtr);

	if (! (newsgroups_status & (NG_POSTABLE | NG_MODERATED))) {
	    mesgPane(XRN_ERROR, NO_POSTABLE_NG_MSG);
	    if (app_resources.editorCommand)
		Call_Editor(0);
	    return;
	}

#ifndef INN
	/* The inews in INN adds the Path: header for us, and has its own
	 * idea of policy.  We don't bother creating it, freeing it, or
	 * anything else in compose.c that has to do with it
	 */
	if (fieldExists("Path:") == XawTextSearchError) {
#if defined(INEWS) || defined(HIDE_PATH)
	    (void) sprintf(buffer, "Path: %s\n", Header.user);
#else
	    (void) sprintf(buffer, "Path: %s!%s\n", Header.path, Header.user);
#endif
	    addField(buffer);
	}
#endif /* INN */
	
/*	stripField("Message-ID:");  .............we need this !!! */
	stripField("Relay-Version:");
	stripField("Posting-Version:");
	
	/* Tell them who we are */
	stripField("X-newsreader:");
	(void) sprintf(buffer, "X-newsreader: xrn %s\n", XRN_VERSION);
	addField(buffer);
       
    } else {
	mode = XRN_MAIL;

	/* Tell them who we are */
	stripField("X-mailer:");
	(void) sprintf(buffer, "X-mailer: xrn %s\n", XRN_VERSION);
	addField(buffer);
    }

#ifndef MOTIF
    XtSetArg(args[0], XtNstring, &ptr);
    XtGetValues(ComposeText, args, XtNumber(args));
#else
    ptr = XmTextGetString(ComposeText);
#endif

    if (PostingMode == FOLLOWUP_REPLY) {
	tries = 2;
    }

    do {
	switch (postArticle(ptr, mode,&ErrMessage)) {
	case POST_FAILED:
	    XBell(XtDisplay(TopLevel), 0);
	    if (ErrMessage) {
		mesgPane(XRN_SERIOUS | append, ErrMessage);
		FREE(ErrMessage);
	    }
	    else {
		mesgPane(XRN_SERIOUS | append,
			 (mode == XRN_NEWS) ? COULDNT_POST_MSG :
			 COULDNT_SEND_MSG);
	    }
	    append = XRN_APPEND;
	    mesgPane(XRN_SERIOUS | append, SAVING_DEAD_MSG,
		     app_resources.deadLetters);
	    if (! saved_dead) {
		saveDeadLetter(ptr);
		saved_dead++;
	    }
	    retry_editing++;
	    break;
	
	case POST_NOTALLOWED:
	    mesgPane(XRN_SERIOUS | append, POST_NOTALLOWED_MSG);
	    append = XRN_APPEND;
	    mesgPane(XRN_SERIOUS | append, SAVING_DEAD_MSG,
		     app_resources.deadLetters);
	    if (! saved_dead) {
		saveDeadLetter(ptr);
		saved_dead++;
	    }
	    break;
	
	case POST_OKAY:
	    mesgPane(XRN_INFO | append, (mode == XRN_NEWS) ?
		     ((newsgroups_status & NG_MODERATED) ?
		      MAILED_TO_MODERATOR_MSG : ARTICLE_POSTED_MSG) :
		     MAIL_MESSAGE_SENT_MSG);
	    append = XRN_APPEND;
	    break;
	}
	tries--;
	if ((mode == XRN_NEWS) && (PostingMode == FOLLOWUP_REPLY)) {
	    mode = XRN_MAIL;
	}
    } while (tries > 0);

    
#ifdef MOTIF
    XtFree(ptr);
#endif

    destroyCompositionTopLevel();
    freeHeader();
    if (app_resources.editorCommand) {
	(void) unlink(EditorFileName);
	editing_status = NoEdit;
    }

#ifdef VMS
	// XXX this needs to be fixed
	returnField("To", toString);
	returnField("Subject", subjString);
	context = 0;

#ifndef MOTIF
	XtSetArg(args[0], XtNstring, &ptr);
	XtGetValues(ComposeText, args, XtNumber(args));
#else
        ptr = XmTextGetString(ComposeText);
#endif

	status = MAIL$SEND_BEGIN(&context, &0, &0);
	if ((status&1) != 1) {
	   mailError(status);
	   return;
	}
	itemList.item_code = MAIL$_SEND_TO_LINE;
	itemList.item_size = strlen(toString);
	itemList.item_ptr = &toString;
	itemList.item_rtn_size = 0;
	itemList.end_list = 0;
	status = MAIL$SEND_ADD_ATTRIBUTE(&context, &itemList, &0);
	if ((status&1) != 1) {
	   mailError(status);
	   return;
	}
	itemList.item_code = MAIL$_SEND_SUBJECT;
	itemList.item_size = strlen(subjString);
	itemList.item_ptr = &subjString;
	itemList.item_rtn_size = 0;
	itemList.end_list = 0;
	status = MAIL$SEND_ADD_ATTRIBUTE(&context, &itemList, &0);
	if ((status&1) != 1) {
	   mailError(status);
	   return;
	}
/*
 * Iterate over the composition string, extracting records
 * delimited by newlines and sending each as a record
 *
 */
	textPtr = ptr;
	if (*textPtr == '\n')
	    textPtr++;

	for (;;) {
/* Find the newline or end of string */
	    for (nl = textPtr; (*nl != '\0') && (*nl != '\n'); nl++);
	    itemList.item_code = MAIL$_SEND_RECORD;
	    itemList.item_size = nl - textPtr;
	    itemList.item_ptr = textPtr;
	    itemList.item_rtn_size = 0;
	    itemList.end_list = 0;
	    status = MAIL$SEND_ADD_BODYPART(&context, &itemList, &0);
	    if ((status&1) != 1) {
		mailError(status);
		return;
	    }
	
	    if (*nl == '\0')
		break;			/* end of string */
	    textPtr = ++nl;		/* skip the newline */
	}
	itemList.item_code = MAIL$_SEND_USERNAME;
	itemList.item_size = strlen(toString);
	itemList.item_ptr = &toString;
	itemList.item_rtn_size = 0;
	itemList.end_list = 0;
	status = MAIL$SEND_ADD_ADDRESS(&context, &itemList, &0);
	if ((status&1) != 1) {
	   mailError(status);
	   return;
	}
	status = MAIL$SEND_MESSAGE(&context, &0, &0);
	if ((status&1) != 1) {
	   mailError(status);
	   return;
	}
	mesgPane(XRN_INFO, MAIL_MESSAGE_SENT_MSG);

#ifdef MOTIF
        XtFree(ptr);
#endif

#endif

    return;
}


#define REALLOC_MAX (8 * BUFFER_SIZE)	/* somewhat arbitrary */
#ifndef MIN
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#endif

static char *
getIncludedArticleText()
{
     char *text, *prefix, input[256];
     int size = BUFFER_SIZE, prefix_size, cur_size;
     FILE *infile;
     
     text = XtMalloc(size);

     if (PostingMode == REPLY) {
	  (void) sprintf(text, "In article %s, you write:\n",
			 Header.messageId );
     } else if (PostingMode == FORWARD) {
	  (void) sprintf(text, "\n------ Forwarded Article %s\n------ From %s\n\n",
			 Header.messageId, Header.from);
     } else {
	  (void) sprintf(text, "In article %s, %s writes:\n",
			 Header.messageId, Header.from);
     }

     cur_size = strlen(text);

     if (app_resources.includeCommand && PostingMode != FORWARD) {
	  char cmdbuf[1024];

	  sprintf(cmdbuf, app_resources.includeCommand,
		  app_resources.includePrefix, Header.artFile);
	  infile = popen(cmdbuf, "r");
	  if (! infile) {
	       mesgPane(XRN_SERIOUS, CANT_INCLUDE_CMD_MSG);
	       FREE(text);
	       return(0);
	  }

	  prefix = "";
	  prefix_size = 0;
     }
     else {
	  infile = fopen(Header.artFile, "r");
	  if (! infile) {
	      mesgPane(XRN_SERIOUS, CANT_OPEN_ART_MSG, errmsg(errno));
	       FREE(text);
	       return(0);
	  }

	  if (app_resources.includeSep && PostingMode != FORWARD) {
	       prefix = app_resources.includePrefix;
	       prefix_size = strlen(prefix);
	  }
	  else {
	       prefix = "";
	       prefix_size = 0;
	  }
     }

     if (! app_resources.includeHeader) {
	  while (fgets(input, 256, infile)) {
	       if (*input == '\n')
		    break;
	  }
     }

     if (!feof(infile)) {
     while (fgets(input, 256, infile)) {
	  int line_size = strlen(input);
	  if (prefix_size + line_size > size - cur_size - 1) {
	       /* 
		* The point of doing the realloc'ing this way is that
		* messages that are bigger tend to be bigger (so to
		* speak), so as we read more lines in, we want to grow
		* the buffer faster to make more room for even more
		* lines.  However, we don't want to grow the buffer
		* *too* much at any one time, so the most we'll
		* realloc in one chunk is REALLOC_MAX.
		*/
	       size += MIN(size,REALLOC_MAX);
	       text = XtRealloc(text, size);
	  }
	  (void) strcpy(&text[cur_size], prefix);
	  cur_size += prefix_size;
	  (void) strcpy(&text[cur_size], input);
	  cur_size += line_size;
     }
     }

     if (PostingMode == FORWARD) {
	 int line_size;
	 (void) sprintf(input, "\n------ End of Forwarded Article\n");
	 line_size = strlen(input);
	 if (prefix_size + line_size > size - cur_size - 1) {
	     /* See above */
	     size += MIN(size,REALLOC_MAX);
	     text = XtRealloc(text, size);
	 }
	 (void) strcpy(&text[cur_size], prefix);
	 cur_size += prefix_size;
	 (void) strcpy(&text[cur_size], input);
	 cur_size += line_size;
     }

     if (app_resources.includeCommand)
	  (void) pclose(infile);
     else
	  (void) fclose(infile);

     return(text);
}


     
static void includeArticleText()
{
    XawTextPosition point;
    char *message_text;

#ifndef MOTIF
    point = XawTextGetInsertionPoint(ComposeText);
    XawTextDisableRedisplay(ComposeText);
#else
    point = XmTextGetCursorPosition(ComposeText);
#endif

    message_text = getIncludedArticleText();

    if (message_text) {
	 XAWTEXTINSERT(ComposeText, point, message_text);
	 FREE(message_text);
    }

#ifndef MOTIF
    XawTextEnableRedisplay(ComposeText);
#endif

    return;
}


static void includeArticleText3 _ARGUMENTS((char **));

static void includeArticleText3(message)
    char **message;
{
     char *message_text;

     message_text = getIncludedArticleText();

     if (message_text) {
	  int oldsize = strlen(*message);

	  *message = XtRealloc(*message, oldsize + strlen(message_text) + 1);
	  (void) strcpy(&(*message)[oldsize], message_text);
	  FREE(message_text);
     }

     return;
}

BUTTON(compIncludeArticle,include article);

/*ARGSUSED*/
static void compIncludeArticleFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (PostingMode == POST) {
	return;
    }
    includeArticleText();
    return;
}

#define XRNinclude_ABORT          0
#define XRNinclude_DOIT           1

static Widget IncludeBox = (Widget) 0;  /* box for typing in the name of a file */
static char *IncludeString = NULL; /* last input string */

/*
 * handler for the include box
 */
static void includeHandler _ARGUMENTS((Widget, XtPointer, XtPointer));

/*ARGSUSED*/
static void includeHandler(widget, client_data, call_data)
    Widget widget;
    XtPointer client_data;
    XtPointer call_data;
{
#ifndef MOTIF
    XawTextPosition point = XawTextGetInsertionPoint(ComposeText);
#else
    XawTextPosition point = XmTextGetCursorPosition(ComposeText);
#endif
    char *file;
    char line[256];
    FILE *filefp;

    if ((int) client_data != XRNinclude_ABORT) {
	/* copy the file */
	file = GetDialogValue(IncludeBox);
	if (! file) {
	    mesgPane(XRN_SERIOUS, NO_FILE_SPECIFIED_MSG);
	}
	else if ((filefp = fopen(utTildeExpand(file), "r")) != NULL) {
#ifndef MOTIF
	    XawTextDisableRedisplay(ComposeText);
#endif
	    while (fgets(line, 256, filefp) != NULL) {
		XAWTEXTINSERT(ComposeText, point, line);
		point += strlen(line);
	    }
	    (void) fclose(filefp);

#ifndef MOTIF
	    XawTextEnableRedisplay(ComposeText);
#endif
	    XtFree(IncludeString);
	    IncludeString = XtNewString(file);
	} else {
	    mesgPane(XRN_SERIOUS, CANT_OPEN_FILE_MSG, file, errmsg(errno));
	}
    }

    PopDownDialog(IncludeBox);
    IncludeBox = 0;

#ifdef MOTIF
#ifndef MOTIF_FIXED
/* Motif text bug -- doesn't display right after insertion so force update */
    if (XtIsRealized(ComposeText)) {
      Arg args[2];
      Dimension width, height;

      XtSetArg(args[0], XmNwidth, &width);
      XtSetArg(args[1], XmNheight, &height);
      XtGetValues(ComposeText, args, 2);
      XClearArea(XtDisplay(ComposeText), XtWindow(ComposeText),
		 0, 0, width, height, True);
    }
#endif
#endif

    return;
}    

BUTTON(compIncludeFile,include file);

/*ARGSUSED*/
static void compIncludeFileFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
	{"abort", includeHandler, (XtPointer) XRNinclude_ABORT},
	{"doit",  includeHandler, (XtPointer) XRNinclude_DOIT},
    };

    if (IncludeBox == (Widget) 0) {
	IncludeBox = CreateDialog(ComposeTopLevel, "    File Name?    ",
				  IncludeString == NULL ? DIALOG_TEXT
				   : IncludeString, args, XtNumber(args));
    }
    PopUpDialog(IncludeBox);

    return;
}


/*ARGSUSED*/
void processMessage(closure, source, id)
    XtPointer closure;
    int *source;
    XtInputId *id;
{
    FILE *filefp;
    struct stat buf;
    char buffer[10];
    char *ptr, *msg_type, *confirm1, *confirm2;
    int mode;
    char *ErrMessage;
    String append = "append";
    Cardinal count = 1;

    read(inchannel,buffer,1);
    if(editing_status != Completed || EditorFileName == NIL(char)){
	return;
    }

    switch (PostingMode) {
    case POST:
    case FOLLOWUP:
	msg_type = "article";
	confirm1 = "Post the article?";
	confirm2 = "Re-edit the article?";
	break;
    case FOLLOWUP_REPLY:
	msg_type = "article and mail message";
	confirm1 = "Post and send the message?";
	confirm2 = "Re-edit the message?";
	break;
    default:
	msg_type= "message";
	confirm1 = "Send the message?";
	confirm2 = "Re-edit the message?";
	break;
    }

    if ((filefp = fopen(EditorFileName, "r")) == NULL) {
	mesgPane(XRN_SERIOUS, CANT_OPEN_TEMP_MSG, EditorFileName, errmsg(errno));
	editing_status = NoEdit;
	compAbortFunction(0, 0, &append, &count);
	return;
    }

    if (fstat(fileno(filefp), &buf) == -1) {
	mesgPane(XRN_SERIOUS, CANT_STAT_TEMP_MSG, EditorFileName, errmsg(errno));
	(void) fclose(filefp);
	(void) unlink(EditorFileName);
	editing_status = NoEdit;
	compAbortFunction(0, 0, &append, &count);
	return;
    }

    if (originalBuf.st_mtime == buf.st_mtime) {
	mesgPane(XRN_INFO, NO_CHANGE_MSG, EditorFileName);
	(void) fclose(filefp);
	(void) unlink(EditorFileName);
	editing_status = NoEdit;
	compAbortFunction(0, 0, &append, &count);
	return;
    }

    if (buf.st_size == 0) {
	mesgPane(XRN_INFO, ZERO_SIZE_MSG, EditorFileName);
	(void) fclose(filefp);
	(void) unlink(EditorFileName);
	editing_status = NoEdit;
	compAbortFunction(0, 0, &append, &count);
	return;
    }

    ptr = XtMalloc(buf.st_size + 1);
    (void) fread(ptr, sizeof(char), buf.st_size, filefp);
    ptr[buf.st_size] = '\0';
    (void) fclose(filefp);

    /* pop up a confirm box */

    if (ConfirmationBox(TopLevel, confirm1, 0, 0) == XRN_CB_ABORT) {
	if (ConfirmationBox(TopLevel, confirm2, 0, 0) == XRN_CB_ABORT) {
	    mesgPane(XRN_SERIOUS, SAVING_DEAD_MSG,
		     app_resources.deadLetters);
	    compAbortFunction(0, 0, &append, &count);
	    saveDeadLetter(ptr);
	    FREE(ptr);
	    (void) unlink(EditorFileName);
	    editing_status = NoEdit;
	    return;
	} else {
	    Call_Editor(NIL(char));
	    FREE(ptr);
	    return;
	}
    }

#ifndef MOTIF
    XtVaSetValues(ComposeText, XtNstring, ptr, 0);
#else
    XtVaSetValues(ComposeText, XmNvalue, ptr, 0);
#endif
    FREE(ptr);

    compSendFunction(0, 0, 0, 0);
    return;
}


static int forkpid; 

extern int outchannel;

static int catch_sigchld _ARGUMENTS((int));

static int catch_sigchld(signo)
    int signo;
{

    if (signo != SIGCHLD) {
	/* whoops! */
	return 1;
    }
    if (forkpid != wait(0)) {
	/* whoops! */
	return 1;
    }
    (void) signal(SIGCHLD, SIG_DFL);
    editing_status = Completed;
    write(outchannel,"1",1);
    return 1;
}


static char *signatureFile _ARGUMENTS((void));


static int
Call_Editor(header)
char * header;
{
    char *signature;
    char *dsp, *file;
    char buffer[1024], buffer2[1024];
    FILE *filefp;
    String append = "append";
    Cardinal count = 1;

    if (editing_status == NoEdit) {
	editing_status = InProgress;
	if(EditorFileName != NIL(char))
	    FREE(EditorFileName);
	EditorFileName = XtNewString(utTempnam(app_resources.tmpDir, "xrn"));
	if ((filefp = fopen(EditorFileName, "w")) == NULL) {
	    mesgPane(XRN_SERIOUS, CANT_OPEN_TEMP_MSG, EditorFileName,
		     errmsg(errno));
	    FREE(header);
	    editing_status = NoEdit;
	    compAbortFunction(0, 0, &append, &count);
	    return (-1);
	}
	if (header) {
	    (void) fwrite(header, sizeof(char), utStrlen(header), filefp);
	    FREE(header);
	}
	else {
	    mesgPane(XRN_SERIOUS, NO_MSG_TEMPLATE_MSG);
	    editing_status = NoEdit;
	    compAbortFunction(0, 0, &append, &count);
	    return (-1);
	}

	(void) fclose(filefp);

	if (stat(EditorFileName, &originalBuf) == -1) {
	    mesgPane(XRN_SERIOUS, CANT_STAT_TEMP_MSG, EditorFileName,
		     errmsg(errno));
	    editing_status = NoEdit;
	    compAbortFunction(0, 0, &append, &count);
	    return (-1);
	}

    }

    /*
     * app_resources.editorCommand is a sprintf'able string with a %s where the
     * file name should be placed.  The result should be a command that
     * handles all editing and windowing.
     *
     * %D is replaced with the display name
     *
     * Examples are:
     *
     *   emacsclient %s
     *   xterm -display %D -e vi %s
     *   xterm -e microEmacs %s
     *
     */

    (void) strcpy(buffer2, app_resources.editorCommand);
    
    if ((dsp = strstr(buffer2, "%D")) != 0) {
	file = strstr(buffer2, "%s");
	*(dsp + 1) = 's';
	if (dsp > file) {
	    (void) sprintf(buffer, buffer2, EditorFileName, XDisplayName(0));
	} else {
	    (void) sprintf(buffer, buffer2, XDisplayName(0), EditorFileName);
	}
    } else {
	(void) sprintf(buffer, app_resources.editorCommand, EditorFileName);
    }

#ifndef VMS
#ifdef VFORK_SUPPORTED
    if ((forkpid = vfork()) == 0) 
#else
    if ((forkpid = fork()) == 0) 
#endif
       {
	    int i;
	    int maxdesc;

#if defined(__hpux)
	    maxdesc = (int) sysconf (_SC_OPEN_MAX);
#else
#ifdef SVR4
#include <ulimit.h>
	    maxdesc = ulimit(UL_GDESLIM);
#else
	    maxdesc = getdtablesize();
#endif
#endif
	    for (i = 3; i < maxdesc; i++) {
		(void) close(i);
	    }
	    (void) execl("/bin/sh", "sh", "-c", buffer, 0);
	    (void) fprintf(stderr, "execl of %s failed\n", buffer);
	    (void) _exit(127);
	}
	if (forkpid < 0) {
	    mesgPane(XRN_SERIOUS, CANT_EDITOR_CMD_MSG, buffer, errmsg(errno));
	    editing_status = NoEdit;
	    compAbortFunction(0, 0, &append, &count);
	    return (-1);
	}
	else {
	    signal(SIGCHLD, (SIG_PF0) catch_sigchld);
	}
#else
    system(buffer);
#endif

   return (0);
}


/*
 * brings up a new vertical pane, not moded, but maintains
 * enough state so that the current group and/or current
 * article can be changed
 *
 * only one compose pane at a time
 *
 * the pane consists of 3 parts: title bar, scrollable text window,
 * button box
 *
 * several functions:
 *    post article
 *    followup article
 *    reply to author
 *    include the text of the article (followup and reply)
 *    include the a file
 *    send a gripe
 *    forward a message
 *
 * If the editorCommand resource is set, then the command in it is
 * used to actually edit the article.  HOWEVER, a composition pane is
 * still created (although not popped up), so that it can be used for
 * processing the article text when the user is actually done editing
 * it.  Yeah, that's gross, but it makes it unnecessary to duplicate
 * code for the widget or the external editor.
 */
static int composePane _ARGUMENTS((char *, char *, XawTextPosition));

static int composePane(titleString, header, point)
    char *titleString;
    char *header;
    XawTextPosition point;
{
    char *signature;
    Widget pane, buttonBox, label;
#ifndef MOTIF
    static char titleStorage[LABEL_SIZE];
#else
    static XmString titleStorage;
    Position x, y;
    Dimension width, height;
#endif
    Position x_val, y_val;
    Dimension width_val, height_val;
#ifndef MOTIF
    static Arg labelArgs[] = {
	{XtNlabel, (XtArgVal) titleStorage},
	{XtNskipAdjust, (XtArgVal) True},
    };
#else
    static Arg labelArgs[] = {
	{XmNlabelString, (XtArgVal) NULL},
	{XmNrecomputeSize, (XtArgVal) False},
	{XmNskipAdjust, (XtArgVal) True},
    };
#endif
#ifndef MOTIF
    static Arg boxArgs[] = {
	{XtNskipAdjust, (XtArgVal) True},
    };
#else
    static Arg boxArgs[] = {
	{XmNskipAdjust, (XtArgVal) True},
    };
#endif
    static Arg shellArgs[] = {
	{XtNinput, (XtArgVal) True},
	{XtNsaveUnder, (XtArgVal) False},
    };
#ifndef MOTIF
    static Arg textArgs[] = {
	{XtNstring, (XtArgVal) NULL},
	{XtNtype,  (XtArgVal) XawAsciiString},
	{XtNeditType,  (XtArgVal) XawtextEdit},
    };
#else
    static Arg textArgs[] = {
      {XmNvalue, (XtArgVal) NULL},
    };
#endif

    if (ComposeTopLevel != (Widget) 0) {
	mesgPane(XRN_SERIOUS, ONE_COMPOSITION_ONLY_MSG);
	FREE(header);
	return(-1);
    }

    if ((PostingMode == FORWARD) ||
	(app_resources.editorCommand &&
	 (PostingMode != POST) && (PostingMode != GRIPE))) {
	includeArticleText3(&header);
    }

    if ((signature = signatureFile()) != NIL(char)) {
	/* current header + '\n' + signature + '\0' */
	header = XtRealloc(header, strlen(header) + strlen(signature) + 2);
	strcat(header, "\n");
	strcat(header, signature);
    }

    ComposeTopLevel = XtCreatePopupShell("Composition", topLevelShellWidgetClass,
					 TopLevel, shellArgs, XtNumber(shellArgs));
#ifndef MOTIF
    XtVaGetValues(Frame,
		  XtNx, (XtPointer) &x_val,
		  XtNy, (XtPointer) &y_val,
		  XtNwidth, (XtPointer) &width_val,
		  XtNheight, (XtPointer) &height_val,
		  (char *) 0);

    pane = XtVaCreateManagedWidget("pane", panedWidgetClass, ComposeTopLevel,
				   XtNx, (XtArgVal) x_val,
				   XtNy, (XtArgVal) y_val,
				   XtNwidth, (XtArgVal) width_val,
				   XtNheight, (XtArgVal) height_val,
				   (char *) 0);

    (void) strcpy(titleStorage, titleString);

    label = XtCreateManagedWidget("label", labelWidgetClass, pane,
				  labelArgs, XtNumber(labelArgs));

    XtSetArg(textArgs[0], XtNstring, header);
    ComposeText = XtCreateManagedWidget("text", asciiTextWidgetClass, pane,
					textArgs, XtNumber(textArgs));

    buttonBox = XtCreateManagedWidget("box", boxWidgetClass, pane,
				      boxArgs, XtNumber(boxArgs));

    XtCreateManagedWidget("compAbort", commandWidgetClass, buttonBox,
			  compAbortArgs, XtNumber(compAbortArgs));
    
    XtCreateManagedWidget("compSend", commandWidgetClass, buttonBox,
			  compSendArgs, XtNumber(compSendArgs));
    
    XtCreateManagedWidget("compSave", commandWidgetClass, buttonBox,
			  compSaveArgs, XtNumber(compSaveArgs));

    if ((PostingMode != POST) && 
	(PostingMode != GRIPE) &&
	(PostingMode != FORWARD)) {
      XtCreateManagedWidget("compIncludeArticle", commandWidgetClass,
			    buttonBox, compIncludeArticleArgs,
			    XtNumber(compIncludeArticleArgs));
    }
    XtCreateManagedWidget("compIncludeFile", commandWidgetClass, buttonBox,
			  compIncludeFileArgs, XtNumber(compIncludeFileArgs));
#else
    pane = XmCreatePanedWindow(ComposeTopLevel, "pane",
			       NULL, 0);
    XtManageChild(pane);
    
    titleStorage = XmStringCreate(titleString, XmSTRING_DEFAULT_CHARSET);
    XtSetArg(labelArgs[0], XmNlabelString, titleStorage);
    label = XmCreateLabel(pane, "label", labelArgs, XtNumber(labelArgs));
    XtManageChild(label);
    
    XtSetArg(textArgs[0], XmNvalue, header);
    ComposeText = XmCreateScrolledText(pane, "text",
				       textArgs, XtNumber(textArgs));
    XtManageChild(ComposeText);
    
    buttonBox = XmCreateRowColumn(pane, "box",
				  boxArgs, XtNumber(boxArgs));
    XtManageChild(buttonBox);
    
    XtManageChild(XmCreatePushButton(buttonBox, "compAbort",
				     compAbortArgs, XtNumber(compAbortArgs)));
    
    XtManageChild(XmCreatePushButton(buttonBox, "compSend",
				     compSendArgs, XtNumber(compSendArgs)));
    
    XtManageChild(XmCreatePushButton(buttonBox, "compSave",
				     compSaveArgs, XtNumber(compSaveArgs)));

    if ((PostingMode != POST) && 
	(PostingMode != GRIPE) &&
	(PostingMode != FORWARD)) {
      XtManageChild(XmCreatePushButton(buttonBox, "compIncludeArticle",
				       compIncludeArticleArgs,
				       XtNumber(compIncludeArticleArgs)));
    }
    XtManageChild(XmCreatePushButton(buttonBox, "compIncludeFile",
				     compIncludeFileArgs,
				     XtNumber(compIncludeFileArgs)));
#endif

    if (app_resources.editorCommand != NIL(char)) {
	return(Call_Editor(header));
    }
    else {
	XtRealizeWidget(ComposeTopLevel);
	XtSetKeyboardFocus(ComposeTopLevel, ComposeText);

	XtVaGetValues(label,
		      XtNheight, (XtPointer) &height_val,
		      (char *) 0);

#ifndef MOTIF
	XawPanedSetMinMax(label, (int) height_val, (int) height_val);
	XawPanedAllowResize(ComposeText, True);
#else /* MOTIF */
	{
	    Arg args[3];

	    XtSetArg(args[0], XmNpaneMaximum, height_val);
	    XtSetArg(args[1], XmNpaneMinimum, height_val);
	    XtSetArg(args[2], XmNallowResize, True);
	    XtSetValues(ComposeText, args, 2);
	}
#endif /* MOTIF */
    
	{
	    static Cursor compCursor = (Cursor) 0;

	    if (compCursor == (Cursor) 0) {
		XColor colors[2];

		colors[0].pixel = app_resources.pointer_foreground;
		colors[1].pixel = app_resources.pointer_background;
		XQueryColors(XtDisplay(TopLevel),
			     DefaultColormap(XtDisplay(TopLevel),
					     DefaultScreen(XtDisplay(TopLevel))),
			     colors, 2);
		compCursor = XCreateFontCursor(XtDisplay(ComposeTopLevel),
					       XC_left_ptr);
		XRecolorCursor(XtDisplay(TopLevel), compCursor,
			       &colors[0], &colors[1]);
	    }
	    XDefineCursor(XtDisplay(ComposeTopLevel),
			  XtWindow(ComposeTopLevel), compCursor);
	}

#ifndef MOTIF
	XawTextSetInsertionPoint(ComposeText, point);
#else /* MOTIF */
	XmTextSetCursorPosition(ComposeText, point);
#endif /* MOTIF */

	XtPopup(ComposeTopLevel, XtGrabNone);

	FREE(header);
	return(0);
    }
}


static char *signatureFile()
/*
 * return a string containing the contents of the users signature file
 *   (in a static buffer)
 *
 * if the signature file is bigger than MAX_SIGNATURE_SIZE, return NIL(char).
 */
{
    char *file;
    FILE *infofp;
    int (*close_func)() = 0;
    long count;
    static char info[MAX_SIGNATURE_SIZE+5];
    static char *retinfo;
    static char *sigfile = NULL;

#if defined(INEWS_READS_SIG)
    /* these posting modes do not go through INEWS, so include the signature */
    if ((PostingMode != REPLY) && (PostingMode != GRIPE)) {
	return 0;
    }
#endif /* INEWS_READS_SIG */

    if (sigfile != NULL) {
	FREE(sigfile);
    }

    if ((file = utTildeExpand(app_resources.signatureFile)) == NIL(char)) {
	return NIL(char);
    }

    /* handle multiple signatures */
    while (1) {
	/* find an appropriate sig */
	struct newsgroup *newsgroup = CurrentGroup;
	char *psigfile = NIL(char);
	char *ptr;

	if(!CurrentGroup) {
	    sigfile = XtMalloc(strlen(file) + 10);
	} else {
	    /* signature according to group or hierarchy */
	    sigfile = XtMalloc(strlen(file) + 10 + strlen(newsgroup->name));
	    if (app_resources.localSignatures) {
		ptr = localKillFile(newsgroup, FALSE);
		(void) strcpy(sigfile, ptr);
		ptr = rindex(sigfile, '/');
		ptr[1] = 0;
		ptr = (char *) (sigfile + strlen(app_resources.expandedSaveDir));
		while ((psigfile = rindex(sigfile, '/')) != NULL) {
		    if (psigfile < ptr) {
			psigfile = NULL;
			break;
		    }
		    psigfile[0] = 0;
		    (void) strcat(sigfile, "/SIGNATURE");
		    if (! access(sigfile, F_OK)) {
			/* FOUND */
			break;
		    }
		    psigfile[0] = 0;
		}
	    }
	    else {
		(void) strcpy(sigfile, file);
		(void) strcat(sigfile, "-");
		ptr = rindex(sigfile, '-');
		(void) strcat(sigfile, newsgroup->name);
		(void) strcat(sigfile, ".");
		while ((psigfile = rindex(sigfile, '.')) != NIL(char)) {
		    if (psigfile < ptr) {
			psigfile = NIL(char);
			break;
		    }
		    psigfile[0] = 0;
		    if (! access(sigfile, F_OK)) {
			/*FOUND*/ 
			break;
		    }
		}
	    }
	}

	if (psigfile != NIL(char)) {
	    /*FOUND*/
	    break;
	}

	/* signature according to posting mode. */
	(void) strcpy(sigfile, file);
	(void) strcat(sigfile, ".");
	(void) strcat(sigfile, PostingModeStrings[PostingMode]);

	if (! access(sigfile, F_OK)) {
	    /*FOUND*/
	    break;
	}

	(void) strcpy(sigfile, file);
	if (! access(sigfile, F_OK)) {
	    break;
	} else {
	    return NIL(char);
	}
    }

    {
	char cmdbuf[1024];
	char *p = rindex(sigfile, '/');

	infofp = NIL(FILE);

	if (! p) {
	     p = sigfile;
	}
	else {
	     p++;
	}

	if (app_resources.executableSignatures && (! access(sigfile, X_OK))) {
	    if (app_resources.signatureNotify) {
		mesgPane(XRN_INFO, EXECUTING_SIGNATURE_MSG, sigfile);
	    }
	    (void) sprintf(cmdbuf, "%s %s %s %s",
			    sigfile,
			    (CurrentGroup ? CurrentGroup->name : "NIL"),
			    PostingModeStrings[PostingMode],
			    (Header.artFile ? Header.artFile : "NIL"));
	    infofp = popen(cmdbuf, "r");
	    close_func = pclose;
	    if (!infofp) {
		mesgPane(XRN_SERIOUS, CANT_EXECUTE_SIGNATURE_MSG, sigfile);
	    }
	} else {
	    if (app_resources.signatureNotify) {
		mesgPane(XRN_INFO, READING_SIGNATURE_MSG, sigfile);
	    }
	}

	if (!infofp) {
	    infofp = fopen(sigfile, "r");
	    close_func = fclose;
	    if (! infofp) {
		mesgPane(XRN_SERIOUS, CANT_READ_SIGNATURE_MSG, sigfile,
			 errmsg(errno));
		return NIL(char);
	    }
	}
    }

    (void) strcpy(info, "-- \n");
    count = fread(&info[4], sizeof(char), sizeof(info) - 4, infofp);
    info[count + 4] = '\0';

    if (! feof(infofp)) {
	/* Signature file is too big */
	retinfo = NIL(char);
    }
    else if (strncmp(info + 4, "--\n", 3) == 0 ||
	     strncmp(info + 4, "-- \n", 4) == 0) {
	retinfo = info + 4;
    } else {
	retinfo = info;
    }

    (void) (*close_func)(infofp);

    return retinfo;
}


static void followup_or_reply _ARGUMENTS((int, int));

static void followup_or_reply(followup, reply)
    int followup, reply;
{
    struct newsgroup *newsgroup = CurrentGroup;
    art_num current = newsgroup->current;
    char title[LABEL_SIZE];
    char *message = 0;
    int message_size = 0, message_total_size = 0;
    int OldPostingMode = PostingMode;

#ifdef GENERATE_EXTRA_FIELDS
    long clock;
    char *ptr, timeString[30];
#endif /* GENERATE_EXTRA_FIELDS */

    getHeader(current);
    Header.artFile = XtNewString(newsgroup->articles[INDEX(current)].filename);

    CHECK_SIZE(1);
    message[0] = '\0';

    if (followup) {
	if (! strcmp(Header.followupTo, "poster")) {
	    char *msg, *p1, *p2;
	    int ret;

	    if (reply) {
		msg = "`Followup-To' line in message says to reply to poster.\nIgnore it and post as well, or just send E-mail?";
		p1 = "post and send mail";
		p2 = "just send mail";
	    }
	    else {
		msg = "`Followup-To' line in message says to reply to poster.\nPost followup or mail reply?";
		p1 = "post";
		p2 = "send mail";
	    }
	    ret = ConfirmationBox(TopLevel, msg, p1, p2);
	    if (ret == XRN_CB_CONTINUE) {
		followup = 0;
		reply = 1;
	    }
	    else {
		FREE(Header.followupTo);
		Header.followupTo = XtNewString("");
	    }
	}
    }

    if (followup) {
	if (reply) {
	    PostingMode = FOLLOWUP_REPLY;
	}
	else {
	    PostingMode = FOLLOWUP;
	}
    }
    else {
	PostingMode = REPLY;
    }
    
    (void) sprintf(title, "%s to article %ld in %s",
		   followup ? (reply ? "Followup and reply" : "Followup") :
		   "Reply", current, newsgroup->name);

    if (followup) {
#ifndef INN
#if defined(INEWS) || defined(HIDE_PATH)
	CHECK_SIZE(strlen("Path: \n") + strlen(Header.user));
	(void) sprintf(&message[strlen(message)], "Path: %s\n", Header.user);
#else /* INEWS or HIDE_PATH */
	CHECK_SIZE(strlen("Path: !\n") + strlen(Header.path) +
		   strlen(Header.user));
	(void) sprintf(&message[strlen(message)], "Path: %s!%s\n",
		       Header.path, Header.user);
#endif /* INEWS or HIDE_PATH */
#else /* INN */
	CHECK_SIZE(1);
	*message = '\0';
	message_size = 0;
#endif /* INN */
	if ((Header.followupTo != NIL(char)) && (*Header.followupTo != '\0')) {
	    Header.newsgroups = XtNewString(Header.followupTo);
	}

	CHECK_SIZE(strlen("Newsgroups: \n") + strlen(Header.newsgroups));
	(void) sprintf(&message[strlen(message)], "Newsgroups: %s\n",
		       Header.newsgroups);

	CHECK_SIZE(strlen("Distribution: \n"));
	(void) strcat(message, "Distribution: ");
	if ((Header.distribution != NIL(char)) && (*Header.distribution != '\0')) {
	    CHECK_SIZE(strlen(Header.distribution));
	    (void) strcat(message, Header.distribution);
	} else if (app_resources.distribution) {
	    CHECK_SIZE(strlen(app_resources.distribution));
	    (void) strcat(message, app_resources.distribution);
#ifdef DISTRIBUTION
	} else {
	    CHECK_SIZE(strlen(DISTRIBUTION));
	    (void) strcat(message, DISTRIBUTION);
#endif /* DISTRIBUTION */
	}
	(void) strcat(message, "\n");

	CHECK_SIZE(strlen("Followup-To: \n"));
	(void) strcat(message, "Followup-To: \n");

	CHECK_SIZE(strlen("References:  \n") +
		   strlen(Header.references) + strlen(Header.messageId));
	(void) strcat(message, "References: ");
	(void) strcat(message, Header.references);
	(void) strcat(message, " ");
	(void) strcat(message, Header.messageId);
	(void) strcat(message, "\n");

	buildFrom(&message, &message_size, &message_total_size);

#ifdef GENERATE_EXTRA_FIELDS
	/* stuff to generate Message-ID and Date... */
	(void) time(&clock);
	(void) strcpy(timeString, asctime(gmtime(&clock)));
	ptr = index(timeString, '\n');
	*ptr = '\0';
	CHECK_SIZE(strlen("Date:  GMT\n") + strlen(timeString));
	(void) sprintf(&message[strlen(message)], "Date: %s GMT\n", timeString);
	CHECK_SIZE(strlen("Message-ID: \n") + strlen(ptr = gen_id()));
	(void) sprintf(&message[strlen(message)], "Message-ID: %s\n", ptr);
#endif /* GENERATE_EXTRA_FIELDS */

	CHECK_SIZE(strlen("Organization: \n") + strlen(Header.organization));
	(void) strcat(message, "Organization: ");
	(void) strcat(message, Header.organization);
	(void) strcat(message, "\n");

	CHECK_SIZE(strlen("Keywords: \n"));
	(void) strcat(message, "Keywords: ");
	if ((Header.keywords != NIL(char)) && (*Header.keywords != '\0')) {
	    CHECK_SIZE(strlen(Header.keywords));
	    (void) strcat(message, Header.keywords);
	}
	(void) strcat(message, "\n");
    }

    if (reply) {
	char *reply_addr;

	reply_addr = ((Header.replyTo != NIL(char)) &&
		      (*Header.replyTo != '\0')) ? Header.replyTo : Header.from;
	CHECK_SIZE(strlen("To: \n") + strlen(reply_addr));
	(void) sprintf(&message[strlen(message)], "To: %s\n", reply_addr);

        if (app_resources.cc == True) {
	    CHECK_SIZE(strlen("Cc: \n") + strlen(Header.user));
	    sprintf(&message[strlen(message)], "Cc: %s\n", Header.user);
	}

	if (app_resources.extraMailHeaders) {
	    CHECK_SIZE(strlen("X-Newsgroups: \nIn-reply-to: %s\n") +
		       strlen(Header.newsgroups) + strlen(Header.messageId));
	    (void) sprintf(&message[strlen(message)],
			   "X-Newsgroups: %s\nIn-reply-to: %s\n",
			   Header.newsgroups, Header.messageId);
	}
    }
	
    if (app_resources.replyTo != NIL(char)) {
	CHECK_SIZE(strlen("Reply-To: \n") + strlen(app_resources.replyTo));
	(void) strcat(message, "Reply-To: ");
	(void) strcat(message, app_resources.replyTo);
	(void) strcat(message, "\n");
    }

    buildSubject(&message, &message_size, &message_total_size);

    CHECK_SIZE(1);
    (void) strcat(message, "\n");

    if (composePane(title, message, (XawTextPosition) strlen(message))) {
	 PostingMode = OldPostingMode;
    }

    return;
}


/* public functions 'reply', 'gripe', 'forward', 'followup', 'followup_and_reply', and 'post' */

void reply()
{
    followup_or_reply(0, 1);
}

char *bugTemplate = 
"X VERSION, RELEASE, and PATCH LEVEL:\n\
    [e.g., X Version 11, Release 4, Patch Level 18]\n\
\n\
CLIENT MACHINE and OPERATING SYSTEM:\n\
    [e.g., DECStation 5000/200 running Ultrix 4.2, ...]\n\
\n\
NEWS SYSTEM and VERSION:\n\
    [e.g., Cnews, Bnews, InterNet News, ...]\n\
\n\
SYNOPSIS:\n\
    [brief description of the problem and where it is located]\n\
\n\
DESCRIPTION:\n\
    [detailed description]\n\
\n\
REPEAT BY:\n\
    [what you did to get the error]\n\
\n\
SAMPLE FIX:\n\
    [preferred, but not necessary.  Please send context diffs (diff -c -b)]";

void gripe()
{
    char title[LABEL_SIZE];
    char *message = 0;
    int OldPostingMode = PostingMode;
    int message_size = 0, message_total_size = 0;
    XawTextPosition point;

    (void) strcpy(title, "Gripe");
    CHECK_SIZE(strlen("To: \nSubject: GRIPE about XRN \n") +
	       strlen(GRIPES) + strlen(XRN_VERSION) + 1);
    (void) sprintf(message,
		   "To: %s\nSubject: GRIPE about XRN %s\n",
		   GRIPES, XRN_VERSION);

    if (app_resources.replyTo != NIL(char)) {
	CHECK_SIZE(strlen("Reply-To: \n") +
		   strlen(app_resources.replyTo));
	(void) strcat(message, "Reply-To: ");
	(void) strcat(message, app_resources.replyTo);
	(void) strcat(message, "\n");
    }

    CHECK_SIZE(strlen(bugTemplate) + 2);
    (void) strcat(message, "\n");

    point = (XawTextPosition) (strlen(message) +
			       (index(bugTemplate, '[') - bugTemplate));
    (void) strcat(message, bugTemplate);
    (void) strcat(message, "\n");

    PostingMode = GRIPE;

    if (composePane(title, message, point))
	 PostingMode = OldPostingMode;

    return;
}


void forward()
{
    char title[LABEL_SIZE];
    char *message = 0;
    int message_size = 0, message_total_size = 0;
    struct newsgroup *newsgroup = CurrentGroup;
    art_num current = newsgroup->current;
    int OldPostingMode = PostingMode;
    
    getHeader(current);
    Header.artFile = XtNewString(newsgroup->articles[INDEX(current)].filename);

    PostingMode = FORWARD;
    
    (void) sprintf(title, "Forward article %ld in %s to a user", current,
		   newsgroup->name);
    CHECK_SIZE(strlen("To: \nSubject:   [ #]\n") +
	       strlen(Header.subject) + strlen(newsgroup->name) +
	       (int) (current / 10) + 1);
    if (app_resources.ccForward == True) {
	CHECK_SIZE(strlen("Cc: \n") + strlen(Header.user));
	(void) sprintf(message, "To: \nCc: %s\nSubject: %s  [%s #%ld]\n",
		       Header.user,
		       Header.subject, newsgroup->name, current);
    } else {
	(void) sprintf(message, "To: \nSubject: %s  [%s #%ld]\n",
		       Header.subject, newsgroup->name, current);
    }

    if (app_resources.replyTo != NIL(char)) {
	CHECK_SIZE(strlen("Reply-To: \n") + strlen(app_resources.replyTo));
	(void) strcat(message, "Reply-To: ");
	(void) strcat(message, app_resources.replyTo);
	(void) strcat(message, "\n");
    }

    CHECK_SIZE(1);
    (void) strcat(message, "\n");

    if (composePane(title, message, (XawTextPosition) 4))
	 PostingMode = OldPostingMode;

    return;
}

#ifdef GENERATE_EXTRA_FIELDS
/*
 *  generate a message id
 */
static char genid[132];

static char *gen_id()
{
    char *timestr, *cp;
    time_t cur_time;

    time(&cur_time);
    timestr = ctime(&cur_time);

    (void) sprintf(genid, "<%.4s%.3s%.2s.%.2s%.2s%.2s@%s>",
		    &timestr[20], &timestr[4], &timestr[8],
		    &timestr[11], &timestr[14], &timestr[17],
		    Header.host);
    cp = &genid[8];

    if (*cp == ' ') {
	do {
	    *cp = *(cp + 1); 
	} while (*cp++);
    }

    return(genid);
}
#endif /* GENERATE_EXTRA_FIELDS */

void followup()
{
    followup_or_reply(1, 0);
}

void followup_and_reply()
{
    followup_or_reply(1, 1);
}


void post(ingroupp)
    int ingroupp;
{
    struct newsgroup *newsgroup = CurrentGroup;
    char title[LABEL_SIZE], buffer[BUFFER_SIZE];
    char *message = 0;
    int message_size = 0, message_total_size = 0;
    int OldPostingMode = PostingMode;
    XawTextPosition point = 0;

#ifdef GENERATE_EXTRA_FIELDS
    long clock;
    char *ptr, timeString[30];
#endif /* GENERATE_EXTRA_FIELDS */
   
    getHeader((art_num) 0);

    if (!ingroupp || (! newsgroup)) {
	FREE(Header.newsgroups);
	Header.newsgroups = XtNewString("");
	(void) sprintf(title, "Post article");
    } else {
	(void) sprintf(title, "Post article to `%s'", CurrentGroup->name);
    }

#ifndef INN
#if defined(INEWS) || defined(HIDE_PATH)
    CHECK_SIZE(strlen("Path: \n") + strlen(Header.user));
    (void) sprintf(message, "Path: %s\n", Header.user);
#else /* INEWS or HIDE_PATH */
    CHECK_SIZE(strlen("Path: !\n") + strlen(Header.path) +
	       strlen(Header.user));
    (void) sprintf(message, "Path: %s!%s\n", Header.path, Header.user);
#endif /* INEWS or HIDE_PATH */
#else /* INN */
    CHECK_SIZE(1);
    *message = '\0';
    message_size = 0;
#endif /* INN */
    CHECK_SIZE(strlen("Newsgroups: \n") + strlen(Header.newsgroups));
    (void) strcat(message, "Newsgroups: ");
    (void) strcat(message, Header.newsgroups);
    (void) strcat(message, "\n");
    if (! (*Header.newsgroups)) {
	 point = strlen(message) - 1;
    }

    CHECK_SIZE(strlen("Distribution: \n"));
    (void) strcat(message, "Distribution: ");
    if (app_resources.distribution) {
	CHECK_SIZE(strlen(app_resources.distribution));
	(void) strcat(message, app_resources.distribution);
#ifdef DISTRIBUTION
    } else {
	CHECK_SIZE(strlen(DISTRIBUTION));
	(void) strcat(message, DISTRIBUTION);
#endif
    }
    (void) strcat(message, "\n");

#ifdef GENERATE_EXTRA_FIELDS
    /* stuff to generate Message-ID and Date... */
    (void) time(&clock);
    (void) strcpy(timeString, asctime(gmtime(&clock)));
    ptr = index(timeString, '\n');
    *ptr = '\0';
    CHECK_SIZE(strlen("Date:  GMT\n") + strlen(timeString));
    (void) sprintf(buffer, "Date: %s GMT\n", timeString);
    (void) strcat(message, buffer);

    CHECK_SIZE(strlen("Message-ID: \n") + strlen(ptr = gen_id()));
    (void) sprintf(buffer, "Message-ID: %s\n", ptr);
    (void) strcat(message, buffer);
#endif /* GENERATE_EXTRA_FIELDS */

    CHECK_SIZE(strlen("Followup-To: \n"));
    (void) strcat(message, "Followup-To: \n");

    buildFrom(&message, &message_size, &message_total_size);

    CHECK_SIZE(strlen("Organization: \n") + strlen(Header.organization));
    (void) strcat(message, "Organization: ");
    (void) strcat(message, Header.organization);
    (void) strcat(message, "\n");

    CHECK_SIZE(strlen("Subject: \n"));
    (void) strcat(message, "Subject: \n");
    if (! point) {
	 point = strlen(message) - 1;
    }

    CHECK_SIZE(strlen("Keywords: \n"));
    (void) strcat(message, "Keywords: \n");

    CHECK_SIZE(1);
    (void) strcat(message, "\n");

    PostingMode = POST;
    if (composePane(title, message, point))
	 PostingMode = OldPostingMode;
    
    return;
}

void cancelArticle()
{
    struct newsgroup *newsgroup = CurrentGroup;
    art_num current = newsgroup->current;
    char buffer[BUFFER_SIZE];
    char *message = 0;
    int message_size = 0, message_total_size = 0;
    char *bufptr;
    char *ErrMessage;
    char *distribution = 0;

    getHeader(current);

    /* verify that the user can cancel the article */
    bufptr = index(Header.from, '@');
    if (bufptr != NIL(char)) {
	bufptr++;
	(void) strcpy(buffer, bufptr);
	if ((bufptr = index(buffer, ' ')) != NIL(char)) {
	    *bufptr = '\0';
	}
	if (strncmp(Header.host, buffer, utStrlen(Header.host))
	   || (strncmp(Header.user, Header.from, utStrlen(Header.user)) 
	      && strcmp(Header.user, "root"))) {
	    mesgPane(XRN_SERIOUS, "Not entitled to cancel the article");
	    freeHeader();
	    return;
        }
    }

#ifndef INN
#if defined(INEWS) || defined(HIDE_PATH)
    CHECK_SIZE(strlen("Path: \n") + strlen(Header.user));
    (void) sprintf(message, "Path: %s\n", Header.user);
#else /* INEWS or HIDE_PATH */
    CHECK_SIZE(strlen("Path: !\n") + strlen(Header.path) +
	       strlen(Header.user));
    (void) sprintf(message, "Path: %s!%s\n", Header.path, Header.user);
#endif /* INEWS or HIDE_PATH */
#else /* INN */
    CHECK_SIZE(1);
    *message = '\0';
    message_size = 0;
#endif /* INN */
    buildFrom(&message, &message_size, &message_total_size);

    CHECK_SIZE(strlen("Subject: cancel \n") + strlen(Header.messageId));
    (void) strcat(message, "Subject: cancel ");
    (void) strcat(message, Header.messageId);
    (void) strcat(message, "\n");

    CHECK_SIZE(strlen("Newsgroups: ,control\n") + strlen(Header.newsgroups));
    (void) strcat(message, "Newsgroups: ");
    (void) strcat(message, Header.newsgroups);
    (void) strcat(message, ",control\n");

    CHECK_SIZE(strlen("References:  \n") + strlen(Header.references) +
	       strlen(Header.messageId));
    (void) strcat(message, "References: ");
    (void) strcat(message, Header.references);
    (void) strcat(message, " ");
    (void) strcat(message, Header.messageId);
    (void) strcat(message, "\n");

    if ((Header.distribution != NIL(char)) && (*Header.distribution != '\0'))
	distribution = Header.distribution;
    else if (app_resources.distribution)
	distribution = app_resources.distribution;

    if (distribution) {
	CHECK_SIZE(strlen ("Distribution: \n") + strlen(distribution));
	(void) strcat(message, "Distribution: ");
	(void) strcat(message, distribution);
	(void) strcat(message, "\n");
    }

    CHECK_SIZE(strlen("Control: cancel \n") + strlen(Header.messageId));
    (void) strcat(message, "Control: cancel ");
    (void) strcat(message, Header.messageId);
    (void) strcat(message, "\n");

    freeHeader();

    switch (postArticle(message, XRN_NEWS,&ErrMessage)) {
    case POST_FAILED:
	if (ErrMessage) {
	    mesgPane(XRN_SERIOUS, ErrMessage);
	    mesgPane(XRN_SERIOUS | XRN_APPEND, CANCEL_ABORTED_MSG);
	    FREE(ErrMessage);
	}
	else {
	    mesgPane(XRN_SERIOUS, CANCEL_ABORTED_MSG);
	}
	break;

    case POST_NOTALLOWED:
	mesgPane(XRN_SERIOUS, POST_NOTALLOWED_MSG);
	mesgPane(XRN_SERIOUS | XRN_APPEND, CANCEL_ABORTED_MSG);
	break;
	    
    case POST_OKAY:
	mesgPane(XRN_INFO, CANCELLED_ART_MSG);
	break;
    }

    return;

}
