#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: compose.c,v 1.82 1995-10-17 10:43:14 jik Exp $";
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
#include <sys/file.h>
#include <signal.h>
#include "error_hnds.h"
#ifdef RESOLVER
#include <sys/socket.h>
#include <netdb.h>
#endif
#include <pwd.h>
#include "compose.h"
#include "ngMode.h"
#include "Text.h"
#include "ButtonBox.h"

#if defined(aiws) || defined(DGUX)
struct passwd *getpwuid();
struct passwd *getpwnam();
#endif

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>

#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Dialog.h>

#if defined(sun) && (defined(sparc) || defined(mc68000)) && !defined(SOLARIS)
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
#include "artstruct.h"
#include "internals.h"
#include "server.h"
#include "mesg.h"
#include "xrn.h"
#include "xmisc.h"
#include "buttons.h"
#include "butdefs.h"
#include "mesg_strings.h"

#ifdef INN
#include <libinn.h>
#endif

static void saveDeadLetter _ARGUMENTS((char *));

#ifdef GENERATE_EXTRA_FIELDS
static char *gen_id _ARGUMENTS((void));
#endif

/*
  XawFmt8Bit only available starting in X11R6.
  */
#ifndef XawFmt8Bit
#define XawFmt8Bit FMT8BIT
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

static int Call_Editor _ARGUMENTS((/* Boolean */ int save_message));

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
#define POST_MAIL	6
#define MAIL		7
static int PostingMode = POST;
static char *PostingModeStrings[] =
    { "post", "followup", "reply", "forward", "gripe", "followup",
      "post", "mail" };

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
#ifndef INEWS
    char *sender_host;
#endif
#ifndef INN
    char *path;
#endif
    char *organization;

    char  *date; /* added to header structure....... */
};

static void freeHeader _ARGUMENTS((struct header *));

/*
 * storage for the header information needed for building, checking,
 * and repairing the header.  Once this is created, the user can go
 * to another article or another group and still use the composition
 * window
 */

static struct header Header, CancelHeader;


BUTTON(compAbort,abort);
BUTTON(compSave,save);
BUTTON(compSend,send);
BUTTON(compIncludeArticle,include article);
BUTTON(compIncludeFile,include file);

/*
 * Get the username of the user running the program, or an empty string
 * if the username can't be determined.  The returned string is
 * allocated and should be freed when it's no longer needed.
 */

char *getUser()
{
    char *user;
    struct passwd *pwd;

    if ((user = getenv("USER")))
	return XtNewString(user);
    if ((pwd = getpwuid(getuid())) && pwd->pw_name)
	return XtNewString(pwd->pw_name);
    return XtNewString("");
}


/*
  Here is how the various host-related fields in outgoing messages are
  determined (under UNIX):

  real_host:
  * Contents of the file UUCPNAME, if it's defined, or
  * The string REAL_HOST, if it's defined, or
  * gethostname (+ gethostbyname, if RESOLVER is defined)
  Note that real_host is *not* domainified if it doesn't have a period
  in it.  If you want it to be a fully qualified domain name, make
  sure that one of the sources above will return a fully qualified
  domain name.

  sender_host:
  * The string SENDER_HOST, if it's defined, or
  * Contents of real_host
  
  host:
  * Contents of the HIDDENHOST environment variable, if it's set, or
  * INN "fromhost" configuration value, if using INN and it's set, or
  * Contents of the file HIDDEN_FILE, if it's defined and non-empty, or
  * The string RETURN_HOST, if it's defined, or
  * Contents of real_host
  If the derived value for host doesn't have a period in it, the
  domain is determined (see below) and appended to it.

  path: (not used under INN)
  * Contents of the HIDDENPATH environment variable, if it's set, or
  * Contents of the file PATH_FILE, if it's defined and non-empty, or
  * Contents of real_host
  
  host's domain: (only if host doesn't already have a period in it)
  * Contents of the DOMAIN environment variable, if it's set, or
  * INN "domain" configuration value, if using INN and it's set, or
  * Contents of the file DOMAIN_FILE, if it's defined and non-empty, or
  * The string DOMAIN_NAME, if it's defined, or
  * Error signalled and post/mail aborted

  Here's how they're used:

  real_host:
  * Message-ID: field (if GENERATE_EXTRA_FIELDS is defined)

  sender_host:
  * Sender: field, if necessary,
  
  Host:
  * From: field,
  * Verification that the user is allowed to cancel an article

  path:
  * Path: field
  
  */
  
    
/*
 * Get the header and other important information for an article.
 *
 * If called with article equal to zero, will only set up the non-article
 * specific entries.
 *
 * Returns XRN_OKAY on success, XRN_ERROR otherwise.  If it fails,
 * displays an error message before returning.
 */
static int getHeader _ARGUMENTS((art_num, struct header *));

static int getHeader(article, Header)
    art_num article;
    struct header *Header;
{
    struct newsgroup *newsgroup = CurrentGroup;
    struct passwd *pwd;
    char host[HOST_NAME_SIZE], buffer[BUFFER_SIZE], *ptr;
    char real_host[HOST_NAME_SIZE];
#ifndef INEWS
    char sender_host[HOST_NAME_SIZE];
#endif
#ifdef INN
    char *inn_org;
#else
    char path[HOST_NAME_SIZE];
#endif

    (void) memset((char *)Header, 0, sizeof(*Header));

    if (article > 0) {
	Header->article = article;
	xhdr(newsgroup, article, "newsgroups", &Header->newsgroups);
	xhdr(newsgroup, article, "subject", &Header->subject);
	xhdr(newsgroup, article, "message-id", &Header->messageId);
	xhdr(newsgroup, article, "followup-to", &Header->followupTo);
	xhdr(newsgroup, article, "references", &Header->references);
	xhdr(newsgroup, article, "from", &Header->from);
	xhdr(newsgroup, article, "reply-to", &Header->replyTo);
	xhdr(newsgroup, article, "distribution", &Header->distribution);
	xhdr(newsgroup, article, "keywords", &Header->keywords);
    } else {
	/* information for 'post' */
	if (newsgroup) {
	    Header->newsgroups = XtNewString(newsgroup->name);
	} else {
	    Header->newsgroups = XtNewString("");
	}
    }

    /*
     * since I'm lazy down below, I'm going to replace NIL pointers with ""
     */
    if (Header->newsgroups == NIL(char)) {
	Header->newsgroups = XtNewString("");
    }
    if (Header->subject == NIL(char)) {
	Header->subject = XtNewString("");
    }
    if (Header->messageId == NIL(char)) {
	Header->messageId = XtNewString("");
    }
    if (Header->followupTo == NIL(char)) {
	Header->followupTo = XtNewString("");
    }
    if (Header->references == NIL(char)) {
	Header->references = XtNewString("");
    }
    if (Header->from == NIL(char)) {
	Header->from = XtNewString("");
    }
    if (Header->replyTo == NIL(char)) {
	Header->replyTo = XtNewString("");
    }
    if (Header->distribution == NIL(char)) {
	Header->distribution = XtNewString("");
    }
    if (Header->keywords == NIL(char)) {
	Header->keywords = XtNewString("");
    }

    real_host[0] = '\0';
    real_host[sizeof(real_host)-1] = '\0';

#ifndef INEWS
    sender_host[0] = '\0';
    sender_host[sizeof(sender_host)-1] = '\0';
#endif

    host[0] = '\0';
    host[sizeof(host)-1] = '\0';

#ifndef INN
    path[0] = '\0';
    path[sizeof(path)-1] = '\0';
#endif

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
    		(void) strncpy(real_host, xbuf, sizeof(real_host)-1);
    		(void) fclose(uup);
        }
    }
#endif

#ifdef REAL_HOST
    if (! real_host[0])
	(void) strncpy(real_host, REAL_HOST, sizeof(real_host)-1);
#endif

    if (! real_host[0]) {
# ifdef RESOLVER
	struct hostent *hent;
# endif
	(void) gethostname(real_host, sizeof(real_host));
# ifdef RESOLVER
	if ((hent = gethostbyname(real_host)))
	    (void) strncpy(real_host, hent->h_name, sizeof(real_host)-1);
# endif
    }

#ifndef INEWS
#ifdef SENDER_HOST
    if (! sender_host[0])
	(void) strncpy(sender_host, SENDER_HOST, sizeof(sender_host)-1);
#endif

    if (! sender_host[0])
	strcpy(sender_host, real_host);
#endif /* ! INEWS */

    if (! host[0])
	if ((ptr = getenv("HIDDENHOST")))
	    (void) strncpy(host, ptr, sizeof(host)-1);

#ifdef INN
    if (! host[0]) {
	/* always let values in inn.conf take precedence */
	char *inn_fromhost = GetConfigValue("fromhost");
	if (inn_fromhost)
	    (void) strncpy(host, inn_fromhost, sizeof(host)-1);
    }
