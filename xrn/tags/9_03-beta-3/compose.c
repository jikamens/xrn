#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: compose.c,v 1.130 2000-10-05 04:07:03 jik Exp $";
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

#ifdef INN
#include <stdio.h>
#include <configdata.h>
#include <clibrary.h>
#include <libinn.h>
#endif

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <ctype.h>
#include <errno.h>
#include <sys/file.h>
#include <signal.h>
#include <assert.h>
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
#include "file_cache.h"

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
#include "patchlevel.h"
#include "xmisc.h"
#include "buttons.h"
#include "butdefs.h"
#include "mesg_strings.h"

struct header {
    struct newsgroup *newsgroup;
    art_num article;
    file_cache_file artFile;
    char *newsgroups;
    char *subject;
    char *id;
    char *followupTo;
    char *references;
    char *from;
    char *sender;
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
static void saveDeadLetter _ARGUMENTS((char *));
static int trim_references _ARGUMENTS((char *));
static void switch_message_type _ARGUMENTS((struct header *));
static char *update_headers _ARGUMENTS((struct header *, int, int, int));
static char *followup_or_reply_title _ARGUMENTS((struct header *, int, int));
static void compose_buttons _ARGUMENTS((Widget, int));
static int check_quoted_text _ARGUMENTS((char *));
static char *insert_courtesy_tag _ARGUMENTS((char *));
static Boolean IsValidAddressField _ARGUMENTS((CONST char *));

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
static Widget ComposeLabel = (Widget) 0;
static Widget ComposeText = (Widget) 0;
static Widget ComposeButtonBox = (Widget) 0;

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

static int Call_Editor _ARGUMENTS((Boolean save_message, Boolean preserve_mtime));

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
#define IS_RESPONSE(mode) ((mode == FOLLOWUP) || (mode == REPLY) || \
			   (mode == FOLLOWUP_REPLY))

/*
 * storage for the header information needed for building, checking,
 * and repairing the header.  Once this is created, the user can go
 * to another article or another group and still use the composition
 * window
 */

static struct header Header, CancelHeader;

#define SIG_PREFIX "-- \n"

BUTTON(compAbort,abort);
BUTTON(compSave,save);
BUTTON(compSwitchFollowup,switch to followup);
BUTTON(compSwitchReply,switch to reply);
BUTTON(compSwitchBoth,switch to followup/reply);
BUTTON(compSend,send);
BUTTON(compIncludeArticle,include article);
BUTTON(compIncludeFile,include file);

static ButtonList CompButtonList[] = {
  {"compAbort",			compAbortCallbacks,		0,	True},
  {"compSwitchFollowup",	compSwitchFollowupCallbacks,	0,	True},
  {"compSwitchReply",		compSwitchReplyCallbacks,	0,	True},
  {"compSwitchBoth",		compSwitchBothCallbacks,	0,	True},
  {"compSend",			compSendCallbacks,		0,	True},
  {"compSave",			compSaveCallbacks,		0,	True},
  {"compIncludeArticle",	compIncludeArticleCallbacks,	0,	True},
  {"compIncludeFile",		compIncludeFileCallbacks,	0,	True},
};

static int CompButtonListCount = XtNumber(CompButtonList);

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
  * Contents of the hiddenHost X resource, if it's set, or
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
  * Contents of the "domainName" X resource, if it's set, or
  * Contents of the DOMAIN environment variable, if it's set, or
  * INN "domain" configuration value, if using INN and it's set, or
  * Contents of the file DOMAIN_FILE, if it's defined and non-empty, or
  * The string DOMAIN_NAME, if it's defined, or
  * Error signalled and post/mail aborted

  Here's how they're used:

  real_host:
  * Message-ID: field (if GENERATE_EXTRA_FIELDS is defined),
  * Verification that the user is allowed to cancel an article

  sender_host:
  * Sender: field, if necessary
  
  host:
  * From: field

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

  Header->newsgroup = newsgroup;

  if (article > 0) {
#define CHECK(field,name) if (art->field) Header->field = XtNewString(art->field); else xhdr(newsgroup, article, name, &Header->field);

    struct article *art = artStructGet(newsgroup, article, False);

    Header->article = article;
    CHECK(newsgroups, "newsgroups");
    CHECK(subject, "subject");
    CHECK(references, "references");
    CHECK(from, "from");
    CHECK(id, "message-id");
    xhdr(newsgroup, article, "followup-to", &Header->followupTo);
    xhdr(newsgroup, article, "sender", &Header->sender);
    xhdr(newsgroup, article, "reply-to", &Header->replyTo);
    xhdr(newsgroup, article, "distribution", &Header->distribution);
    xhdr(newsgroup, article, "keywords", &Header->keywords);

#undef CHECK
  } else
    /* information for 'post' */
    if (newsgroup)
      Header->newsgroups = XtNewString(newsgroup->name);
    else
      Header->newsgroups = XtNewString("");

  /*
   * since I'm lazy down below, I'm going to replace NIL pointers with ""
   */
  if (Header->newsgroups == NIL(char))
    Header->newsgroups = XtNewString("");
  if (Header->subject == NIL(char))
    Header->subject = XtNewString("");
  if (Header->id == NIL(char))
    Header->id = XtNewString("");
  if (Header->followupTo == NIL(char))
    Header->followupTo = XtNewString("");
  if (Header->references == NIL(char))
    Header->references = XtNewString("");
  if (Header->from == NIL(char))
    Header->from = XtNewString("");
  if (Header->sender == NIL(char))
    Header->sender = XtNewString("");
  if (Header->replyTo == NIL(char))
    Header->replyTo = XtNewString("");
  if (Header->distribution == NIL(char))
    Header->distribution = XtNewString("");
  if (Header->keywords == NIL(char))
    Header->keywords = XtNewString("");

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

  if ((! host[0]) && app_resources.hiddenHost && *app_resources.hiddenHost)
    (void) strncpy(host, app_resources.hiddenHost, sizeof(host)-1);

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
    file_cache_file_release(FileCache, Header->artFile);
    FREE(Header->newsgroups);
    FREE(Header->subject);
    FREE(Header->id);
    FREE(Header->followupTo);
    FREE(Header->references);
    FREE(Header->from);
    FREE(Header->sender);
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

static void _buildFrom _ARGUMENTS((struct header *, char **, Boolean,
				   Boolean, int *, int *));