#endif /* INN */

#ifdef HIDDEN_FILE
    if (! host[0])
	if (ptr = getinfofromfile(HIDDEN_FILE))
	    (void) strncpy(host, ptr, sizeof(host)-1);
#endif /* HIDDEN_FILE */

#ifdef RETURN_HOST
    if (! host[0])
	(void) strncpy(host, RETURN_HOST, sizeof(host)-1);
#endif

    if (! host[0])
	(void) strcpy(host, real_host);

#ifndef INN
    if (! path[0])
	if ((ptr = getenv("HIDDENPATH")))
	    (void) strncpy(path, ptr, sizeof(path)-1);
# ifdef PATH_FILE
    if (! path[0])
	if (ptr = getinfofromfile(PATH_FILE))
	    (void) strncpy(path, ptr, sizeof(path)-1);
# endif
    if (! path[0])
	(void) strcpy(path, real_host);
#endif /* ! INN */
    
    /* If the host name is not a full domain name, put in the domain */
    if (index (host, '.') == NIL(char)) {
        char *domain = app_resources.domainName;

	if (! domain)
	    domain = getenv("DOMAIN");
#ifdef INN
	if (! domain)
	    domain = GetFileConfigValue("domain");
#endif
#ifdef DOMAIN_FILE
	if (! domain)
	    domain = getinfofromfile(DOMAIN_FILE);
#endif
#ifdef DOMAIN_NAME
	if (! domain)
	    domain = DOMAIN_NAME;
#endif

	if (! domain) {
	    mesgPane(XRN_SERIOUS, 0, NO_DOMAIN_MSG);
	    freeHeader(Header);
	    return XRN_ERROR;
	}

	(void) strncat(host, domain, sizeof(host) - strlen(host));
    }

    Header->host = XtNewString(host);
    Header->real_host = XtNewString(real_host);
#ifndef INEWS
    Header->sender_host = XtNewString(sender_host);
#endif
#ifndef INN    
    Header->path = XtNewString(path);
#endif /* INN */

    if (app_resources.organization != NIL(char)) {
	Header->organization = XtNewString(app_resources.organization);
#ifndef apollo
    } else if ((ptr = getenv ("ORGANIZATION")) != NIL(char)) {
#else
    } else if ((ptr = getenv ("NEWSORG")) != NIL(char)) {
#endif
	Header->organization = XtNewString(ptr);
#ifdef INN
    } else if ((inn_org = GetConfigValue("organization"))) {
	Header->organization = XtNewString(inn_org);
#endif /* INN */    
#ifdef ORG_FILE
    } else if ((ptr = getinfofromfile(ORG_FILE)) != NULL) {
	Header->organization = XtNewString(ptr);
#endif /* ORG_FILE */
    } else {
#ifdef ORG_NAME
	Header->organization = XtNewString(ORG_NAME);
#else
	Header->organization = XtNewString("");
#endif /* ORG_NAME */
    }

    Header->user = getUser();

    pwd = getpwuid(getuid());

    if ((Header->real_user = pwd->pw_name)) {
	Header->real_user = XtNewString(Header->real_user);
    } else {
	Header->real_user = XtNewString("");
    }

    if ((Header->fullname = getenv("FULLNAME"))) {
	Header->fullname = XtNewString(Header->fullname);
    } else if ((Header->fullname = pwd->pw_gecos)) {
	Header->fullname = XtNewString(Header->fullname);

	ptr = index(Header->fullname, ',');
	if (ptr != NIL(char)) {
	    *ptr = '\0';
	}
    
	/* & expansion */
	ptr = index(Header->fullname, '&');
	if (ptr != NIL(char)) {
	    char *p = buffer + (ptr - Header->fullname);

	    buffer[0] = '\0';
	    *ptr = '\0';
	    (void) strcpy(buffer, Header->fullname);
	    (void) strcat(buffer, Header->user);
	    if (isascii(*p)) {
		*p = toupper(*p);
	    }
	    ptr++;
	    (void) strcat(buffer, ptr);
	    FREE(Header->fullname);
	    Header->fullname = XtNewString(buffer);
	}
    } else {
	Header->fullname = XtNewString("");
    }
    return XRN_OKAY;
}

/*
 * See if a field exists in the header that begins with `fieldName'.
 * If last_field is non-null, then the field being searched for must
 * be before that position in the header; otherwise, the last matching
 * field in the header is returned.  Therefore, last_field can be used
 * to iterate through all fields with a specific name in the header.
 */
static int fieldExists _ARGUMENTS((char *, int *));

static int fieldExists(fieldName, last_field)
    char *fieldName;
    int *last_field;
{
    long pos, eoh;
    char buf[128];
    
    if (last_field) {
	eoh = *last_field;
    }
    else {
	eoh = TextSearch(ComposeText, 0, TextSearchRight, "\n\n");
	if (eoh == -1)
	    eoh = TextGetLength(ComposeText);
    }

    /*
     * The field is found if it either has a newline right before it,
     * or if it's the first thing in the posting.  We do the search
     * with the newline first, because that's the more common case,
     * and then, if it fails, try the same search without the new line
     * and see if we end up at position 0.
     */

    (void) strcpy(buf, "\n");
    (void) strcat(buf, fieldName);

    pos = TextSearch(ComposeText, eoh, TextSearchLeft, buf);
    if (pos > -1)
	return pos + 1;

    pos = TextSearch(ComposeText, strlen(fieldName), TextSearchLeft,
		     fieldName);
    if ((pos > -1) && (pos < eoh))
	return pos;

    return -1;
}

static Boolean fieldIsMultiple _ARGUMENTS((char *));

static Boolean fieldIsMultiple(fieldName)
    char *fieldName;
{
    int pos;

    if ((pos = fieldExists(fieldName, 0)) < 0)
	return False;
    if (fieldExists(fieldName, &pos) < 0)
	return False;

    return True;
}

/*
 * add a header field to a message.
 * this is a destructive operation.
 */
static void addField _ARGUMENTS((char *));

static void addField(field)
    char *field;
{
    TextReplace(ComposeText, field, strlen(field), 0, 0);
}    

/*
  Get the bounds of one of the fields in the header with the specified
  field name.  If the field isn't in the header, than the function
  will return false.  Otherwise, "left" will be the first character
  position in the field (including the field name), and "right" will
  be the first character position after the field.
  */
static Boolean fieldBounds _ARGUMENTS((char *, long *, long *));

static Boolean fieldBounds(fieldName, left, right)
    char *fieldName;
    long *left, *right;
{
    String str;

    if ((*left = fieldExists(fieldName, 0)) < 0)
	return False;

    str = TextGetString(ComposeText);

    *right = *left;
    do {
	*right = TextSearch(ComposeText, *right, TextSearchRight, "\n");
	if (*right < 0) {
	    *right = strlen(str);
	    break;
	}
	(*right)++;
    } while ((str[*right] == ' ') || (str[*right] == '\t'));

    FREE(str);
    return True;
}

    
/*
 * remove all fields from a header that begin with `fieldName'.
 * this is a destructive operation.
 */
static void stripField _ARGUMENTS((char *));

static void stripField(fieldName)
    char *fieldName;
{
    long left, right;

    while (fieldBounds(fieldName, &left, &right))
	TextReplace(ComposeText, "", 0, left, right);
}

#define CHECK_SIZE_EXTENDED(ptr,size,total,incr) \
     size += (incr); \
     while (size >= total) { \
	  total += BUFFER_SIZE; \
	  ptr = XtRealloc(ptr, total); \
     }

#define CHECK_SIZE(s) CHECK_SIZE_EXTENDED(message,message_size,\
					  message_total_size,s)
			    
/*
 * Return the characters from any one of the fields in the header
 * matching fieldName.
 */

static void returnField _ARGUMENTS((char *, char **, int *, int *));

static void returnField(fieldName, out, msg_size, msg_total_size)
    char *fieldName;
    char **out;
    int *msg_size, *msg_total_size;
{
    char *message = *out;
    int message_size = *msg_size;
    int message_total_size = *msg_total_size;

    long left, right;

    if (fieldBounds(fieldName, &left, &right)) {
	String ptr = TextGetString(ComposeText);
	CHECK_SIZE(right - left);
	(void) strncpy(message, ptr + left, right - left);
	message[right - left] = '\0';
	FREE(ptr);
    }
    else
	*message = '\0';

    *out = message;
    *msg_size = message_size;
    *msg_total_size = message_total_size;

    return;
}

static void destroyCompositionTopLevel()
{
    if (app_resources.editorCommand == NIL(char)) {
	XtPopdown(ComposeTopLevel);
    }
    TextDestroy(ComposeText);
    XtDestroyWidget(ComposeTopLevel);
    ComposeTopLevel = (Widget) 0;
    return;
}

Boolean pendingCompositionP()
{
    return(ComposeTopLevel ? True : False);
}

static void freeHeader(Header)
    struct header *Header;
{
    FREE(Header->artFile);
    FREE(Header->newsgroups);
    FREE(Header->subject);
    FREE(Header->messageId);
    FREE(Header->followupTo);
    FREE(Header->references);
    FREE(Header->from);
    FREE(Header->replyTo);
    FREE(Header->distribution);
    FREE(Header->keywords);
    FREE(Header->user);
    FREE(Header->real_user);
    FREE(Header->fullname);
    FREE(Header->host);
    FREE(Header->real_host);
#ifndef INEWS
    FREE(Header->sender_host);
#endif
#ifndef INN	
    FREE(Header->path);
#endif /* INN */	
    FREE(Header->organization);
    return;
}

/*
 * add a subject field to the header of a message.
 * deal with supressing multiple '[rR][eE]: ' strings
 */
static void buildSubject _ARGUMENTS((struct header *, char **, int *, int *));

static void buildSubject(Header, msg, msg_size, msg_total_size)
    struct header *Header;
    char **msg;
    int *msg_size, *msg_total_size;
{
    char *message = *msg;
    int message_size = *msg_size, message_total_size = *msg_total_size;

    if (STREQN(Header->subject, "Re: ", 4) ||
	STREQN(Header->subject, "RE: ", 4) ||
	STREQN(Header->subject, "re: ", 4)) {
	CHECK_SIZE(sizeof("Subject: ") - 1);
	(void) strcat(message, "Subject: ");
    } else {
	CHECK_SIZE(sizeof("Subject: Re: ") - 1);
	(void) strcat(message, "Subject: Re: ");
    }
    CHECK_SIZE(strlen(Header->subject) + 1);
    (void) strcat(message, Header->subject);
    (void) strcat(message, "\n");

    *msg = message;
    *msg_size = message_size;
    *msg_total_size = message_total_size;

    return;
}

#ifdef INEWS
static void buildFrom _ARGUMENTS((struct header *, char **, int *, int *));

static void buildFrom(Header, msg, msg_size, msg_total_size)
    struct header *Header;
    char **msg;
    int *msg_size, *msg_total_size;

#else /* ! INEWS */

#define buildFrom(Header,msg,msg_size,msg_total_size) \
	_buildFrom(Header,(msg), False, False, (msg_size), (msg_total_size))
#define buildRealFrom(Header,msg,msg_size,msg_total_size) \
	_buildFrom(Header,(msg), True, False, (msg_size), (msg_total_size))
#define buildSender(Header,msg,msg_size,msg_total_size) \
	_buildFrom(Header,(msg), True, True, (msg_size), (msg_total_size))

static void _buildFrom _ARGUMENTS((struct header *, char **, /* Boolean */ int,
				  /* Boolean */ int, int *, int *));

static void _buildFrom(Header, msg, real_addr, sender, msg_size, msg_total_size)
    struct header *Header;
    char **msg;
    Boolean real_addr, sender;
    int *msg_size, *msg_total_size;
#endif /* INEWS */
{
    char *message = *msg;
    int message_size = *msg_size;
    int message_total_size = *msg_total_size;
    char *fieldName, *user, *host;

#ifndef INEWS
    if (sender)
	fieldName = "Sender";
    else
#endif
	fieldName = "From";

#ifndef INEWS
    if (real_addr) {
	user = Header->real_user;
	host = Header->sender_host;
    }
    else {
#endif
	user = Header->user;
	host = Header->host;
#ifndef INEWS
    }
#endif

    CHECK_SIZE(sizeof(": @ ()\n") - 1 + strlen(fieldName) + strlen(user) +
	       strlen(host) + strlen(Header->fullname));
    
    (void) sprintf(&message[strlen(message)], "%s: %s@%s (%s)\n",
		   fieldName, user, host, Header->fullname);

    *msg = message;
    *msg_size = message_size;
    *msg_total_size = message_total_size;

    return;
}

static void buildReplyTo _ARGUMENTS((char **, int *, int *));

static void buildReplyTo(msg, msg_size, msg_total_size)
    char **msg;
    int *msg_size, *msg_total_size;
{
    char *message = *msg;
    int message_size = *msg_size;
    int message_total_size = *msg_total_size;

    if (app_resources.replyTo) {
	CHECK_SIZE(sizeof("Reply-To: \n") - 1 + strlen(app_resources.replyTo));
	(void) sprintf(&message[strlen(message)], "Reply-To: %s\n",
		       app_resources.replyTo);
    }

    *msg = message;
    *msg_size = message_size;
    *msg_total_size = message_total_size;

    return;
}

static void buildReferences _ARGUMENTS((struct header *, char **, int *, int *));

static void buildReferences(Header, msg, msg_size, msg_total_size)
    struct header *Header;
    char **msg;
    int *msg_size, *msg_total_size;
{
    char *message = *msg;
    int message_size = *msg_size;
    int message_total_size = *msg_total_size;

    CHECK_SIZE(sizeof("References:  \n") - 1 + strlen(Header->references) +
	       strlen(Header->messageId));
    (void) sprintf(&message[strlen(message)], "References: %s %s\n",
		   Header->references, Header->messageId);

    *msg = message;
    *msg_size = message_size;
    *msg_total_size = message_total_size;

    return;
}

static void buildPath _ARGUMENTS((struct header *, char **, int *, int *));

static void buildPath(Header, msg, msg_size, msg_total_size)
    struct header *Header;
    char **msg;
    int *msg_size, *msg_total_size;
{
    char *message = *msg;
    int message_size = *msg_size;
    int message_total_size = *msg_total_size;

#ifndef INN
#if defined(INEWS) || defined(HIDE_PATH)
    CHECK_SIZE(sizeof("Path: \n") - 1 + strlen(Header->user));
    (void) sprintf(&message[strlen(message)], "Path: %s\n", Header->user);
#else /* INEWS or HIDE_PATH */
    CHECK_SIZE(sizeof("Path: !\n") - 1 + strlen(Header->path) +
	       strlen(Header->user));
    (void) sprintf(&message[strlen(message)], "Path: %s!%s\n",
		   Header->path, Header->user);
#endif /* INEWS or HIDE_PATH */
#endif /* INN */

    *msg = message;
    *msg_size = message_size;
    *msg_total_size = message_total_size;

    return;
}

static Boolean buildNewsgroups _ARGUMENTS((struct header *, char **,
					   int *, int *));

static Boolean buildNewsgroups(Header, msg, msg_size, msg_total_size)
    struct header *Header;
    char **msg;
    int *msg_size, *msg_total_size;
{
    char *message = *msg;
    int message_size = *msg_size;
    int message_total_size = *msg_total_size;

    char *ngptr;

    if (*Header->followupTo)
	if (! strcmp(Header->followupTo, "poster"))
	    ngptr = Header->newsgroups;
	else
	    ngptr = Header->followupTo;
    else if (*Header->newsgroups)
	ngptr = Header->newsgroups;
    else
	ngptr = "";

    CHECK_SIZE(sizeof("Newsgroups: \n") - 1 + strlen(ngptr));
    (void) sprintf(&message[strlen(message)], "Newsgroups: %s\n", ngptr);

    *msg = message;
    *msg_size = message_size;
    *msg_total_size = message_total_size;

    return(*ngptr ? True : False);
}

static void buildDistribution _ARGUMENTS((struct header *, char **,
					  int *, int *));

static void buildDistribution(Header, msg, msg_size, msg_total_size)
    struct header *Header;
    char **msg;
    int *msg_size, *msg_total_size;
{
    char *message = *msg;
    int message_size = *msg_size;
    int message_total_size = *msg_total_size;

    char *distptr;

    if (*Header->distribution)
	distptr = Header->distribution;
    else if (app_resources.distribution)
	distptr = app_resources.distribution;
    else
#ifdef DISTRIBUTION
	distptr = DISTRIBUTION
#else
	distptr = ""
#endif
	    ;

    CHECK_SIZE(sizeof("Distribution: \n") - 1 + strlen(distptr));
    (void) sprintf(&message[strlen(message)], "Distribution: %s\n", distptr);

    *msg = message;
    *msg_size = message_size;
    *msg_total_size = message_total_size;
}

#ifdef GENERATE_EXTRA_FIELDS
/*
  Stuff to generate Message-ID and Date...
  */
static void buildExtraFields _ARGUMENTS((char **, int *, int *));