static void _buildFrom(
		       _ANSIDECL(struct header *,	Header),
		       _ANSIDECL(char **,		msg),
		       _ANSIDECL(Boolean,		real_addr),
		       _ANSIDECL(Boolean,		sender),
		       _ANSIDECL(int *,			msg_size),
		       _ANSIDECL(int *,			msg_total_size)
		       )
     _KNRDECL(struct header *,	Header)
     _KNRDECL(char **,		msg)
     _KNRDECL(Boolean,		real_addr)
     _KNRDECL(Boolean,		sender)
     _KNRDECL(int *,		msg_size)
     _KNRDECL(int *,		msg_total_size)
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
    int msg_len, ref_len, new_ref_len;

    CHECK_SIZE(sizeof("References:  \n") - 1 + strlen(Header->references) +
	       strlen(Header->id));
    (void) sprintf(&message[(msg_len = strlen(message))], "References: %s %s\n",
		   Header->references, Header->id);

    ref_len = strlen(&message[msg_len]);
    new_ref_len = trim_references(&message[msg_len]);
    message_size -= (ref_len - new_ref_len);

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

static int buildNewsgroups _ARGUMENTS((struct header *, char **, int *, int *));

static int buildNewsgroups(Header, msg, msg_size, msg_total_size)
    struct header *Header;
    char **msg;
    int *msg_size, *msg_total_size;
{
    char *message = *msg;
    int message_size = *msg_size;
    int message_total_size = *msg_total_size;

    char *ngptr;
    int ng_count = 0;

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

    if (*ngptr) {
      for (message = ngptr; message; message = strchr(message + 1, ','))
	ng_count++;
    }
	
    return ng_count;
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

static void compAbortUtil _ARGUMENTS((int, Boolean));

static void compAbortUtil(
			  _ANSIDECL(int,	mesg_name),
			  _ANSIDECL(Boolean,	save)
			  )
     _KNRDECL(int,	mesg_name)
     _KNRDECL(Boolean,	save)
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
#if defined(__osf__) || defined(__hpux)
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
    char *ptr, *ptr2;
    int mode, i, j, comma;
    unsigned long newsgroups_status WALL(= 0);
    int tries = 1, saved = 0;
    int saved_dead = 0;

    TextDisableRedisplay(ComposeText);

    if ((PostingMode == POST) || (PostingMode == FOLLOWUP) ||
	(PostingMode == FOLLOWUP_REPLY) || (PostingMode == POST_MAIL)) {

	mode = XRN_NEWS;

	if (fieldIsMultiple("Subject:")) {
	    XBell(XtDisplay(TopLevel), 0);
	    mesgPane(XRN_SERIOUS, mesg_name, MULTI_MSG, "Subject");
	    if (app_resources.editorCommand)
		Call_Editor(True, False);
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
		    Call_Editor(True, False);
		TextEnableRedisplay(ComposeText);
		CLEANUP(); return;
	    }
	    buffer_size = 0;
	    *buffer = '\0';
	    buildSubject(&Header, &buffer, &buffer_size, &buffer_total_size);
	    addField(buffer);
	}

 	/* Let's be more strict, since inews doesn't see empty headers.  */
	buffer_size = 0;
 	returnField("Subject:", &buffer, &buffer_size, &buffer_total_size);
	ptr = buffer + sizeof("Subject:") - 1;
	while ((*ptr == ' ') || (*ptr == '\t') || (*ptr == '\n'))
	    ptr++;
 	if (! *ptr) {
	    XBell(XtDisplay(TopLevel), 0);
	    mesgPane(XRN_SERIOUS, mesg_name, EMPTY_SUBJECT_MSG);
	    mesgPane(XRN_SERIOUS | XRN_SAME_LINE, mesg_name, FILL_IN_RESEND_MSG);
	    if (app_resources.editorCommand)
		Call_Editor(False, False);
	    TextEnableRedisplay(ComposeText);
	    CLEANUP(); return;
 	}

	if ((PostingMode == FOLLOWUP) || (PostingMode == FOLLOWUP_REPLY)) {
	    if (fieldExists("References:", 0) < 0) {
		buffer_size = 0;
		CHECK_SIZE_EXTENDED(buffer, buffer_size, buffer_total_size,
				    sizeof("References: \n") +
				    strlen(Header.id));
		(void) sprintf(buffer, "References: %s\n", Header.id);
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
		Call_Editor(True, False);
	    TextEnableRedisplay(ComposeText);
	    CLEANUP(); return;
	}

	buffer_size = 0;
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
	      char *addr, *addr_buf, *end, *nl;

	      addr = strchr(buffer, ':') + 1;
	      addr_buf = addr = XtNewString(addr);

	      /* Get rid of newlines */
	      while ((nl = strchr(addr, '\n')))
		*nl = ' ';

	      /* Strip whitespace in front */
	      while (isspace((unsigned char) *addr))
		addr++;

	      /* Strip whitespace at the end */
	      for (end = strchr(addr, '\0') - 1;
		   end > addr && isspace((unsigned char) *end);
		   end--)
		*end = '\0';

	      /* Now make sure its basic syntax is correct */
	      if (! IsValidAddressField(addr)) {
		  XBell(XtDisplay(TopLevel), 0);
		  mesgPane(XRN_SERIOUS, 0, BAD_FROM_MSG, addr);
		  XtFree(addr_buf);
		  if (app_resources.editorCommand)
		    Call_Editor(True, False);
		  TextEnableRedisplay(ComposeText);
		  CLEANUP(); return;
	      }

#ifdef SENDMAIL_VERIFY
	      if (app_resources.verifyFrom) {
		char *verify_command, *ptr, *ptr2;
		int cmd_length;

		while (isspace((unsigned char)*addr))
		  addr++;
		if ((ptr = strchr(addr, '\n'))) {
		  *ptr-- = '\0';
		  while ((ptr >= addr) && isspace((unsigned char)*ptr))
		    *ptr-- = '\0';
		}

		cmd_length = strlen(SENDMAIL_VERIFY) + strlen(addr) + 4;
		for (ptr = strpbrk(addr, "\'\\"); ptr;
		     ptr = strpbrk(ptr + 1, "\'\\"))
		  cmd_length += 3;

		verify_command = XtMalloc(cmd_length);
		(void) strcpy(verify_command, SENDMAIL_VERIFY);
		ptr = &verify_command[sizeof(SENDMAIL_VERIFY)-1];
		*ptr++ = ' '; *ptr++ = '\'';
		for (ptr2 = addr; *ptr2; ptr2++) {
		  if (*ptr2 == '\'' || *ptr2 == '\\') {
		    *ptr++ = '\''; *ptr++ = '\\'; *ptr++ = *ptr2; *ptr++ = '\'';
		  }
		  else {
		    *ptr++ = *ptr2;
		  }
		}
		*ptr++ = '\''; *ptr++ = '\0';

		if (system(verify_command)) {
		  XBell(XtDisplay(TopLevel), 0);
		  mesgPane(XRN_SERIOUS, 0, BAD_FROM_MSG, addr);
		  XtFree(addr_buf);
		  if (app_resources.editorCommand)
		    Call_Editor(True, False);
		  TextEnableRedisplay(ComposeText);
		  CLEANUP(); return;
		}
	      }
#endif /* SENDMAIL_VERIFY */

	      XtFree(addr_buf);
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
		Call_Editor(True, False);
	    TextEnableRedisplay(ComposeText);
	    CLEANUP(); return;
	}
	    
	if (fieldExists("Newsgroups:", 0) < 0) {
	    int got_groups;

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
		Call_Editor(True, False);
	    TextEnableRedisplay(ComposeText);
	    CLEANUP(); return;
	}

	buffer_size = 0;
	returnField("Newsgroups:", &buffer, &buffer_size, &buffer_total_size);

	/* "buffer" now contains the original "Newsgroups" field. */

	XtFree(buffer2);
	buffer2 = XtNewString(buffer);
	buffer2_size = strlen(buffer2);
	buffer2_total_size = buffer2_size + 1;

	/* "buffer2" now contains the original "Newsgroups" field */

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

	/* "buffer" now contains the fixed-up "Newsgroups" field */

	/* Need to preserve buffer because newsgroupsStatusUnion will
	   modify it. */
	XtFree(buffer3);
	buffer3 = XtNewString(buffer);
	buffer3_size = j;
	buffer3_total_size = j + 1;

	/* "buffer3" now contains the fixed-up "Newsgroups" field */

	for (ptr = buffer + 11 /* skip "Newsgroups:" */; *ptr == ' ';
	     ptr++) /* empty */;
	newsgroups_status = newsgroupsStatusUnion(ptr);

	if (! (newsgroups_status & NG_POSTABLE)) {
	    mesgPane(XRN_ERROR, mesg_name, NO_POSTABLE_NG_MSG);
	    if (app_resources.editorCommand)
		Call_Editor(False, False);
	    TextEnableRedisplay(ComposeText);
	    CLEANUP(); return;
	}

	if (strcmp(buffer2, buffer3)) {
	    stripField("Newsgroups:");
	    addField(buffer3);
	}

	i = j = 0;

#define COUNT_NEWSGROUPS(var, buffer, name) \
	if (! var) { \
	  ptr = buffer + sizeof(name); \
	  while (*ptr && ((*ptr == ' ') || (*ptr == '\t') || \
			  (*ptr == '\n'))) \
	    ptr++; \
	  if (! *ptr) \
	    var = 0; \
	  else \
	    for ( ; ptr; ptr = strchr(ptr + 1, ',')) \
	      var++; \
	}

#if CROSSPOST_PROHIBIT
	COUNT_NEWSGROUPS(i, buffer3, "Newsgroups:");
	if (i >= CROSSPOST_PROHIBIT) {
	  char *buf = XtMalloc(strlen(CROSSPOST_PROHIBIT_MSG) + 20);

	  sprintf(buf, CROSSPOST_PROHIBIT_MSG, i, CROSSPOST_PROHIBIT-1);

	  XBell(XtDisplay(TopLevel), 0);
	  ChoiceBox(TopLevel, buf, 1, OK_MSG);
	  if (app_resources.editorCommand)
	    Call_Editor(True, False);
	  TextEnableRedisplay(ComposeText);
	  CLEANUP(); XtFree(buf); return;
	}
#endif
	if (app_resources.warnings.posting.crossPost) {
	  char *buf = XtMalloc(strlen(CROSSPOST_CONFIRM_MSG) + 10);

	  COUNT_NEWSGROUPS(i, buffer3, "Newsgroups:");
	  if ((i >= app_resources.warnings.posting.crossPost) &&
	      sprintf(buf, CROSSPOST_CONFIRM_MSG, i) &&
	      (ChoiceBox(TopLevel, buf, 2, POST_MSG, EDIT_MSG) == 2)) {
	    if (app_resources.editorCommand)
	      Call_Editor(True, True);
	    TextEnableRedisplay(ComposeText);
	    CLEANUP(); XtFree(buf); return;
	  }

	  XtFree(buf);
	}
	if (app_resources.warnings.posting.followupTo) {
	  COUNT_NEWSGROUPS(i, buffer3, "Newsgroups:");
	  if (i >= app_resources.warnings.posting.followupTo) {
	    buffer2_size = 0;
	    returnField("Followup-To:", &buffer2, &buffer2_size,
			&buffer2_total_size);
	    if (*buffer2) {
	      COUNT_NEWSGROUPS(j, buffer2, "Followup-To:");
	    }
	    if (j) {
	      char *buf =
		XtMalloc(strlen(FOLLOWUP_FOLLOWUPTO_CONFIRM_MSG) + 20);

	      if ((j >= app_resources.warnings.posting.followupTo) &&
		  sprintf(buf, FOLLOWUP_FOLLOWUPTO_CONFIRM_MSG, i, j) &&
		  (ChoiceBox(TopLevel, buf, 2, POST_MSG, EDIT_MSG) == 2)) {
		if (app_resources.editorCommand)
		  Call_Editor(True, True);
		TextEnableRedisplay(ComposeText);
		CLEANUP(); XtFree(buf); return;
	      }

	      XtFree(buf);
	    }
	    else {
	      char *buf = XtMalloc(strlen(FOLLOWUP_CONFIRM_MSG) + 10);

	      sprintf(buf, FOLLOWUP_CONFIRM_MSG, i);
	      if (ChoiceBox(TopLevel, buf, 2, POST_MSG, EDIT_MSG) == 2) {
		if (app_resources.editorCommand)
		  Call_Editor(True, True);
		TextEnableRedisplay(ComposeText);
		CLEANUP(); XtFree(buf); return;
	      }
	    }
	  }
	}

	if (fieldIsMultiple("Path:")) {
	    XBell(XtDisplay(TopLevel), 0);
	    mesgPane(XRN_SERIOUS, mesg_name, MULTI_MSG, "Path");
	    if (app_resources.editorCommand)
		Call_Editor(True, False);
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

    if ((ptr2 = strstr(ptr, "\n\n"))) {
      ptr2 += 2;
      while (*ptr2 && isspace((unsigned char)*ptr2))
	ptr2++;
    }

    if (! (ptr2 && *ptr2 && strncmp(ptr2, SIG_PREFIX, sizeof(SIG_PREFIX)-1))) {
      XBell(XtDisplay(TopLevel), 0);
      mesgPane(XRN_SERIOUS, mesg_name, NO_BODY_MSG);
      if (app_resources.editorCommand)
	Call_Editor(True, False);
      TextSetInsertionPoint(ComposeText, TextGetLength(ComposeText));
      TextEnableRedisplay(ComposeText);
      XtFree(ptr);
      CLEANUP();
      return;
    }

    if (! check_quoted_text(ptr) &&
	(ConfirmationBox(ComposeTopLevel, ONLY_INCLUDED_MSG, YES_STRING,
			 RE_EDIT_MSG, True) == XRN_CB_ABORT)) {
      if (app_resources.editorCommand)
	Call_Editor(True, True);
      TextSetInsertionPoint(ComposeText, ptr2 - ptr);
      TextEnableRedisplay(ComposeText);
      XtFree(ptr);
      CLEANUP();
      return;
    }

    /* Strip empty Followup-To fields, since some NNTP servers choke
       on them. */
    buffer_size = 0;
    returnField("Followup-To:", &buffer, &buffer_size, &buffer_total_size);
    ptr2 = buffer + sizeof("Followup-To:") - 1;
    while (*ptr2 && isspace(*ptr2))
      ptr2++;
    if (! *ptr2) {
      stripField("Followup-To:");
      FREE(ptr);
      ptr = TextGetString(ComposeText);
    }

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
	  {
	    char *file, *alt_file;

	    if (mode == XRN_NEWS) {
	      file = app_resources.saveSentPostings;
	      alt_file = app_resources.saveSentMail;
	    }
	    else {
	      file = app_resources.saveSentMail;
	      alt_file = app_resources.saveSentPostings;
	    }
	    if (file && ((! alt_file) ||
			 (strcmp(file, alt_file) != 0) ||
			 (! saved))) {
	      saveMessage(file, ptr);
	      saved++;
	    }
	    mesgPane(XRN_INFO, mesg_name, (mode == XRN_NEWS) ?
		     ((newsgroups_status & NG_MODERATED) ?
		      MAILED_TO_MODERATOR_MSG : ARTICLE_POSTED_MSG) :
		     MAIL_MESSAGE_SENT_MSG);
	    break;
	  }
	}
	tries--;
	if ((mode == XRN_NEWS) &&
	    ((PostingMode == FOLLOWUP_REPLY) || (PostingMode == POST_MAIL))) {
	  ptr = insert_courtesy_tag(ptr);
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
     FILE *infile = NULL;
     
     text = XtMalloc(size);

     if (PostingMode == REPLY) {
        (void) sprintf(text, REPLY_YOU_WRITE_MSG, Header.id);
     } else if (PostingMode == FORWARD) {
        (void) sprintf(text, FORWARDED_ARTIKEL_MSG, Header.id, Header.from);
     } else {
        (void) sprintf(text, FOLLOWUP_AUTHOR_WRITE_MSG, Header.id, Header.from);
     }

     cur_size = strlen(text);

     if (app_resources.includeCommand && PostingMode != FORWARD) {
	  char cmdbuf[1024];

	  if (Header.artFile) {
	    sprintf(cmdbuf, app_resources.includeCommand,
		    app_resources.includePrefix,
		    file_cache_file_name(FileCache, Header.artFile));
	    infile = xrn_popen(cmdbuf, "r");
	  }

	  if (! infile) {
	       mesgPane(XRN_SERIOUS, 0, CANT_INCLUDE_CMD_MSG);
	       FREE(text);
	       return(0);
	  }

	  prefix = "";
	  prefix_size = 0;
     }
     else {
	  if (Header.artFile)
	    infile = fopen(file_cache_file_name(FileCache, Header.artFile), "r");
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
    if (! IS_RESPONSE(PostingMode))
      return;
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
    char *ptr, *confirms[5], *question;
    int confirm_count = 0;
    int mesg_name = newMesgPaneName();
    int choice;
    int mode1 = PostingMode, mode2 = PostingMode;

    read(inchannel,buffer,1);
    if(editing_status != Completed || EditorFileName == NIL(char)){
	return;
    }

    confirms[confirm_count++] = ABORT_STRING;

    confirms[confirm_count++] = RE_EDIT_MSG;
   
    switch (PostingMode) {
    case POST:
      question = ASK_POST_ARTICLE_MSG;
      break;
    case FOLLOWUP:
      question = ASK_POST_ARTICLE_MSG;
      confirms[confirm_count++] = AS_REPLY_MSG;
      confirms[confirm_count++] = AS_FOLLOWUP_REPLY_MSG;
      mode1 = REPLY;
      mode2 = FOLLOWUP_REPLY;
      break;
    case REPLY:
      question = ASK_SEND_MSG;
      confirms[confirm_count++] = AS_FOLLOWUP_MSG;
      confirms[confirm_count++] = AS_FOLLOWUP_REPLY_MSG;
      mode1 = FOLLOWUP;
      mode2 = FOLLOWUP_REPLY;
      break;
    case FORWARD:
    case GRIPE:
    case MAIL:
      question = ASK_SEND_MSG;
      break;
    case FOLLOWUP_REPLY:
      question = ASK_POST_SEND_MSG;
      confirms[confirm_count++] = AS_FOLLOWUP_MSG;
      confirms[confirm_count++] = AS_REPLY_MSG;
      mode1 = FOLLOWUP;
      mode2 = REPLY;
      break;
    case POST_MAIL:
    default:
      question = ASK_POST_SEND_MSG;
      break;
    }

    confirms[confirm_count++] = YES_STRING;
      
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
	mesgPane(XRN_SERIOUS, mesg_name, NO_CHANGE_MSG, EditorFileName);
	(void) fclose(filefp);
	(void) unlink(EditorFileName);
	editing_status = NoEdit;
	compAbortUtil(mesg_name, False);
	return;
    }

    if (buf.st_size == 0) {
	mesgPane(XRN_SERIOUS, mesg_name, ZERO_SIZE_MSG, EditorFileName);
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

    TextSetString(ComposeText, ptr);
    FREE(ptr);

    choice = ChoiceBox(TopLevel, question, confirm_count, confirms[0],
		       confirms[1], confirms[2], confirms[3], confirms[4]);

    if (choice == 1) {
      compAbortUtil(mesg_name, True);
      (void) unlink(EditorFileName);
      editing_status = NoEdit;
    }
    else if (choice == 2) {
      Call_Editor(False, False);
    }
    else if (choice == confirm_count) {
      compSendFunction(0, 0, 0, 0);
    }
    else {
      PostingMode = (choice == 3) ? mode1 : mode2;
      switch_message_type(&Header);
      Call_Editor(True, True);
    }

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
Call_Editor(
	    _ANSIDECL(Boolean,	save_message),
	    _ANSIDECL(Boolean,	preserve_mtime)
	    )
     _KNRDECL(Boolean,	save_message)
     _KNRDECL(Boolean,	preserve_mtime)
{
    char *dsp, *file;
    char buffer[1024], buffer2[1024];
    FILE *filefp;
    char *header = 0;
    int mesg_name = newMesgPaneName();
    struct stat statbuf1, statbuf2;

    if (save_message) {
	header = TextGetString(ComposeText);
    }
    
    if ((editing_status == NoEdit) || header) {
	editing_status = InProgress;
	if (EditorFileName) {
	  if (preserve_mtime) {
	    statbuf1.st_mtime = 0;
	    (void) stat(EditorFileName, &statbuf1);
	  }
	  utTempnamFree(EditorFileName);
	}
	EditorFileName = utTempnam(app_resources.tmpDir, "xrn");
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

	if (stat(EditorFileName, &statbuf2) == -1) {
	    mesgPane(XRN_SERIOUS, mesg_name, CANT_STAT_TEMP_MSG, EditorFileName,
		     errmsg(errno));
	    editing_status = NoEdit;
	    compAbortUtil(mesg_name, False);
	    return (-1);
	}
	if (!preserve_mtime || !statbuf1.st_mtime ||
	    (originalBuf.st_mtime == statbuf1.st_mtime))
	  originalBuf = statbuf2;
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
    Widget pane;
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
	(app_resources.editorCommand && IS_RESPONSE(PostingMode)))
      includeArticleText3(&header);

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

    ComposeLabel = XtCreateManagedWidget("label", labelWidgetClass, pane,
					 labelArgs, XtNumber(labelArgs));

    ComposeText = TextCreate("text", False, pane);
    TextSetString(ComposeText, header);
    FREE(header);
 
    ComposeButtonBox = ButtonBoxCreate("box", pane);

    compose_buttons(ComposeButtonBox, True);
    
    if (app_resources.editorCommand != NIL(char)) {
	return(Call_Editor(True, False));
    }
    else {
	XtRealizeWidget(ComposeTopLevel);
	/*
	  Needs to happen after widget is realized, so that the width
	  of the button box is correct.
	  */
	ButtonBoxDoneAdding(ComposeButtonBox);

	XtSetKeyboardFocus(ComposeTopLevel, ComposeText);

	XtVaGetValues(ComposeLabel,
		      XtNheight, (XtPointer) &height_val,
		      (char *) 0);

	XawPanedSetMinMax(ComposeLabel, (int) height_val, (int) height_val);
	XawPanedAllowResize(TEXT_PANE_CHILD(ComposeText), True);
    
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

static void compose_buttons(box, first_time)
     Widget box;
     int first_time;
{
  int both, followup, reply;
  
  both = (PostingMode == FOLLOWUP_REPLY);
  followup = both || (PostingMode == FOLLOWUP);
  reply = both || (PostingMode == REPLY);

  setButtonActive(CompButtonList, "compSwitchFollowup", reply);
  setButtonActive(CompButtonList, "compSwitchReply", followup);
  setButtonActive(CompButtonList, "compSwitchBoth",
		  (followup || reply) && !both);
  setButtonActive(CompButtonList, "compIncludeArticle", followup || reply);

  doButtons(NULL, box, CompButtonList, &CompButtonListCount, BOTTOM);

  if (! first_time)
    ButtonBoxDoneAdding(box);
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
			    (Header.artFile ?
			     file_cache_file_name(FileCache, Header.artFile) :
			     "NIL"));
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

    (void) strcpy(info, SIG_PREFIX);
    count = fread(&info[4], sizeof(char), sizeof(info) - 4, infofp);
    info[count + 4] = '\0';

    if (! feof(infofp)) {
	/* Signature file is too big */
	mesgPane(XRN_SERIOUS, 0, SIGNATURE_TOO_BIG_MSG, sigfile);
	retinfo = NIL(char);
    }
    else if (strncmp(info + 4, SIG_PREFIX, 4) == 0) {
	retinfo = info + 4;
    } else {
	retinfo = info;
    }

    (void) (*close_func)(infofp);

    return retinfo;
}


/*
  Update the headers of a message so that it has all the appropriate
  headers for either a followup, reply, or both.

  If "first_time" is true, then assume that there is no current
  message and all headers have to be created from scratch.  Otherwise,
  check for headers in the current message and only create the ones
  that need to be created; also, delete headers that are no longer
  relevant.

  Return an allocated string containing the headers to be added.
*/
static char *update_headers(Header, first_time, followup, reply)
     struct header *Header;
     int first_time, followup, reply;
{
  char *message = 0;
  int message_size = 0, message_total_size = 0;

  CHECK_SIZE(1);
  message[0] = '\0';
  message_size = 0;

  if (first_time || (fieldExists("From:", NULL)  < 0))
    buildFrom(Header, &message, &message_size, &message_total_size);
  if (first_time || (fieldExists("Reply-To:", NULL) < 0))
    buildReplyTo(&message, &message_size, &message_total_size);
  if (first_time || (fieldExists("Subject:", NULL) < 0))
    buildSubject(Header, &message, &message_size, &message_total_size);

  if (followup) {
    int num_groups;

    if (first_time || (fieldExists("Path:", NULL) < 0))
      buildPath(Header, &message, &message_size, &message_total_size);

    if (first_time || (fieldExists("Newsgroups:", NULL) < 0)) {
      num_groups = buildNewsgroups(Header, &message, &message_size,
				   &message_total_size);
      if (app_resources.warnings.followup.crossPost &&
	  num_groups >= app_resources.warnings.followup.crossPost)
	mesgPane(XRN_WARNING, 0, FOLLOWUP_MULTIPLE_NGS_MSG);
      if (app_resources.warnings.followup.followupTo &&
	  *Header->followupTo && strcmp(Header->followupTo, Header->newsgroups) &&
	  strcmp(Header->followupTo, "poster"))
	mesgPane(XRN_WARNING, 0, FOLLOWUP_FOLLOWUPTO_MSG);
    }

    if (first_time || (fieldExists("Distribution:", NULL) < 0))
      buildDistribution(Header, &message, &message_size, &message_total_size);

    if (first_time || (fieldExists("Followup-To:", NULL) < 0)) {
      CHECK_SIZE(sizeof("Followup-To: \n") - 1);
      (void) strcat(message, "Followup-To: \n");
    }

    if (first_time || (fieldExists("References:", NULL) < 0))
      buildReferences(Header, &message, &message_size, &message_total_size);

    if (first_time || (fieldExists("Organization:", NULL) < 0)) {
      CHECK_SIZE(sizeof("Organization: \n") - 1 + strlen(Header->organization));
      (void) sprintf(&message[strlen(message)], "Organization: %s\n",
		     Header->organization);
    }

    if (first_time || (fieldExists("Keywords:", NULL) < 0)) {
      CHECK_SIZE(sizeof("Keywords: \n") - 1);
      (void) strcat(message, "Keywords: ");
      if ((Header->keywords != NIL(char)) && (*Header->keywords != '\0')) {
	CHECK_SIZE(strlen(Header->keywords));
	(void) strcat(message, Header->keywords);
      }
      (void) strcat(message, "\n");
    }
  }
  else if (! first_time) {
    stripField("Path:");
    stripField("Newsgroups:");
    stripField("Distribution:");
    stripField("Followup-To:");
    stripField("References:");
    stripField("Organization:");
    stripField("Keywords:");
  }

  if (reply) {
    if (first_time || (fieldExists("To:", NULL) < 0)) {
      char *reply_addr = (Header->replyTo && *Header->replyTo)
	? Header->replyTo : Header->from;
      CHECK_SIZE(sizeof("To: \n") - 1 + strlen(reply_addr));
      (void) sprintf(&message[strlen(message)], "To: %s\n", reply_addr);
    }

    if (app_resources.cc == True) {
      if (first_time || (fieldExists("Cc:", NULL) < 0)) {
	CHECK_SIZE(sizeof("Cc: \n") - 1 + strlen(Header->user));
	sprintf(&message[strlen(message)], "Cc: %s\n", Header->user);
      }
    }

    if (! followup) {
      if (first_time || (fieldExists("X-Newsgroups:", NULL) < 0)) {
	CHECK_SIZE(sizeof("X-Newsgroups: \n") - 1 +
		   strlen(Header->newsgroups));
	(void)sprintf(&message[strlen(message)],
		      "X-Newsgroups: %s\n", Header->newsgroups);
      }
      if (first_time || (fieldExists("In-reply-to:", NULL) < 0)) {
	CHECK_SIZE(sizeof("In-reply-to: \n") - 1 + strlen(Header->id));
	(void) sprintf(&message[strlen(message)],
		       "In-reply-to: %s\n", Header->id);
      }
    }
  }
  else if (! first_time) {
    stripField("To:");
    stripField("Cc:");
    stripField("X-Newsgroups:");
    stripField("In-reply-to:");
  }
  
  return message;
}

static char *followup_or_reply_title(Header, followup, reply)
     struct header *Header;
     int followup, reply;
{
  struct newsgroup *newsgroup = Header->newsgroup;
  art_num current = newsgroup->current;
  static char title[LABEL_SIZE];

  (void) sprintf(title, followup ? (reply ? FOLLOWUP_REPLY_TO_TITLE_MSG
				    : FOLLOWUP_TO_TITLE_MSG) :
		 REPLY_TO_TITLE_MSG, current, newsgroup->name);
  return title;
}

static void followup_or_reply _ARGUMENTS((int, int));

static void followup_or_reply(followup, reply)
    int followup, reply;
{
    struct newsgroup *newsgroup = CurrentGroup;
    art_num current = newsgroup->current;
    char *title;
    char *message = 0;
    int message_size = 0, message_total_size = 0;
    int OldPostingMode = PostingMode;
    struct article *art;

    CHECK_COMPOSE_PANE();
    
    if (getHeader(current, &Header) != XRN_OKAY)
	return;

    art = artStructGet(newsgroup, current, False);
    if (art->file)
      file_cache_file_copy(FileCache, *art->file, &Header.artFile);

    if (followup) {
	if (! strcmp(Header.followupTo, "poster")) {
	    int ret;

	    ret = ChoiceBox(TopLevel,
			    reply ? ASK_POSTER_FANDR_MSG : ASK_POSTER_REPLY_MSG,
			    reply ? 2 : 3,
			    POST_AND_SEND_MSG,
			    reply ? SEND_MAIL_MSG : POST_MSG,
			    SEND_MAIL_MSG);

	    if (reply && (ret == 2))
	      ret = 3;

	    /*
	      1 means post & mail
	      2 means post
	      3 means mail
	      */

	    followup = (ret < 3);
	    reply = (ret != 2);
	}
    }

    title = followup_or_reply_title(&Header, followup, reply);

    if (followup)
	if (reply)
	  PostingMode = FOLLOWUP_REPLY;
	else
	  PostingMode = FOLLOWUP;
    else
      PostingMode = REPLY;

    message = update_headers(&Header, True, followup, reply);
    message_size = strlen(message);
    message_total_size = message_size;
    
    CHECK_SIZE(1);
    (void) strcat(message, "\n");

    if (composePane(title, message, strlen(message))) {
	 PostingMode = OldPostingMode;
    }

    return;
}

static void switch_message_type(Header)
     struct header *Header;
{
  int both, followup, reply;
  char *title;
  char *headers;

 if (! ComposeTopLevel)
   return;

  both = (PostingMode == FOLLOWUP_REPLY);
  followup = both || (PostingMode == FOLLOWUP);
  reply = both || (PostingMode == REPLY);

  if (! (followup || reply))
    return;

  title = followup_or_reply_title(Header, followup, reply);
  XtVaSetValues(ComposeLabel, XtNlabel, title, 0);

  headers = update_headers(Header, False, followup, reply);
  addField(headers);
  XtFree(headers);

  compose_buttons(ComposeButtonBox, False);

  TextSetInsertionPoint(ComposeText, TextGetLength(ComposeText));
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
    if (art->file)
      file_cache_file_copy(FileCache, *art->file, &Header.artFile);

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

    CHECK_SIZE(sizeof("Subject:   []\n") - 1 +
	       strlen(Header.subject) + strlen(newsgroup->name) +
	       (int) (current / 10) + 1);
    (void) sprintf(&message[strlen(message)],
		   "Subject: %s  [%s]\n", Header.subject,
		   newsgroup->name);

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
    int got_groups;

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


void post(
	  _ANSIDECL(Boolean,	ingroupp)
	  )
     _KNRDECL(Boolean,	ingroupp)
{
    post_or_mail(ingroupp, True, False);
}

void mail()
{
    post_or_mail(False, False, True);
}

void post_and_mail(
		   _ANSIDECL(Boolean,	ingroupp)
		   )
     _KNRDECL(Boolean,	ingroupp)
{
    post_or_mail(ingroupp, True, True);
}

static Boolean addressMatches(art_line, user, host)
     char *art_line, *user, *host;
{
  char *at, *line_user, *line_host;

  assert(art_line && user && host);

  for (at = strchr(art_line, '@'); at; at = strchr(at + 1, '@')) {
    line_user = at - 1;
    while ((line_user > art_line) &&
	   !isspace((unsigned char)*line_user) && (*line_user != '<'))
      line_user--;
    line_host = at + 1;
    if (! ((strncmp(line_user, user, at - line_user) &&
	    strncmp(line_user, "root", at - line_user)) ||
	   strncmp(line_host, host, strlen(host))))
      return True;
  }
  return False;
}

			      
static Boolean _canCancelArticle()
{
  return(addressMatches(CancelHeader.from,
			CancelHeader.real_user, CancelHeader.real_host) ||
	 addressMatches(CancelHeader.sender,
			CancelHeader.real_user, CancelHeader.real_host));
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
	mesgPane(XRN_SERIOUS, mesg_name, USER_CANT_CANCEL_MSG);
	return;
    }

    CHECK_SIZE(1);
    *message = '\0';
    message_size = 0;

    buildFrom(&CancelHeader, &message, &message_size, &message_total_size);
#ifndef INEWS
    {
      char *buf = 0;
      int buf_size = 0, buf_total_size = 0;

      CHECK_SIZE_EXTENDED(buf, buf_size, buf_total_size, 1);
      *buf = '\0';
      buf_size = 0;

      buildRealFrom(&CancelHeader, &buf, &buf_size, &buf_total_size);
      if (strcmp(message, buf))
	buildSender(&CancelHeader, &message, &message_size, &message_total_size);
    }
#endif /* ! INEWS */

    buildReplyTo(&message, &message_size, &message_total_size);
    buildPath(&CancelHeader, &message, &message_size, &message_total_size);

    CHECK_SIZE(sizeof("Subject: cancel \n") - 1 + strlen(CancelHeader.id));
    (void) sprintf(&message[strlen(message)], "Subject: cancel %s\n",
		   CancelHeader.id);

    CHECK_SIZE(sizeof("Newsgroups: \n") - 1 +
	       strlen(CancelHeader.newsgroups));
    (void) sprintf(&message[strlen(message)], "Newsgroups: %s\n",
		   CancelHeader.newsgroups);

    buildReferences(&CancelHeader, &message, &message_size, &message_total_size);

    if (CancelHeader.distribution && *CancelHeader.distribution) {
	CHECK_SIZE(strlen ("Distribution: \n") + strlen(CancelHeader.distribution));
	(void) sprintf(&message[strlen(message)], "Distribution: %s\n",
		       CancelHeader.distribution);
    }

    CHECK_SIZE(sizeof("Control: cancel \n") - 1 + strlen(CancelHeader.id));
    (void) sprintf(&message[strlen(message)], "Control: cancel %s\n",
		   CancelHeader.id);

    CHECK_SIZE(sizeof("\nCancel message from XRN .\n") + sizeof(XRN_VERSION) - 2);
    (void) sprintf(&message[strlen(message)], "\nCancel message from XRN %s.\n",
		   XRN_VERSION);

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
	    
    case POST_OKAY: {
	char *groups = XtNewString(CancelHeader.newsgroups);
	unsigned long status = newsgroupsStatusUnion(groups);

	XtFree(groups);

	if (status & NG_MODERATED)
	  mesgPane(XRN_INFO, mesg_name, CANCEL_TO_MODERATOR_MSG);
	else
	  mesgPane(XRN_INFO, mesg_name, CANCELLED_ART_MSG);

	break;
      }
    }

    freeHeader(&CancelHeader);
    FREE(message);

    return;
}

/*
  Reformat and trim the "References:" line.  Separate the IDs from the
  field name with a single space, and separate all IDs with a single
  space.  Remove corrupted IDs, and make sure the length of the whole
  thing including the final newline is MAXREFSIZE or less characters
  when done.  When trimming because there are too many IDs, preserve
  the first ID and then as many as possible at the end while remaining
  within the length limit.

  Returns the length of the reformatted line, not including the final
  null.
*/
#define MAXREFSIZE 510 /* Includes the final newline but not the trailing null */

static int trim_references(refs)
     char *refs;
{
  char *inptr = XtNewString(refs), *inbuf = inptr;
  char *outptr = XtMalloc(strlen(inptr)+1), *outbuf = outptr;
  char *ptr, **ids;
  int id_count = 0, id_count_size = 1, start_at;
  int total_length = 1; /* the final newline */
  
  ids = (char **) XtMalloc(id_count_size * sizeof(*ids));

  /* Copy field name */
  while (*inptr != ':') {
    *outptr++ = *inptr++;
    total_length++;
  }
  *outptr++ = *inptr++;
  total_length++;

  /* Skip beginning whitespace */
  while (isspace((unsigned char)*inptr))
    inptr++;

  /* Tokenize */
  for (ptr = strtok(inptr, " \t\n"); ptr; ptr = strtok(NULL, " \t\n")) {
    int len;

    if ((*ptr != '<') /* No opening brace */
	|| strchr(ptr + 1, '<') /* An extra opening brace */
	|| (ptr[(len = strlen(ptr)) - 1] != '>') /* No closing brace */
	|| (strchr(ptr, '>') - ptr != len - 1)) /* An extra closing brace */
      continue;

    if (id_count == id_count_size) {
      id_count_size *= 2;
      ids = (char **) XtRealloc((char *)ids, id_count_size * sizeof(*ids));
    }

    ids[id_count++] = ptr;
    total_length += 1 + len;	/* space and then message ID */
  }

  /* Always include the first Message ID.  Other than that, figure out
     how many we can include at the end while still remaining within
     the line length limit.

     The GNKSA says that we should always include message IDs that are
     mentioned in the body of the message.  We're not doing that
     explicitly right now.
  */

  for (start_at = 1; (start_at < id_count) && (total_length > MAXREFSIZE);
       start_at++) {
    total_length -= 1 + strlen(ids[start_at]);
  }

  /* Copy the first ID */
  *outptr++ = ' ';
  if (! id_count) {
    goto done;
  }
  strcpy(outptr, ids[0]);
  outptr += strlen(ids[0]);

  /* Copy remaining IDs */
  while (start_at < id_count) {
    *outptr++ = ' ';
    strcpy(outptr, ids[start_at]);
    outptr += strlen(ids[start_at]);
    start_at++;
  }

 done:
  *outptr++ = '\n';
  *outptr++ = '\0';
  (void) strcpy(refs, outbuf);
  XtFree(inbuf);
  XtFree(outbuf);
  XtFree((char *)ids);
  return(total_length);
}

void compSwitchFollowupFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
  PostingMode = FOLLOWUP;
  switch_message_type(&Header);
}

void compSwitchReplyFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
  PostingMode = REPLY;
  switch_message_type(&Header);
}

void compSwitchBothFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
  PostingMode = FOLLOWUP_REPLY;
  switch_message_type(&Header);
}