static void buildExtraFields(msg, msg_size, msg_total_size)
    char **msg;
    int *msg_size, *msg_total_size;
{
    char *message = *msg;
    int message_size = *msg_size;
    int message_total_size = *msg_total_size;

    long clock;
    char *ptr, timeString[34];

    (void) time(&clock);
    (void) strftime(timeString, sizeof(timeString),
		    "%a, %d %b %Y %H:%M:%S %Z", localtime(&clock));
    timeString[sizeof(timeString)-5] = '\0';
    CHECK_SIZE(sizeof("Date: \n") - 1 + strlen(timeString));
    (void) sprintf(&message[strlen(message)], "Date: %s\n", timeString);

    CHECK_SIZE(sizeof("Message-ID: \n") - 1 + strlen(ptr = gen_id()));
    (void) sprintf(&message[strlen(message)], "Message-ID: %s\n", ptr);

    *msg = message;
    *msg_size = message_size;
    *msg_total_size = message_total_size;
}
#else /* ! GENERATE_EXTRA_FIELDS */

#define buildExtraFields(msg, msg_size, msg_total_size)

#endif /* GENERATE_EXTRA_FIELDS */

static void compAbortUtil _ARGUMENTS((int, /* Boolean */ int));

static void compAbortUtil(mesg_name, save)
    int mesg_name;
    Boolean save;
{
    char *ptr;
    char *msg WALL(= 0);

    if (save)
	msg = TextGetString(ComposeText);
    
    destroyCompositionTopLevel();
    freeHeader(&Header);

    switch (PostingMode) {
    case POST:
    case FOLLOWUP:
      ptr = POST_FOLLOWUP_MSG;
	break;
    case FOLLOWUP_REPLY:
    case POST_MAIL:
      ptr = FOLLOWUP_REPLY_MSG;
	break;
    default:
      ptr = DEFAULT_MAIL_MSG;
	break;
    }

    mesgPane(XRN_INFO, mesg_name, MSG_ABORTED_MSG, ptr);
    if (save) {
	mesgPane(XRN_INFO, mesg_name, SAVING_DEAD_MSG,
		 app_resources.deadLetters);
	saveDeadLetter(msg);
	XtFree(msg);
    }
}


/*ARGSUSED*/
void compAbortFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    compAbortUtil(newMesgPaneName(), True);
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
	mesgPane(XRN_SERIOUS, 0, NO_FILE_NOT_SAVED_MSG);
	return;
    }
    
    (void) sprintf(error_buffer, SAVE_IN_MSG, file);
    infoNow(error_buffer);
    
    if ((fp = fopen(file, "a")) != NULL) {
	char *user = getUser();
	int ret;

	/* Provide initial 'From' line (note ctime() provides a newline) */

	(void) time(&clock);
	ret = fprintf(fp, "From %s %s", user, ctime(&clock));
	XtFree(user);
	if (ret == EOF)
	    goto finished;

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


/*ARGSUSED*/
void compSaveFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    String ptr;
    
    ptr = TextGetString(ComposeText);
    saveMessage(app_resources.savePostings, ptr);
    return;
}

static void saveDeadLetter(msg)
    char *msg;
{
    saveMessage(app_resources.deadLetters, msg);
    return;
}

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
	 newsgroup = strtok(0, ",\n")) {
	if (*newsgroup == '\0')
	    continue;
	if (avl_lookup(NewsGroupTable, newsgroup, (char **)&ngptr)) {
	    status |= ngptr->status;
	}
    }

    return(status);
}


#define CLEANUP() { XtFree(buffer); XtFree(buffer2); XtFree(buffer3); }