/*
  Check if the article appears to contain only quoted text.  If so,
  return False; else, return True.  That is, the return value of this
  function indicates whether the article "should be posted," given its
  included-text content.

  Does this as follows:

  1) Skip past the header and any whitespace following it.
  2) If the first line doesn't start with "I" (as in "In article..."),
     return True.
  3) If the next line doesn't start with whitespace, return True.
  4) If, before the end of the message or a line containing the
     signature prefix, we encounter a line containing something other
     than whitespace that doesn't start with the include prefix,
     return True.
  5) Else, return false.
*/
  
static int check_quoted_text(article)
     char *article;
{
  char *ptr;
  int prefix_len = strlen(app_resources.includePrefix);

  /* 1) */
  if (! (ptr = strstr(article, "\n\n")))
    return True;
  ptr += 2;
  while (*ptr && isspace((unsigned char)*ptr))
    ptr++;

  /* 2) */
  if (*ptr != 'I')
    return True;

  /* 3) */
  if (! (ptr = strchr(ptr, '\n')))
    return True;
  ptr++;
  if (! isspace((unsigned char)*ptr))
    return True;

  /* 4) */
  for (article = strchr(ptr, '\n');
       article && strncmp(article + 1, SIG_PREFIX, sizeof(SIG_PREFIX)-1);
       article = ptr) {
    article++;
    ptr = strchr(article, '\n');
    if (*article && ! strncmp(article, app_resources.includePrefix, prefix_len))
      continue;
    while (*article && isspace((unsigned char)*article) &&
	   (!ptr || article < ptr))
      article++;
    if (*article && !isspace((unsigned char)*article))
      return True;
  }

  return False;
}