/*ARGSUSED*/
void compSendFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *buffer = XtMalloc(1), *buffer2 = XtMalloc(1), *buffer3 = XtMalloc(1);
    int buffer_size = 0, buffer_total_size = 1;
    int buffer2_size = 0, buffer2_total_size = 1;
    int buffer3_size = 0, buffer3_total_size = 1;
    char *ErrMessage;
    int mesg_name = newMesgPaneName();

    /*
     * I am loathe to use buffers that are 1024 bytes long for the old
     * from line, new from line and sender line, since they are almost
     * certainly going to be much shorter than this, but this seems to
     * be the way things are done elsewhere in this file, so I'll
     * stick with it.
     */
    char *ptr;
    int mode, i, j, comma;
    unsigned long newsgroups_status WALL(= 0);
    int tries = 1;
    int retry_editing = 0;
    int saved_dead = 0;

    TextDisableRedisplay(ComposeText);

    if ((PostingMode == POST) || (PostingMode == FOLLOWUP) ||
	(PostingMode == FOLLOWUP_REPLY) || (PostingMode == POST_MAIL)) {

	mode = XRN_NEWS;

	if (fieldIsMultiple("Subject:")) {
	    XBell(XtDisplay(TopLevel), 0);
	    mesgPane(XRN_SERIOUS, mesg_name, MULTI_MSG, "Subject");
	    if (app_resources.editorCommand)
		Call_Editor(True);
	    TextEnableRedisplay(ComposeText);
	    CLEANUP(); return;
	}

	if (fieldExists("Subject:", 0) < 0) {
	    if ((PostingMode == POST) || (PostingMode == POST_MAIL)) {
		XBell(XtDisplay(TopLevel), 0);
		addField("Subject: \n");
		mesgPane(XRN_SERIOUS, mesg_name, NO_SUBJECT_MSG);
		mesgPane(XRN_SERIOUS | XRN_SAME_LINE, mesg_name, EMPTY_ADDED_MSG);
		mesgPane(XRN_SERIOUS | XRN_SAME_LINE, mesg_name,
			 FILL_IN_RESEND_MSG);
		if (app_resources.editorCommand)
		    Call_Editor(True);
		TextEnableRedisplay(ComposeText);
		CLEANUP(); return;
	    }
	    buffer_size = 0;
	    *buffer = '\0';
	    buildSubject(&Header, &buffer, &buffer_size, &buffer_total_size);
	    addField(buffer);
	}

 	/* Let's be more strict, since inews doesn't see empty headers.  */
 	returnField("Subject:", &buffer, &buffer_size, &buffer_total_size);
	ptr = buffer + sizeof("Subject:") - 1;
	while ((*ptr == ' ') || (*ptr == '\t') || (*ptr == '\n'))
	    ptr++;
 	if (! *ptr) {
	    XBell(XtDisplay(TopLevel), 0);
	    mesgPane(XRN_SERIOUS, mesg_name, EMPTY_SUBJECT_MSG);
	    mesgPane(XRN_SERIOUS | XRN_SAME_LINE, mesg_name, FILL_IN_RESEND_MSG);
	    if (app_resources.editorCommand)
		Call_Editor(False);
	    TextEnableRedisplay(ComposeText);
	    CLEANUP(); return;
 	}

	if ((PostingMode == FOLLOWUP) || (PostingMode == FOLLOWUP_REPLY)) {
	    if (fieldExists("References:", 0) < 0) {
		buffer_size = 0;
		CHECK_SIZE_EXTENDED(buffer, buffer_size, buffer_total_size,
				    sizeof("References: \n") +
				    strlen(Header.messageId));
		(void) sprintf(buffer, "References: %s\n", Header.messageId);
		addField(buffer);
	    }
	}

	/*
	 * Get rid of any Sender: field currently in the message --
	 * the Sender: field should not ever be inserted by the user.
	 */
	stripField("Sender:");

	if (fieldIsMultiple("From:")) {
	    XBell(XtDisplay(TopLevel), 0);
	    mesgPane(XRN_SERIOUS, mesg_name, MULTI_MSG, "From");
	    if (app_resources.editorCommand)
		Call_Editor(True);
	    TextEnableRedisplay(ComposeText);
	    CLEANUP(); return;
	}

	returnField("From:", &buffer, &buffer_size, &buffer_total_size);

	if (! *buffer) {
	    buildFrom(&Header, &buffer, &buffer_size, &buffer_total_size);
	    addField(buffer);
	}
#ifndef INEWS /* let inews insert its own Sender: field, if necessary */
	else {
	    *buffer2 = '\0';
	    buffer2_size = 0;
	    buildRealFrom(&Header, &buffer2, &buffer2_size, &buffer2_total_size);
	    if (strcmp(buffer, buffer2)) {
		*buffer = '\0';
		buffer_size = 0;
		buildSender(&Header, &buffer, &buffer_size, &buffer2_size);
		addField(buffer);
	    }
	}
#endif /* ! INEWS */	

	if (fieldIsMultiple("Newsgroups:")) {
	    XBell(XtDisplay(TopLevel), 0);
	    mesgPane(XRN_SERIOUS, mesg_name, MULTI_MSG, "Newsgroups");
	    if (app_resources.editorCommand)
		Call_Editor(True);
	    TextEnableRedisplay(ComposeText);
	    CLEANUP(); return;
	}
	    
	if (fieldExists("Newsgroups:", 0) < 0) {
	    Boolean got_groups;

	    buffer_size = 0;
	    *buffer = '\0';
	    got_groups = buildNewsgroups(&Header, &buffer, &buffer_size,
					 &buffer_total_size);
	    addField(buffer);

	    XBell(XtDisplay(TopLevel), 0);

	    mesgPane(XRN_SERIOUS, mesg_name, NO_NEWSGROUPS_MSG);
	    if (got_groups)
		mesgPane(XRN_SERIOUS | XRN_SAME_LINE, mesg_name, DEFAULT_ADDED_MSG);
	    else {
		mesgPane(XRN_SERIOUS | XRN_SAME_LINE, mesg_name, EMPTY_ADDED_MSG);
		mesgPane(XRN_SERIOUS | XRN_SAME_LINE, mesg_name,
			 FILL_IN_RESEND_MSG);
	    }
	    if (app_resources.editorCommand)
		Call_Editor(True);
	    TextEnableRedisplay(ComposeText);
	    CLEANUP(); return;
	}

	returnField("Newsgroups:", &buffer, &buffer_size, &buffer_total_size);

	XtFree(buffer2);
	buffer2 = XtNewString(buffer);
	buffer2_size = strlen(buffer2);
	buffer2_total_size = buffer2_size + 1;

	ptr = buffer;

	/*
	 * fix up the Newsgroups: field - inews can not handle spaces
	 * between group names
	 */
	j = 0;
	comma = 0;
	for (i = 0; i < buffer2_size; i++) {
	    if (comma && (buffer[i] == ' ')) continue;
	    comma = 0;
	    ptr[j++] = buffer[i];
	    if (buffer[i] == ',') {
		comma = 1;
	    }
	}
	ptr[j] = '\0';

	/* Need to preserve buffer because newsgroupsStatusUnion will
	   modify it. */
	XtFree(buffer3);
	buffer3 = XtNewString(buffer);
	buffer3_size = j;
	buffer3_total_size = j + 1;

	for (ptr = buffer + 11 /* skip "Newsgroups:" */; *ptr == ' ';
	     ptr++) /* empty */;
	newsgroups_status = newsgroupsStatusUnion(ptr);

	if (! (newsgroups_status & NG_POSTABLE)) {
	    mesgPane(XRN_ERROR, mesg_name, NO_POSTABLE_NG_MSG);
	    if (app_resources.editorCommand)
		Call_Editor(False);
	    TextEnableRedisplay(ComposeText);
	    CLEANUP(); return;
	}

	if (strcmp(buffer2, buffer3)) {
	    stripField("Newsgroups:");
	    addField(buffer3);
	}

	if (fieldIsMultiple("Path:")) {
	    XBell(XtDisplay(TopLevel), 0);
	    mesgPane(XRN_SERIOUS, mesg_name, MULTI_MSG, "Path");
	    if (app_resources.editorCommand)
		Call_Editor(True);
	    TextEnableRedisplay(ComposeText);
	    CLEANUP(); return;
	}

#ifndef INN
	/* The inews in INN adds the Path: header for us, and has its own
	 * idea of policy.  We don't bother creating it, freeing it, or
	 * anything else in compose.c that has to do with it
	 */
	if (fieldExists("Path:", 0) < 0) {
	    buffer_size = 0;
	    *buffer = '\0';
	    buildPath(&Header, &buffer, &buffer_size, &buffer_total_size);
	    addField(buffer);
	}
#endif /* INN */
	
/*	stripField("Message-ID:");  .............we need this !!! */
	stripField("Relay-Version:");
	stripField("Posting-Version:");

#ifdef IDENTIFY_VERSION_IN_MESSAGES
	/* Tell them who we are */
	stripField("X-newsreader:");
	buffer_size = 0;
	CHECK_SIZE_EXTENDED(buffer, buffer_size, buffer_total_size,
			    sizeof("X-newsreader: xrn \n") +
			    sizeof(XRN_VERSION) - 1); /* - 1 because we don't
							 need both nulls */
	(void) sprintf(buffer, "X-newsreader: xrn %s\n", XRN_VERSION);
	addField(buffer);
#endif
    } else {
	mode = XRN_MAIL;

#ifdef IDENTIFY_VERSION_IN_MESSAGES
	/* Tell them who we are */
	stripField("X-mailer:");
	buffer_size = 0;
	CHECK_SIZE_EXTENDED(buffer, buffer_size, buffer_total_size,
			    sizeof("X-mailer: xrn \n") +
			    sizeof(XRN_VERSION) - 1);
	(void) sprintf(buffer, "X-mailer: xrn %s\n", XRN_VERSION);
	addField(buffer);
#endif
    }

    ptr = TextGetString(ComposeText);

    if ((PostingMode == FOLLOWUP_REPLY) || (PostingMode == POST_MAIL)) {
	tries = 2;
    }

    do {
	switch (postArticle(ptr, mode,&ErrMessage)) {
	case POST_FAILED:
	    XBell(XtDisplay(TopLevel), 0);
	    if (ErrMessage) {
		mesgPane(XRN_SERIOUS, mesg_name, ErrMessage);
		FREE(ErrMessage);
	    }
	    else {
		mesgPane(XRN_SERIOUS, mesg_name,
			 (mode == XRN_NEWS) ? COULDNT_POST_MSG :
			 COULDNT_SEND_MSG);
	    }
	    if (! saved_dead) {
		mesgPane(XRN_SERIOUS, mesg_name, SAVING_DEAD_MSG,
			 app_resources.deadLetters);
		saveDeadLetter(ptr);
		saved_dead++;
	    }
	    retry_editing++;
	    break;
	
	case POST_NOTALLOWED:
	    mesgPane(XRN_SERIOUS, mesg_name, POST_NOTALLOWED_MSG);
	    if (! saved_dead) {
		mesgPane(XRN_SERIOUS, mesg_name, SAVING_DEAD_MSG,
			 app_resources.deadLetters);
		saveDeadLetter(ptr);
		saved_dead++;
	    }
	    break;
	
	case POST_OKAY:
	    mesgPane(XRN_INFO, mesg_name, (mode == XRN_NEWS) ?
		     ((newsgroups_status & NG_MODERATED) ?
		      MAILED_TO_MODERATOR_MSG : ARTICLE_POSTED_MSG) :
		     MAIL_MESSAGE_SENT_MSG);
	    break;
	}
	tries--;
	if ((mode == XRN_NEWS) &&
	    ((PostingMode == FOLLOWUP_REPLY) || (PostingMode == POST_MAIL))) {
	    mode = XRN_MAIL;
	}
    } while (tries > 0);

    destroyCompositionTopLevel();
    freeHeader(&Header);
    if (app_resources.editorCommand) {
	(void) unlink(EditorFileName);
	editing_status = NoEdit;
    }
    FREE(ptr);

    CLEANUP(); return;
}

#undef CLEANUP

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
        (void) sprintf(text, REPLY_YOU_WRITE_MSG,
			 Header.messageId );
     } else if (PostingMode == FORWARD) {
        (void) sprintf(text, FORWARDED_ARTIKEL_MSG,
			 Header.messageId, Header.from);
     } else {
        (void) sprintf(text, FOLLOWUP_AUTHOR_WRITE_MSG,
			 Header.messageId, Header.from);
     }

     cur_size = strlen(text);

     if (app_resources.includeCommand && PostingMode != FORWARD) {
	  char cmdbuf[1024];

	  sprintf(cmdbuf, app_resources.includeCommand,
		  app_resources.includePrefix, Header.artFile);
	  infile = xrn_popen(cmdbuf, "r");
	  if (! infile) {
	       mesgPane(XRN_SERIOUS, 0, CANT_INCLUDE_CMD_MSG);
	       FREE(text);
	       return(0);
	  }

	  prefix = "";
	  prefix_size = 0;
     }
     else {
	  infile = fopen(Header.artFile, "r");
	  if (! infile) {
	      mesgPane(XRN_SERIOUS, 0, CANT_OPEN_ART_MSG, errmsg(errno));
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
	 while (fgets(input, sizeof(input), infile)) {
	     if (! index(input, '\n'))
		 continue;
	     if (*input == '\n')
		 break;
	 }
     }

     if (!feof(infile)) {
	 Boolean do_prefix = True;

	 while (fgets(input, sizeof(input), infile)) {
	     int line_size = strlen(input);
	     if ((do_prefix ? prefix_size : 0) + line_size >
		 size - cur_size - 1) {
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
	     if (do_prefix) {
		 (void) strcpy(&text[cur_size], prefix);
		 cur_size += prefix_size;
	     }
	     (void) strcpy(&text[cur_size], input);
	     cur_size += line_size;
	     if (text[cur_size-1] == '\n')
		 do_prefix = True;
	     else
		 do_prefix = False;
	 }
     }

     if (PostingMode == FORWARD) {
	 int line_size;
       (void) sprintf(input, FORWARDED_ARTICLE_END_MSG);
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
	  (void) xrn_pclose(infile);
     else
	  (void) fclose(infile);

     return(text);
}


     
static void includeArticleText()
{
    long point;
    char *message_text;

    point = TextGetInsertionPoint(ComposeText);

    message_text = getIncludedArticleText();

    if (message_text) {
	TextReplace(ComposeText, message_text, strlen(message_text),
		    point, point);
	FREE(message_text);
    }

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


/*ARGSUSED*/
void compIncludeArticleFunction(widget, event, string, count)
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
    long point;
    char *file;
    FILE *filefp;
    char *message;
    struct stat sb;
    int num_read, total_read = 0;

    if ((int) client_data == XRNinclude_ABORT) {
	/* empty */
    }
    else if (! (file = GetDialogValue(IncludeBox))) {
	mesgPane(XRN_SERIOUS, 0, NO_FILE_SPECIFIED_MSG);
    }
    else if (! (filefp = fopen(utTildeExpand(file), "r"))) {
	mesgPane(XRN_SERIOUS, 0, CANT_OPEN_FILE_MSG, file, errmsg(errno));
    }
    else if (fstat(fileno(filefp), &sb) == -1) {
	mesgPane(XRN_SERIOUS, 0, "%s: %s", file, errmsg(errno));
	(void) fclose(filefp);
    }
    else {
	message = XtMalloc(sb.st_size);
	while ((num_read = fread(message + total_read, sizeof(*message),
				 sb.st_size - total_read,
				 filefp)) > 0)
	    total_read += num_read;
	(void) fclose(filefp);
	point = TextGetInsertionPoint(ComposeText);
	TextReplace(ComposeText, message, total_read, point, point);
	XtFree(message);
	XtFree(IncludeString);
	IncludeString = XtNewString(file);
    }

    PopDownDialog(IncludeBox);
    IncludeBox = 0;

    return;
}    


/*ARGSUSED*/
void compIncludeFileFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
      {ABORT_STRING, includeHandler, (XtPointer) XRNinclude_ABORT},
      {DOIT_STRING,  includeHandler, (XtPointer) XRNinclude_DOIT},
    };

    if (IncludeBox == (Widget) 0) {
      IncludeBox = CreateDialog(ComposeTopLevel, ASK_FILE_MSG,
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
    int mesg_name = newMesgPaneName();

    read(inchannel,buffer,1);
    if(editing_status != Completed || EditorFileName == NIL(char)){
	return;
    }

    switch (PostingMode) {
    case POST:
    case FOLLOWUP:
      msg_type = POST_FOLLOWUP_MSG;
      confirm1 = ASK_POST_ARTICLE_MSG ;
      confirm2 = ASK_RE_EDIT_ARTCILE_MSG ;
	break;
    case FOLLOWUP_REPLY:
    case POST_MAIL:
      msg_type = FOLLOWUP_REPLY_MSG;
      confirm1 = ASK_POST_SEND_MSG;  /* ... and/or ... */
      confirm2 = ASK_RE_EDIT_MSG;
	break;
    default:
      msg_type= DEFAULT_MAIL_MSG;
      confirm1 = ASK_POST_SEND_MSG; /* ... and/or ... */
      confirm2 = ASK_RE_EDIT_MSG;
	break;
    }

    if ((filefp = fopen(EditorFileName, "r")) == NULL) {
	mesgPane(XRN_SERIOUS, mesg_name, CANT_OPEN_TEMP_MSG, EditorFileName,
		 errmsg(errno));
	editing_status = NoEdit;
	compAbortUtil(mesg_name, False);
	return;
    }

    if (fstat(fileno(filefp), &buf) == -1) {
	mesgPane(XRN_SERIOUS, mesg_name, CANT_STAT_TEMP_MSG, EditorFileName,
		 errmsg(errno));
	(void) fclose(filefp);
	(void) unlink(EditorFileName);
	editing_status = NoEdit;
	compAbortUtil(mesg_name, False);
	return;
    }

    if (originalBuf.st_mtime == buf.st_mtime) {
	mesgPane(XRN_INFO, mesg_name, NO_CHANGE_MSG, EditorFileName);
	(void) fclose(filefp);
	(void) unlink(EditorFileName);
	editing_status = NoEdit;
	compAbortUtil(mesg_name, False);
	return;
    }

    if (buf.st_size == 0) {
	mesgPane(XRN_INFO, mesg_name, ZERO_SIZE_MSG, EditorFileName);
	(void) fclose(filefp);
	(void) unlink(EditorFileName);
	editing_status = NoEdit;
	compAbortUtil(mesg_name, False);
	return;
    }

    ptr = XtMalloc(buf.st_size + 1);
    (void) fread(ptr, sizeof(char), buf.st_size, filefp);
    ptr[buf.st_size] = '\0';
    (void) fclose(filefp);

    /* pop up a confirm box */

    if (ConfirmationBox(TopLevel, confirm1, 0, 0) == XRN_CB_ABORT) {
	if (ConfirmationBox(TopLevel, confirm2, 0, 0) == XRN_CB_ABORT) {
	    compAbortUtil(mesg_name, True);
	    (void) unlink(EditorFileName);
	    editing_status = NoEdit;
	} else {
	    Call_Editor(False);
	}
	FREE(ptr);
	return;
    }

    TextSetString(ComposeText, ptr);
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
Call_Editor(save_message)
    Boolean save_message;
{
    char *dsp, *file;
    char buffer[1024], buffer2[1024];
    FILE *filefp;
    char *header = 0;
    int mesg_name = newMesgPaneName();

    if (save_message) {
	header = TextGetString(ComposeText);
    }
    
    if ((editing_status == NoEdit) || header) {
	editing_status = InProgress;
	if(EditorFileName != NIL(char))
	    FREE(EditorFileName);
	EditorFileName = utTempnam(app_resources.tmpDir, "xrn");
	EditorFileName = XtNewString(EditorFileName);
	if ((filefp = fopen(EditorFileName, "w")) == NULL) {
	    mesgPane(XRN_SERIOUS, mesg_name, CANT_OPEN_TEMP_MSG, EditorFileName,
		     errmsg(errno));
	    editing_status = NoEdit;
	    compAbortUtil(mesg_name, False);
	    return (-1);
	}
	do_chmod(filefp, EditorFileName, 0600);
	if (header) {
	    (void) fwrite(header, sizeof(char), utStrlen(header), filefp);
	}
	else {
	    mesgPane(XRN_SERIOUS, mesg_name, NO_MSG_TEMPLATE_MSG);
	    editing_status = NoEdit;
	    compAbortUtil(mesg_name, False);
	    return (-1);
	}

	(void) fclose(filefp);

	if (stat(EditorFileName, &originalBuf) == -1) {
	    mesgPane(XRN_SERIOUS, mesg_name, CANT_STAT_TEMP_MSG, EditorFileName,
		     errmsg(errno));
	    editing_status = NoEdit;
	    compAbortUtil(mesg_name, False);
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

#ifdef VFORK_SUPPORTED
    if ((forkpid = vfork()) == 0) 
#else /* ! VFORK_SUPPORTED */
    if ((forkpid = fork()) == 0) 
#endif /* VFORK_SUPPORTED */
       {
	    int i;
	    int maxdesc;

#ifdef __hpux
	    maxdesc = (int) sysconf (_SC_OPEN_MAX);
#else /* ! __hpux */
# ifdef SVR4
# include <ulimit.h>
	    maxdesc = ulimit(UL_GDESLIM);
# else /* ! SVR4 */
	    maxdesc = getdtablesize();
# endif /* SVR4 */
#endif /* __hpux */
	    for (i = 3; i < maxdesc; i++) {
		(void) close(i);
	    }
	    (void) execl("/bin/sh", "sh", "-c", buffer, 0);
          (void) fprintf(stderr, ERROR_EXEC_FAILED_MSG, buffer);
	    (void) _exit(127);
	}
	if (forkpid < 0) {
	    mesgPane(XRN_SERIOUS, mesg_name, CANT_EDITOR_CMD_MSG, buffer,
		     errmsg(errno));
	    editing_status = NoEdit;
	    compAbortUtil(mesg_name, False);
	    return (-1);
	}
	else {
	    signal(SIGCHLD, (SIG_PF0) catch_sigchld);
	}

   return (0);
}

#define CHECK_COMPOSE_PANE() \
    if (ComposeTopLevel) { \
	mesgPane(XRN_SERIOUS, 0, ONE_COMPOSITION_ONLY_MSG); \
	return; \
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
static int composePane _ARGUMENTS((char *, char *, long));

static int composePane(titleString, header, point)
    char *titleString;
    char *header;
    long point;
{
    char *signature;
    Widget pane, buttonBox, label;
    Dimension height_val;
    static char titleStorage[LABEL_SIZE];
    Dimension width_val;
    static Arg labelArgs[] = {
	{XtNlabel, (XtArgVal) titleStorage},
	{XtNskipAdjust, (XtArgVal) True},
    };
    static Arg shellArgs[] = {
	{XtNinput, (XtArgVal) True},
	{XtNsaveUnder, (XtArgVal) False},
    };

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
    XtVaGetValues(TopLevel,
		  XtNwidth, (XtPointer) &width_val,
		  XtNheight, (XtPointer) &height_val,
		  (char *) 0);

    pane = XtVaCreateManagedWidget("pane", panedWidgetClass, ComposeTopLevel,
				   XtNwidth, (XtArgVal) width_val,
				   XtNheight, (XtArgVal) height_val,
				   (char *) 0);

    (void) strcpy(titleStorage, titleString);

    label = XtCreateManagedWidget("label", labelWidgetClass, pane,
				  labelArgs, XtNumber(labelArgs));

    ComposeText = TextCreate("text", False, pane);
    TextSetString(ComposeText, header);
    FREE(header);
 
    buttonBox = ButtonBoxCreate("box", pane);

    (void) ButtonBoxAddButton("compAbort", compAbortCallbacks, buttonBox);
    (void) ButtonBoxAddButton("compSend", compSendCallbacks, buttonBox);
    (void) ButtonBoxAddButton("compSave", compSaveCallbacks, buttonBox);

    if ((PostingMode != POST) && 
	(PostingMode != GRIPE) &&
	(PostingMode != FORWARD)) {
	(void) ButtonBoxAddButton("compIncludeArticle",
				  compIncludeArticleCallbacks, buttonBox);
    }
    (void) ButtonBoxAddButton("compIncludeFile", compIncludeFileCallbacks,
			      buttonBox);

    if (app_resources.editorCommand != NIL(char)) {
	return(Call_Editor(True));
    }
    else {
	XtRealizeWidget(ComposeTopLevel);
	/*
	  Needs to happen after widget is realized, so that the width
	  of the button box is correct.
	  */
	ButtonBoxDoneAdding(buttonBox);

	XtSetKeyboardFocus(ComposeTopLevel, ComposeText);

	XtVaGetValues(label,
		      XtNheight, (XtPointer) &height_val,
		      (char *) 0);

	XawPanedSetMinMax(label, (int) height_val, (int) height_val);
	XawPanedAllowResize(ComposeText, True);
    
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

	TextSetInsertionPoint(ComposeText, point);

	XtPopup(ComposeTopLevel, XtGrabNone);

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
    struct newsgroup *newsgroup = CurrentGroup;
    char *psigfile = 0;
    char *ptr;

#if defined(INEWS_READS_SIG)
    /* these posting modes do not go through INEWS, so include the signature */
    if ((PostingMode != REPLY) && (PostingMode != GRIPE) &&
	(PostingMode != MAIL)) {
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
    /* find an appropriate sig */

    if(!newsgroup) {
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

    if (! psigfile) {
	/* signature according to posting mode. */
	(void) strcpy(sigfile, file);
	(void) strcat(sigfile, ".");
	(void) strcat(sigfile, PostingModeStrings[PostingMode]);
	if (access(sigfile, F_OK)) {
	    (void) strcpy(sigfile, file);
	    if (access(sigfile, F_OK)) {
		return 0;
	    }
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
		mesgPane(XRN_INFO, 0, EXECUTING_SIGNATURE_MSG, sigfile);
	    }
	    (void) sprintf(cmdbuf, "%s %s %s %s",
			    sigfile,
			    (newsgroup ? newsgroup->name : "NIL"),
			    PostingModeStrings[PostingMode],
			    (Header.artFile ? Header.artFile : "NIL"));
	    infofp = xrn_popen(cmdbuf, "r");
	    close_func = xrn_pclose;
	    if (!infofp) {
		mesgPane(XRN_SERIOUS, 0, CANT_EXECUTE_SIGNATURE_MSG, sigfile);
	    }
	} else {
	    if (app_resources.signatureNotify) {
		mesgPane(XRN_INFO, 0, READING_SIGNATURE_MSG, sigfile);
	    }
	}

	if (!infofp) {
	    infofp = fopen(sigfile, "r");
	    close_func = fclose;
	    if (! infofp) {
		mesgPane(XRN_SERIOUS, 0, CANT_READ_SIGNATURE_MSG, sigfile,
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
	mesgPane(XRN_SERIOUS, 0, SIGNATURE_TOO_BIG_MSG, sigfile);
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
    struct article *art;

    CHECK_COMPOSE_PANE();
    
    if (getHeader(current, &Header) != XRN_OKAY)
	return;

    art = artStructGet(newsgroup, current, False);
    Header.artFile = XtNewString(art->filename);

    if (followup) {
	if (! strcmp(Header.followupTo, "poster")) {
	    char *msg, *p1, *p2;
	    int ret;

	    if (reply) {
              msg = ASK_POSTER_FANDR_MSG; 
              p1 = POST_AND_SEND_MSG;
              p2 = SEND_MAIL_MSG;
	    }
	    else {
              msg = ASK_POSTER_REPLY_MSG;
              p1 = POST_MSG;
              p2 = SEND_MAIL_MSG;
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
          (void) sprintf(title, FOLLOWUP_REPLY_TO_TITLE_MSG,
                           current, newsgroup->name);
	}
	else {
	    PostingMode = FOLLOWUP;
          (void) sprintf(title, FOLLOWUP_TO_TITLE_MSG,
                           current, newsgroup->name);
	}
    }
    else {
	PostingMode = REPLY;
          (void) sprintf(title, REPLY_TO_TITLE_MSG,
                           current, newsgroup->name);
    }
    
    CHECK_SIZE(1);
    message[0] = '\0';
    message_size = 0;
    
    buildFrom(&Header, &message, &message_size, &message_total_size);
    buildReplyTo(&message, &message_size, &message_total_size);
    buildSubject(&Header, &message, &message_size, &message_total_size);

    if (followup) {
	buildPath(&Header, &message, &message_size, &message_total_size);
	(void) buildNewsgroups(&Header, &message, &message_size,
			       &message_total_size);
	buildDistribution(&Header, &message, &message_size,
			  &message_total_size);

	CHECK_SIZE(sizeof("Followup-To: \n") - 1);
	(void) strcat(message, "Followup-To: \n");

	buildReferences(&Header, &message, &message_size, &message_total_size);
	
	buildExtraFields(&message, &message_size, &message_total_size);
	
	CHECK_SIZE(sizeof("Organization: \n") - 1 + strlen(Header.organization));
	(void) sprintf(&message[strlen(message)], "Organization: %s\n",
		       Header.organization);

	CHECK_SIZE(sizeof("Keywords: \n") - 1);
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
	CHECK_SIZE(sizeof("To: \n") - 1 + strlen(reply_addr));
	(void) sprintf(&message[strlen(message)], "To: %s\n", reply_addr);

        if (app_resources.cc == True) {
	    CHECK_SIZE(sizeof("Cc: \n") - 1 + strlen(Header.user));
	    sprintf(&message[strlen(message)], "Cc: %s\n", Header.user);
	}

	if (! followup) {
	    CHECK_SIZE(sizeof("X-Newsgroups: \nIn-reply-to: %s\n") - 1 +
		       strlen(Header.newsgroups) + strlen(Header.messageId));
	    (void) sprintf(&message[strlen(message)],
			   "X-Newsgroups: %s\nIn-reply-to: %s\n",
			   Header.newsgroups, Header.messageId);
	}
    }
	
    CHECK_SIZE(1);
    (void) strcat(message, "\n");

    if (composePane(title, message, strlen(message))) {
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
    long point;

    CHECK_COMPOSE_PANE();
    
    (void) strcpy(title, "Gripe");

    if (getHeader(0, &Header) != XRN_OKAY)
	return;

    CHECK_SIZE(1);
    *message = '\0';
    message_size = 0;

    buildFrom(&Header, &message, &message_size, &message_total_size);
    buildReplyTo(&message, &message_size, &message_total_size);

    CHECK_SIZE(sizeof("To: \nSubject: GRIPE about XRN \n") - 1 +
	       sizeof(GRIPES) - 1 + sizeof(XRN_VERSION) - 1);
    (void) sprintf(&message[strlen(message)],
		   "To: %s\nSubject: GRIPE about XRN %s\n",
		   GRIPES, XRN_VERSION);

    CHECK_SIZE(sizeof(bugTemplate) - 1 + 2);
    (void) strcat(message, "\n");

    point = strlen(message) + (index(bugTemplate, '[') - bugTemplate);

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
    long point;
    struct article *art;

    CHECK_COMPOSE_PANE();
    
    if (getHeader(current, &Header) != XRN_OKAY)
	return;

    art = artStructGet(newsgroup, current, False);
    Header.artFile = XtNewString(art->filename);

    PostingMode = FORWARD;
    
    (void) sprintf(title, FORWARD_TO_TITLE_MSG, current,
		   newsgroup->name);

    CHECK_SIZE(1);
    *message = '\0';
    message_size = 0;

    buildFrom(&Header, &message, &message_size, &message_total_size);
    buildReplyTo(&message, &message_size, &message_total_size);

    CHECK_SIZE(sizeof("To: \n") - 1);
    (void) strcat(message, "To: \n");

    point = strlen(message) - 1;

    CHECK_SIZE(sizeof("Subject:   [ #]\n") - 1 +
	       strlen(Header.subject) + strlen(newsgroup->name) +
	       (int) (current / 10) + 1);
    (void) sprintf(&message[strlen(message)],
		   "Subject: %s  [%s #%ld]\n", Header.subject,
		   newsgroup->name, current);

    if (app_resources.ccForward == True) {
	CHECK_SIZE(sizeof("Cc: \n") - 1 + strlen(Header.user));
	(void) sprintf(&message[strlen(message)], "Cc: %s\n", Header.user);
    }

    CHECK_SIZE(1);
    (void) strcat(message, "\n");

    if (composePane(title, message, point))
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
		    Header.real_host);
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

void post_or_mail(ingroupp, post, mail)
    Boolean ingroupp, post, mail;
{
    struct newsgroup *newsgroup = CurrentGroup;
    char title[LABEL_SIZE];
    char *message = 0;
    int message_size = 0, message_total_size = 0;
    int OldPostingMode = PostingMode;
    long point = 0;
    Boolean got_groups;

    CHECK_COMPOSE_PANE();
    
    if (getHeader((art_num) 0, &Header) != XRN_OKAY)
	return;

    if (!ingroupp || !post || (! newsgroup)) {
	FREE(Header.newsgroups);
	Header.newsgroups = XtNewString("");
    }

    if (post)
	if (mail) {
	    PostingMode = POST_MAIL;
	    if (*Header.newsgroups)
		(void) sprintf(title, POST_MAIL_ARTICLE_TO_MSG,
			       Header.newsgroups);
	    else
		(void) strcpy(title, POST_MAIL_ARTICLE_MSG);
	}
	else {
	    PostingMode = POST;
	    if (*Header.newsgroups)
		(void) sprintf(title, POST_ARTICLE_TO_MSG,
			       Header.newsgroups);
	    else
		(void) strcpy(title, POST_ARTICLE_MSG);
	}
    else {
	PostingMode = MAIL;
	(void) strcpy(title, MAIL_MSG);
    }

    CHECK_SIZE(1);
    *message = '\0';
    message_size = 0;

    buildFrom(&Header, &message, &message_size, &message_total_size);
    buildReplyTo(&message, &message_size, &message_total_size);

    if (post) {
	buildPath(&Header, &message, &message_size, &message_total_size);

	got_groups = buildNewsgroups(&Header, &message, &message_size,
				     &message_total_size);

	if (! got_groups)
	    point = strlen(message) - 1;
    }

    if (mail) {
	CHECK_SIZE(sizeof("To: \n") - 1);
	(void) strcat(message, "To: \n");
	if (! point)
	    point = strlen(message) - 1;

        if (app_resources.cc == True) {
	    CHECK_SIZE(sizeof("Cc: \n") - 1 + strlen(Header.user));
	    sprintf(&message[strlen(message)], "Cc: %s\n", Header.user);
	}
    }

    CHECK_SIZE(sizeof("Subject: \n") - 1);
    (void) strcat(message, "Subject: \n");
    if (! point)
	 point = strlen(message) - 1;

    if (post) {
	buildDistribution(&Header, &message, &message_size, &message_total_size);

	CHECK_SIZE(sizeof("Followup-To: \n") - 1);
	(void) strcat(message, "Followup-To: \n");

        CHECK_SIZE(sizeof("Organization: \n") - 1 + strlen(Header.organization));
	(void) sprintf(&message[strlen(message)], "Organization: %s\n",
		       Header.organization);

	CHECK_SIZE(sizeof("Keywords: \n") - 1);
	(void) strcat(message, "Keywords: \n");
    }

    buildExtraFields(&message, &message_size, &message_total_size);

    CHECK_SIZE(1);
    (void) strcat(message, "\n");

    if (composePane(title, message, point))
	 PostingMode = OldPostingMode;
    
    return;
}


void post(ingroupp)
    Boolean ingroupp;
{
    post_or_mail(ingroupp, True, False);
}

void mail()
{
    post_or_mail(False, False, True);
}

void post_and_mail(ingroupp)
    Boolean ingroupp;
{
    post_or_mail(ingroupp, True, True);
}


static Boolean _canCancelArticle()
{
    char buffer[BUFFER_SIZE], *bufptr;

    /* verify that the user can cancel the article */
    if ((bufptr = index(CancelHeader.from, '@'))) {
	bufptr++;
	(void) strcpy(buffer, bufptr);
	if ((bufptr = index(buffer, ' ')))
	    *bufptr = '\0';
	if (strncmp(CancelHeader.host, buffer, utStrlen(CancelHeader.host))
	   || (strncmp(CancelHeader.user, CancelHeader.from,
		       utStrlen(CancelHeader.user)) 
	      && strcmp(CancelHeader.user, "root")))
	    return False;
    }

    return True;
}


Boolean canCancelArticle()
{
    struct newsgroup *newsgroup = CurrentGroup;
    art_num current = newsgroup->current;
    Boolean ret;

    if (getHeader(current, &CancelHeader) != XRN_OKAY)
	return True;
    ret = _canCancelArticle();
    freeHeader(&CancelHeader);
    return ret;
}

    
void cancelArticle()
{
    struct newsgroup *newsgroup = CurrentGroup;
    art_num current = newsgroup->current;
    char *message = 0;
    int message_size = 0, message_total_size = 0;
    char *ErrMessage;
    int mesg_name = newMesgPaneName();

    if (getHeader(current, &CancelHeader) != XRN_OKAY)
	return;

    /* verify that the user can cancel the article */
    if (! _canCancelArticle()) {
	freeHeader(&CancelHeader);
	return;
    }

    CHECK_SIZE(1);
    *message = '\0';
    message_size = 0;

    buildFrom(&CancelHeader, &message, &message_size, &message_total_size);
    buildReplyTo(&message, &message_size, &message_total_size);
    buildPath(&CancelHeader, &message, &message_size, &message_total_size);

    CHECK_SIZE(sizeof("Subject: cancel \n") - 1 + strlen(CancelHeader.messageId));
    (void) sprintf(&message[strlen(message)], "Subject: cancel %s\n",
		   CancelHeader.messageId);

    CHECK_SIZE(sizeof("Newsgroups: ,control\n") - 1 +
	       strlen(CancelHeader.newsgroups));
    (void) sprintf(&message[strlen(message)], "Newsgroups: %s,control\n",
		   CancelHeader.newsgroups);

    buildReferences(&CancelHeader, &message, &message_size, &message_total_size);

    if (CancelHeader.distribution && *CancelHeader.distribution) {
	CHECK_SIZE(strlen ("Distribution: \n") + strlen(CancelHeader.distribution));
	(void) sprintf(&message[strlen(message)], "Distribution: %s\n",
		       CancelHeader.distribution);
    }

    CHECK_SIZE(sizeof("Control: cancel \n") - 1 + strlen(CancelHeader.messageId));
    (void) sprintf(&message[strlen(message)], "Control: cancel %s\n",
		   CancelHeader.messageId);

    freeHeader(&CancelHeader);

    switch (postArticle(message, XRN_NEWS,&ErrMessage)) {
    case POST_FAILED:
	if (ErrMessage) {
	    mesgPane(XRN_SERIOUS, mesg_name, ErrMessage);
	    mesgPane(XRN_SERIOUS | XRN_SAME_LINE, mesg_name, "  ");
	    mesgPane(XRN_SERIOUS | XRN_SAME_LINE, mesg_name, CANCEL_ABORTED_MSG);
	    FREE(ErrMessage);
	}
	else {
	    mesgPane(XRN_SERIOUS, mesg_name, CANCEL_ABORTED_MSG);
	}
	break;

    case POST_NOTALLOWED:
	mesgPane(XRN_SERIOUS, mesg_name, POST_NOTALLOWED_MSG);
	mesgPane(XRN_SERIOUS | XRN_SAME_LINE, mesg_name, CANCEL_ABORTED_MSG);
	break;
	    
    case POST_OKAY:
	mesgPane(XRN_INFO, mesg_name, CANCELLED_ART_MSG);
	break;
    }

    FREE(message);

    return;

}