static char *insert_courtesy_tag(message)
     char *message;
{
  char *header, *note, *body, *new_message;
  int note_len;

  if (! (note = app_resources.courtesyCopyMessage))
    note = COURTESY_COPY_MSG;
  if (! *note)
    return message;

  header = message;

  body = strstr(message, "\n\n");
  body += 2;

  new_message = XtMalloc(strlen(message) + strlen(note) + 3);

  strncpy(new_message, header, body - header);
  new_message[body - header] = '\0';

  strcat(new_message, note);
  if (note[(note_len = strlen(note))-1] != '\n') {
    strcat(new_message, "\n\n");
  }
  else if ((note_len == 1) || note[note_len-2] != '\n') {
    strcat(new_message, "\n");
  }

  strcat(new_message, body);

  XtFree(message);
  return(new_message);
}

/*
  Converts address fields as follows:

  From			To
  ----			--
  address		address
  stuff1 <address>	address
  address (stuff2)	address

  Makes sure that "address" contains '@', contains only valid
  characters after the '@', and is quoted if it contains characters
  before the '@' that need quoting.  Makes sure that "stuff1" either
  is quoted or doesn't contain a period or comma.  Makes sure that
  "stuff1" and "stuff2" don't contain parentheses or angle brackets.
  Returns true if all this is OK or false otherwise.
*/

static Boolean check_address _ARGUMENTS((CONST char *));
static Boolean check_stuff _ARGUMENTS((CONST char *, Boolean));

static Boolean IsValidAddressField(value_in)
     CONST char *value_in;
{
  char *value, *value_buf, *address, *stuff, *ptr, *end;
  Boolean retval = True;

  value_buf = value = XtNewString(value_in);
  address = stuff = NULL;

  while ((ptr = strchr(value, '\n')))
    *ptr = ' ';
  while (isspace((unsigned char) *value))
    value++;
  for (end = strchr(value, '\0'), ptr = end - 1;
       ptr > value && isspace((unsigned char) *ptr);
       ptr--, end--)
    *ptr = '\0';

  /* Is it stuff1 <address>? */
  ptr = end - 1;
  if (ptr > value && *ptr == '>') {
    *ptr-- = '\0';
    for (ptr = value; *ptr && *ptr != '<'; ptr++)
      /* empty */;
    if (*ptr != '<') {
      retval = False;
      goto finished;
    }
    address = ptr + 1;
    stuff = value;
    *ptr-- = '\0';
    while (ptr > stuff && isspace((unsigned char) *ptr))
      *ptr-- = '\0';
    if (! (retval = check_address(address)))
      goto finished;
    if (! (retval = check_stuff(stuff, True)))
      goto finished;
  }
  /* Is it address (stuff2)? */
  else if (ptr > value && *ptr == ')') {
    *ptr-- = '\0';
    while (ptr > value && *ptr != '(')
      ptr--;
    if (*ptr != '(') {
      retval = False;
      goto finished;
    }
    address = value;
    stuff = ptr + 1;
    *ptr-- = '\0';
    while (ptr > address && isspace((unsigned char) *ptr))
      *ptr-- = '\0';
    if (! (retval = check_address(address)))
      goto finished;
    if (! (retval = check_stuff(stuff, False)))
      goto finished;
  }
  /* Otherwise, it's just an address */
  else {
    retval = check_address(value);
  }

 finished:
  XtFree(value_buf);
  return retval;
}

/*
  Make sure the address is in the form local-part@domain.  If
  local-part contains characters that need quoting, make sure that it
  is surrounded by quotation marks.  Returns True if the address is OK
  or False otherwise.
*/

static Boolean check_address(address)
     CONST char *address;
{
  static char special_chars[] = "()<>@,;:\\\"[] ";
  Boolean in_quotes, in_quoted_pair, in_local_part, found_dot;
  CONST char *ptr;

  in_quotes = in_quoted_pair = found_dot = False;
  in_local_part = True;

  for (ptr = address; *ptr; ptr++) {
    if (in_local_part) {
      if (in_quoted_pair) {
	in_quoted_pair = False;
	continue;
      }
      if (*ptr == '\\') {
	in_quoted_pair = True;
	continue;
      }
      if (in_quotes) {
	if (*ptr == '"')
	  in_quotes = False;
	continue;
      }
      if (*ptr == '"') {
	in_quotes = True;
	continue;
      }
      if (*ptr == '@') {
	if (ptr == address) /* No local part! */
	  return False;
	in_local_part = False;
	continue;
      }
    }
    if (strchr(special_chars, *ptr))
      return False;
    if (in_local_part)
      continue;
    if (*ptr == '.')
      found_dot++;
  }

  if (found_dot)
    return True;

  return False;
}

/*
  Make sure that "stuff" doesn't contain parentheses, angle
  brackets, or internal quotation marks.  Furthermore, if
  "needs_quoting" is true, make sure that it is enclosed in quotation
  marks if it contains periods or commas.
*/

static Boolean check_stuff(
			   _ANSIDECL(CONST char *,	stuff),
			   _ANSIDECL(Boolean,		needs_quoting)
			   )
     _KNRDECL(CONST char *,	stuff)
     _KNRDECL(Boolean,		needs_quoting)
{
  static char specials[] = "()<>\"";
  static char quoted_specials[] = ".,";

  CONST char *ptr, *end;
  Boolean is_quoted = False;

  end = strchr(stuff, '\0');

  if (*stuff == '"') {
    if (*--end != '"')
      return False;
    stuff++;
    is_quoted = True;
  }

  for (ptr = stuff; ptr < end; ptr++) {
    if (strchr(specials, *ptr))
      return False;
    if (needs_quoting && !is_quoted && strchr(quoted_specials, *ptr))
      return False;
  }

  return True;
}
