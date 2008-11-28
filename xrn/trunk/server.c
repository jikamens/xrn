
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: server.c,v 1.181 2006-01-03 16:38:46 jik Exp $";
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

#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include "avl.h"
#include "news.h"
#include "artstruct.h"
#include "mesg.h"
#include "error_hnds.h"
#include "resources.h"
#include "server.h"
#include "internals.h"
#include "mesg_strings.h"
#include "clientlib.h"
#include "xmisc.h"
#include "xthelper.h"
#include "compose.h"
#include "buttons.h"
#include "xrn.h"
#include "newsrcfile.h"
#include "varfile.h"
#include "dialogs.h"
#include "sort.h"
#include "file_cache.h"

#if defined(sun) && (defined(sparc) || defined(mc68000)) && !defined(SOLARIS)
#include <vfork.h>
#endif

extern int errno;

#define BUFFER_SIZE 1024
/* This constant must be a 2^x times BUFFER_SIZE, for some x. */
#define MAX_BUFFER_SIZE (8*BUFFER_SIZE)

Boolean ServerDown = False, PostingAllowed = True, FastServer = False;
int server_page_height = 24;

static struct newsgroup *currentNewsgroup = 0;
#define SETNEWSGROUP(n) (((n) == currentNewsgroup) ? 0 : \
			 getgroup((n), 0, 0, 0, True))

static void get_article_headers(struct newsgroup *, art_num);

/*
  Get a line of data from the server.

  Returns a char pointer to the line of data. The caller SHOULD NOT
  FREE THIS DATA, as it is static.

  The trailing "\r\n" sequence is stripped from the returned line.

  On error, sets ServerDown and returns an empty string.
  */

static char *get_data_from_server _ARGUMENTS((int));

static char *get_data_from_server(discard_excess)
     int discard_excess;
{
  static int size = 0, end, left, len;
  static char *str = NULL;
  Boolean continuing = False;

  if (! size) {
    size = BUFFER_SIZE;
    str = XtMalloc(size);
  }

  ServerDown = False;

  for (end = 0, left = size; ; end += len, left -= len) {
    if (get_server(&str[end], size - end) < 0) {
      ServerDown = True;
      *str = '\0';
      break;
    }

    len = strlen(&str[end]);

    if (left - len == 1) {
      continuing = True;
      /* Only one byte left at the end, which means that we didn't
	 read a full line and the last byte is the null put there by
	 fgets. */
      if (size >= MAX_BUFFER_SIZE) {
	if (discard_excess) {
	  char garbage_buf[BUFFER_SIZE];
	  do {
	    if (get_server(garbage_buf, sizeof(garbage_buf)) < 0) {
	      ServerDown = True;
	      *str = '\0';
	      break;
	    }
	  } while (strlen(garbage_buf) == sizeof(garbage_buf) - 1);
	}
	break;
      }
      left += size;
      size *= 2;
      str = XtRealloc(str, size);
    }
    else {
      if (continuing && (len == 1)) {
	/* It's possible that there was a "\r\n" pair split between
	   the previous and last get_server() calls. */
	if ((end > 1) && (str[end-1] == '\r') && (str[end] == '\n'))
	  str[end-1] = '\0';
      }
      break;
    }
  }

  return str;
}

static int wants_user_pass_authentication _ARGUMENTS((void));
static int user_pass_authentication _ARGUMENTS((void));

static int wants_user_pass_authentication()
{
  char *ptr = app_resources.authenticator;

  if (! ptr)
    /* Default to user/pass authentication. */
    return 1;
  else if (! app_resources.authenticatorCommand) {
    /* Fall back on user/pass. */
    app_resources.authenticator = NULL;
    return 1;
  }
     
  while (*ptr && isspace((unsigned char)*ptr))
    ptr++;

  return(! strncasecmp(ptr, "user/pass", sizeof("user/pass")-1));
}

/*
  Cache the password so the user only has to be prompted once.
*/
static char *authinfo_password = NULL;

/*
  This function assumes that wants_user_pass_authentication() has
  already been called and returned True.  I.e., don't call this
  function if you don't already know that the authenticator resource
  is in the correct format.
  */
static int user_pass_authentication()
{
  /*
    The format of the authenticator resource when user/pass
    authentication is being used is:

    ws "user/pass" ws username ws "/" ws password ws

    "ws" stands for whitespace, which is optional in all cases.  If
    the username is omitted, the return value of getUser() is used.
    If the slash is omitted, then the user is prompted for a
    password.  Neither the username nor the password may have spaces
    in it.

    The password is allowed to be there only if
    ALLOW_RESOURCE_PASSWORDS is defined to a non-zero value.
    */
  char *user = 0, *pass = 0, *ptr;
  char *buf = 0;
  char cmdbuf[BUFSIZ], *response;
  int retval;

  if (app_resources.authenticator) {
    buf = XtNewString(app_resources.authenticator);
    
    user = buf;

    while (*user && isspace((unsigned char)*user))
      user++;

    user += sizeof("user/pass") - 1;

    while (*user && isspace((unsigned char)*user))
      user++;

    if (! (ptr = strchr(user, '/')))
      ptr = strchr(user, '\0');

    while ((ptr > user) && isspace((unsigned char)*(ptr - 1)))
      ptr--;

    *ptr = '\0';

#if ALLOW_RESOURCE_PASSWORDS
    pass = app_resources.authenticator;
    if ((pass = strchr(pass, '/')) && (pass = strchr(pass + 1, '/'))) {
      pass++;
      while (*pass && isspace((unsigned char)*pass))
	pass++;
      ptr = strchr(pass, '\0');
      while ((ptr > pass) && isspace((unsigned char)*(ptr - 1)))
	ptr--;
      *ptr = '\0';
      if (! *pass)
	pass = 0;
    }
#endif /* ALLOW_RESOURCE_PASSWORDS */
  }
  
  if (! (user && *user)) {
    user = getUser();
    if (! *user) {
      retval = -1;
      goto done;
    }
  }
  else
    user = XtNewString(user);

  if (! pass)
    if (authinfo_password)
      pass = XtNewString(authinfo_password);
    else {
      if ((pass = PasswordBox(TopLevel, NNTP_PASSWORD_MSG)))
	authinfo_password = XtNewString(pass);
    }
  else
    pass = XtNewString(pass);

  if (! pass) {
    retval = -1;
    goto done;
  }

  (void) sprintf(cmdbuf, "AUTHINFO USER %s", user);
  put_server(cmdbuf);
  response = get_data_from_server(True);
  if (*response != CHAR_CONT) {
    retval = -1;
    goto done;
  }

  (void) sprintf(cmdbuf, "AUTHINFO PASS %s", pass);
  put_server(cmdbuf);
  response = get_data_from_server(True);
  if (*response != CHAR_OK) {
    if (authinfo_password) {
      XtFree(authinfo_password);
      authinfo_password = NULL;
      retval = 1;
    }
    else
      retval = -1;
    goto done;
  }

  retval = 0;
  
done:
  XtFree(buf);
  XtFree(user);
  XtFree(pass);
  return retval;
}


/*
  Returns 0 on successful authentication, <0 if authentication failed
  and can't be retried, or >0 if authentication failed and can be
  retried.
*/
  
static int authenticate _ARGUMENTS((void));

static int
authenticate() {
    extern FILE *ser_rd_fp, *ser_wr_fp;
    char tmpbuf[BUFSIZ], cmdbuf[BUFSIZ];
    char *authcmd;
    static int cookiefd = -1;
#ifdef USE_PUTENV
    static char *old_env = 0;
    char *new_env;
#endif

    if (wants_user_pass_authentication())
      return user_pass_authentication();

    /* If we have authenticated before, NNTP_AUTH_FDS already
       exists, pull out the cookiefd. Just in case we've nested. */
    if (cookiefd == -1 && (authcmd = getenv("NNTP_AUTH_FDS"))) {
	sscanf(authcmd, "%*d.%*d.%d", &cookiefd);
    }

    if (cookiefd == -1) {
	char *tempfile = utTempnam(app_resources.tmpDir, "xrn");
	FILE *f = fopen(tempfile, "w+");
	if (! f) {
	    mesgPane(XRN_SERIOUS, 0, CANT_OPEN_TEMP_MSG, tempfile, errmsg(errno));
	    return 1;
	}
	(void) unlink(tempfile);
	utTempnamFree(tempfile);
	cookiefd = fileno(f);
    }

    strcpy(tmpbuf, "AUTHINFO GENERIC ");
    strcat(tmpbuf, app_resources.authenticator);
    put_server(tmpbuf);

#ifdef USE_PUTENV
    sprintf(tmpbuf, "NNTP_AUTH_FDS=%d.%d.%d", fileno(ser_rd_fp),
	    fileno(ser_wr_fp), cookiefd);
    new_env = XtNewString(tmpbuf);
    putenv(new_env);
    XtFree(old_env);
    old_env = new_env;
#else
    sprintf(tmpbuf, "%d.%d.%d", fileno(ser_rd_fp), fileno(ser_wr_fp),
	    cookiefd);
    setenv("NNTP_AUTH_FDS", tmpbuf, 1);
#endif

    sprintf(cmdbuf, app_resources.authenticatorCommand,
	    app_resources.authenticator);
    /* We have to assume that authentication can't be retried if this
       fails, because there's no way for the authenticator command to
       communicate to us whether it's safe to retry. */
    return (system(cmdbuf) ? -1 : 0);
}

static int check_authentication _ARGUMENTS((char *, char **));

Boolean authentication_failure = False;

static int
check_authentication(command, response)
    char *command;   /* command to resend           */
    char **response; /* response from the command   */
{
  int ret;

  authentication_failure = False;
  if (STREQN(*response, "480 ", 4)) {
    if (((ret = authenticate()) < 0) ||
	((ret > 0) && ! ehErrorRetryXRN(AUTH_FAILED_RETRY_MSG, False))) {
      if (atoi(*response) != 502)
	*response = "502 Authentication failed";
      authentication_failure = True;
    }
    else if (ret > 0) {
      ServerDown = True;
      return(0);
    } else {
      put_server(command);
      *response = get_data_from_server(True);
    }
    return (1);
  }
  return(0);
}


/*
 * Check for a timeout message from the server, or for an authentication
 * request.
 */

static void check_server_response _ARGUMENTS((char *, char **));

static void check_server_response(command, response)
    char *command;   /* command to resend           */
    char **response; /* response from the command   */
{
    /*
     * try to recover from a timeout
     *
     *   this assumes that the timeout message stays the same
     *   since the error number (503) is used for more than just
     *   timeout
     *
     *   Any response with error number 503 containing the string
     *   "imeout" is treated as a timeout message.  We search for just
     *   "imeout" rather than "timeout" so that "Timeout" is also
     *   valid.  We also search for "Time Out".
     */

    if (check_authentication(command, response))
      return;

    if (ServerDown ||
	((atoi(*response) == 503) && strstr(*response, "Time Out")) ||
	((atoi(*response) == 503) && strstr(*response, "imeout"))) {

	mesgPane(XRN_SERIOUS, 0, LOST_CONNECT_ATTEMPT_RE_MSG);
	start_server();
	mesgPane(XRN_INFO, 0, RECONNECTED_MSG);

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
	*response = get_data_from_server(True);
        (void) check_authentication(command, response);
    }

    return;
}

/*
  Fetch an article from the server into the "base_file" field of the
  article structure for the article, unless the "fetch" argument is
  false, in which case it will only return success if the article has
  already previously been fetched.

  If the article was fetched successfully, returns a positive number
  and fills in the pointer to the fetched cache file structure.

  If the article was unavailable, returns 0.  If there was an error
  (e.g., disk full) fetching the article, returns a negative number.
  In both of these cases, the contents of the cache file structure
  pointer are undefined.
    
  Does not modify the data in the fetched article in any way, except
  undoing double '.' characters at line beginnings.

  The returned cache file is locked until unlocked by the caller. */
static int get_base_article _ARGUMENTS((struct newsgroup *, art_num,
					file_cache_file **, Boolean));

static int get_base_article(
			    _ANSIDECL(struct newsgroup *,	newsgroup),
			    _ANSIDECL(art_num, 			artnum),
			    _ANSIDECL(file_cache_file **,	ret_cache_file),
			    _ANSIDECL(Boolean,			fetch)
			    )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			artnum)
     _KNRDECL(file_cache_file **,	ret_cache_file)
     _KNRDECL(Boolean,			fetch)
{
  struct article *art;
  long start_time, end_time;
  char command[BUFFER_SIZE], *message, *line, *cr WALL(= NULL);
  int byteCount = 0, len;
  file_cache_file *cache_file;
  FILE *fp;
  Boolean is_partial = False, was_partial, pending_cr = False;

  art = artStructGet(newsgroup, artnum, True);

  if (art->base_file && *art->base_file) {
    file_cache_file_lock(FileCache, *art->base_file);
    *ret_cache_file = art->base_file;
    artStructSet(newsgroup, &art);
    return 1;
  }

  if (! fetch) {
    artStructSet(newsgroup, &art);
    return 0;
  }

  CLEAR_BASE_FILE(art);

  if (SETNEWSGROUP(newsgroup)) {
    artStructSet(newsgroup, &art);
    /* Should I indicate that the article is unavailable or that there
       was an error fetching it?  I'm going to opt for the former,
       although I'm not 100% convinced that's the right answer.  In
       any case, this should almost never happen. */
    return 0;
  }

  start_time = time(0);

  cache_file = (file_cache_file *) XtMalloc(sizeof(*cache_file));

  if (! (fp = file_cache_file_open(FileCache, cache_file))) {
    sprintf(error_buffer, FILE_CACHE_OPEN_MSG, file_cache_dir_get(FileCache),
	    errmsg(errno));
    FREE(cache_file);
    artStructSet(newsgroup, &art);
    if (ehErrorRetryXRN(error_buffer, True))
      return get_base_article(newsgroup, artnum, ret_cache_file, True);
  }

  do_chmod(fp, file_cache_file_name(FileCache, *cache_file), 0600);

  (void) sprintf(command, "ARTICLE %ld", artnum);
  put_server(command);
  message = get_data_from_server(True);

  check_server_response(command, &message);

  if (*message != CHAR_OK) {
    artStructSet(newsgroup, &art);
    file_cache_file_destroy(FileCache, *cache_file);
    FREE(cache_file);
    return 0;
  }

  while (1) {
    line = get_data_from_server(False);

    was_partial = is_partial;

    byteCount += (len = strlen(line));

    is_partial = (len >= MAX_BUFFER_SIZE - 1);

    if (!line[0] && ServerDown) {
      /* error */
      (void) fclose(fp);
      file_cache_file_destroy(FileCache, *cache_file);
      FREE(cache_file);
      artStructSet(newsgroup, &art);
      return 0;
    }

    if (!was_partial && line[0] == '.' && !line[1])
      /* end of the article */
      break;

    if (was_partial && pending_cr && (line[0] != '\n') &&
	(fputc('\r', fp) == EOF))
      goto disk_full;

    if ((pending_cr = (is_partial && (cr = strrchr(line, '\r')) && !cr[1])))
      *cr = '\0';

    if (!was_partial && line[0] == '.' && line[1] == '.')
      line++;

    if ((fputs(line, fp) == EOF) || (!is_partial && (fputc('\n', fp) == EOF))) {
      /* disk full? */
      while ((line = get_data_from_server(False)) &&
	     (line[0] || !ServerDown) &&
	     (was_partial || (line[0] != '.') || line[1]))
	/* empty */;
    disk_full:
      (void) fclose(fp);
    disk_full_closed:
      file_cache_file_destroy(FileCache, *cache_file);
      FREE(cache_file);
      artStructSet(newsgroup, &art);
      if (file_cache_free_space(FileCache, 1))
	return get_base_article(newsgroup, artnum, ret_cache_file, True);
      return -1;
    }
  }

  end_time = time(0);

  if (byteCount) {
    long seconds = end_time - start_time;
    double kilobytes = (double) byteCount / 1024.0;
    double speed;

    if (! seconds)
      seconds = 1;

    speed = kilobytes / (double) seconds;

    if (speed >= (double) app_resources.prefetchMinSpeed)
      FastServer = True;
    else if (kilobytes > app_resources.prefetchMinSpeed)
      FastServer = False;
  }

  if (fclose(fp) == EOF)
    goto disk_full_closed;

  if (! file_cache_file_close(FileCache, *cache_file)) {
    file_cache_file_destroy(FileCache, *cache_file);
    FREE(cache_file);
    artStructSet(newsgroup, &art);
    return -1;
  }

  *ret_cache_file = art->base_file = cache_file;
  artStructSet(newsgroup, &art);

  return 1;
}
  

/*
 * retrieve article number 'artnumber' in the current group
 *
 *   returns:  the cache file (locked) that the article is stored in
 *             or NIL(char) if the article is not avaiable
 */
file_cache_file *getarticle(newsgroup, artnumber, retposition, flags)
     struct newsgroup *newsgroup;
     art_num artnumber;  /* # of article in the current group to retrieve */
     long *retposition;     /* if non-null, return parameter for byte position of
			       header/body seperation       */
     int flags;
{
  file_cache_file *base_cache_file, *cache_file;
  int ret;
  FILE *basefp, *articlefp;
  static char *buf = NULL, *buf_base;
  static int buf_size = 0;
  char *buf_ptr, *ptr;
  int byteCount = 0, lineCount = 0, error = 0;
  Boolean found_sep = False, last_stripped = False;
  static char *field = NULL;
  static int field_size = 0;
  long position WALL(= 0);

  if (SETNEWSGROUP(newsgroup))
    return NULL;

  if ((ret = get_base_article(newsgroup, artnumber, &base_cache_file, True)) < 0) {
  cache_error:
    sprintf(error_buffer, FILE_CACHE_OPEN_MSG, file_cache_dir_get(FileCache),
	    errmsg(errno));
    if (ehErrorRetryXRN(error_buffer, True))
      return getarticle(newsgroup, artnumber, retposition, flags);
  }
  else if (ret == 0) {
    /* article is unavailable */
    return NULL;
  }

  if (! (basefp = fopen(file_cache_file_name(FileCache, *base_cache_file),
			"r"))) {
    file_cache_file_unlock(FileCache, *base_cache_file);
    goto cache_error;
  }

  cache_file = (file_cache_file *) XtMalloc(sizeof(*cache_file));

  if (! (articlefp = file_cache_file_open(FileCache, cache_file))) {
    (void) fclose(basefp);
    FREE(cache_file);
    file_cache_file_unlock(FileCache, *base_cache_file);
    goto cache_error;
  }

  do_chmod(articlefp, file_cache_file_name(FileCache, *cache_file), 0600);

  if (! buf) {
    buf_base = buf = XtMalloc(BUFFER_SIZE);
    buf_size = BUFFER_SIZE;
  }

  buf_ptr = buf;

  while (fgets(buf_ptr, buf_size - (buf_ptr - buf), basefp)) {
    if ((strlen(buf_ptr) == buf_size - (buf_ptr - buf) - 1) &&
	(buf_ptr[buf_size - (buf_ptr - buf) - 2] != '\n')) {
      buf_size *= 2;
      buf_base = buf = XtRealloc(buf, buf_size);
      buf_ptr = &buf[strlen(buf)];
      continue;
    }

    buf_ptr = buf; /* for the next time around */

    /* find header/body separation */
    if (! found_sep) {
      if (*buf == '\n') {
	position = byteCount;
	found_sep = True;
      }
    }
	      
    /* strip the headers */
    if (!found_sep && !(flags & FULL_HEADER)) {
      if ((*buf == ' ') || (*buf == '\t')) { /* continuation line */
	if (last_stripped)
	  continue;
      }
      else {
	Boolean stripping = (app_resources.headerMode == STRIP_HEADERS);

	if ((ptr = index(buf, ':')) == NIL(char))
	  continue; /* weird header line, skip */
	if (*(ptr+1) == '\0')
	  continue; /* empty field, skip */
	if (ptr - buf + 1 > field_size) {
	  field_size = ptr - buf + 1;
	  field = XtRealloc(field, field_size);
	}
	(void) strncpy(field, buf, ptr - buf);
	field[ptr - buf] = '\0';
	utDowncase(field);
	last_stripped = (avl_lookup(app_resources.headerTree, field, &ptr) ?
			 (stripping ? True : False) :
			 (stripping ? False : True));
	if (last_stripped)
	  continue;
	if (app_resources.displayLocalTime && !strcmp(field, "date"))
	  tconvert(buf+6, buf+6);
      }
    }

    /* handle rotation of the article body */
    if ((flags & ROTATED) && found_sep) {
      for (ptr = buf; *ptr != '\0'; ptr++) {
	if (((*ptr >= 'A') && (*ptr <= 'Z')) ||
	    ((*ptr >= 'a') && (*ptr <= 'z'))) {
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
    if ((flags & XLATED) && found_sep)
      /* I'm assuming here that utXlate() won't change the
	 length of the string.  If that changes, a "len =
	 strlen(buf)" line needs to be added after the
	 "utXlate(buf)" call. */
      utXlate(buf);
#endif /* XLATE */

    /* handle ^L (poorly?) */
    if (found_sep && (flags & PAGEBREAKS) && (*buf == '\f')) {
      int i, lines;
      lines = server_page_height;
      lines -= lineCount % lines;
      for (i = 0; i < lines; i++) {
	if (putc('\n', articlefp) == EOF) {
	  error++;
	  break;
	}
      }
      if (error)
	break;
      lineCount += lines;
      byteCount += lines;
      buf++;
    }

    if (found_sep && (flags & BACKSPACES)) {
      char *orig, *copy;

      for (orig = copy = buf; *orig; orig++)
	if (*orig == '\b') {
	  if (copy > buf)
	    copy--;
	}
	else
	  *copy++ = *orig;

      *copy = '\0';
    }

    if (fputs(buf, articlefp) == EOF) {
      error++;
      break;
    }

    byteCount += strlen(buf);
    lineCount++;
    buf = buf_base;
  }

  (void) fclose(basefp);

  if (!error) {
    if ((fclose(articlefp) == 0) &&
	file_cache_file_close(FileCache, *cache_file)) {
      if (retposition)
	*retposition = position;
      file_cache_file_unlock(FileCache, *base_cache_file);
      return(cache_file);
    }
  } else
    (void) fclose(articlefp);

  (void) sprintf(error_buffer, ERROR_WRITING_FILE_MSG,
		 file_cache_file_name(FileCache, *cache_file),
		 errmsg(errno));
  file_cache_file_destroy(FileCache, *cache_file);
  FREE(cache_file);
  if (file_cache_free_space(FileCache, 1) || /* free up at least one article */
      ehErrorRetryXRN(error_buffer, True))
    return getarticle(newsgroup, artnumber, retposition, flags);
  return(NULL);
}

/*
 * enter a new group and get its statistics
 *
 *   returns: NO_GROUP on failure, 0 on success
 *
 * When returning NO_GROUP, sets first, last and number to 0.
 * Only displays an error if "display_error" is True.
 */
int getgroup(
	     _ANSIDECL(struct newsgroup *,	newsgroup),
	     _ANSIDECL(art_num *,		first),
	     _ANSIDECL(art_num *,		last),
	     _ANSIDECL(int *,			number),
	     _ANSIDECL(Boolean,			display_error)
	     )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num *,		first)
     _KNRDECL(art_num *,		last)
     _KNRDECL(int *,			number)
     _KNRDECL(Boolean,			display_error)
{
    char command[BUFFER_SIZE], *message;
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
	message = get_data_from_server(True);

	check_server_response(command, &message);
    
	if (*message != CHAR_OK) {
	  int code = atoi(message);

	  if ((code == ERR_ACCESS) && ! authentication_failure) {
	    if (display_error)
	      mesgPane(XRN_SERIOUS, 0, GROUP_ACCESS_DENIED_MSG,
		       newsgroup->name);
	  }
	  else if (code == ERR_NOGROUP) {
	    if (display_error)
	      mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG,
		       newsgroup->name);
	  }
	  else {
	    char *mybuf = XtMalloc(strlen(ERROR_REQUEST_FAILED_MSG) +
				   strlen(command) +
				   strlen(message));
	    (void) sprintf(mybuf, ERROR_REQUEST_FAILED_MSG,
			   command, message);
	    ehErrorExitXRN(mybuf);
	  }

	  /* remove the group from active use ??? */

	  if (number)
	    *number = 0;
	  if (first)
	    *first = 0;
	  if (last)
	    *last = 0;

	  return(NO_GROUP);
	}

	currentNewsgroup = newsgroup;

	/* break up the message */
	count = sscanf(message, "%ld %ld %ld %ld", &code, &num, &frst, &lst);
	assert(count == 4);
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

#define NEWGROUPS_VARIABLE "newgroups_date"

char *newgroupsDate()
{
    static char date_buf[18], *message, *ptr;
    struct tm *curtime;
#if defined(__osf__) || defined(__hpux)
    time_t clock;
#else
    long clock;
#endif

    /*
      First, try the "date" command.
      */
    put_server("DATE");
    message = get_data_from_server(True);

    check_server_response("DATE", &message);

    if (*message == CHAR_INF) {
	ptr = strchr(message, ' ');
	assert(ptr);
	while (*ptr == ' ')
	    ptr++;

	/* year */
	(void) strncpy(date_buf, ptr + 2, 6);
	date_buf[6] = ' ';
	(void) strncpy(date_buf + 7, ptr + 8, 6);
	(void) strcpy(date_buf + 13, " GMT");
    } else {
	clock = time(0);
	curtime = gmtime(&clock);
	assert(curtime);
	(void) sprintf(date_buf, "%02d%02d%02d %02d%02d%02d GMT",
		       curtime->tm_year % 100, curtime->tm_mon + 1,
		       curtime->tm_mday, curtime->tm_hour, curtime->tm_min,
		       curtime->tm_sec);
    }

    return date_buf;
}

/*
  Unparse a newsgroup structure into an active file line.

  The result is returned as a pointer to a static structure.  It does
  not include a trailing newline.
  */

char *unparse_active_line(newsgroup)
struct newsgroup *newsgroup;
{
  static char *line_buf = 0;
  static int line_size = 0;
  int this_size;
  
  if (! line_buf) {
    line_size = 80; /* arbitrary starting size */
    line_buf = XtMalloc(line_size);
  }

  this_size =
    utStrlen(newsgroup->name) +	/* name */
    1 +			  	/* space after it */
    10 +			/* last article number */
    1 +			  	/* space after it */
    10 +			/* first article number */
    1 +			  	/* space after it */
    1 +			  	/* group type */
    1;				/* null */

  if (line_size < this_size) {
    line_size = this_size;
    line_buf = XtRealloc(line_buf, line_size);
  }

  (void) sprintf(line_buf, "%s %ld %ld %c", newsgroup->name,
		 newsgroup->last, newsgroup->first,
		 IS_POSTABLE(newsgroup) ?
		 (IS_MODERATED(newsgroup) ? 'm' : 'y') : 'n');

  return line_buf;
}

/*
  Return true if the specified newsgroup is supposed to be ignored,
  false otherwise.
  */
static Boolean is_ignored_newsgroup _ARGUMENTS((char *));

static Boolean is_ignored_newsgroup(group)
     char *group;
{
  static int inited = 0, ign_count, val_count;
#ifdef POSIX_REGEX
  static regex_t *ign_list, *val_list;
#else
  static char **ign_list, **val_list;
#endif
  int i;

  if (! inited) {
    val_list = parseRegexpList(app_resources.validNewsgroups,
			      "validNewsgroups", &val_count);
    ign_list = parseRegexpList(app_resources.ignoreNewsgroups,
			      "ignoreNewsgroups", &ign_count);
    inited++;
  }

  if (val_count) {
    Boolean is_valid = False;

    for (i = 0; i < val_count; i++) {
      if (
#ifdef POSIX_REGEX
	  ! regexec(&val_list[i], group, 0, 0, 0)
#else
# ifdef SYSV_REGEX
	  regex(val_list[i], group)
# else
	  ! re_comp(val_list[i]) && re_exec(group)
# endif
#endif
	  ) {
	is_valid = True;
#ifdef DEBUG
	fprintf(stderr, "is_ignored_newsgroup: %s matches valid list\n", group);
#endif
	break;
      }
    }

    if (! is_valid) 
      return True;
  }

  for (i = 0; i < ign_count; i++) {
    if (
#ifdef POSIX_REGEX
	! regexec(&ign_list[i], group, 0, 0, 0)
#else
# ifdef SYSV_REGEX
	regex(ign_list[i], group)
# else
	! re_comp(ign_list[i]) && re_exec(group)
# endif
#endif
	) {
#ifdef DEBUG
	fprintf(stderr, "is_ignored_newsgroup: %s matches ignore list\n", group);
#endif
      return True;
    }
  }

  return False;
}

  
/*
  Parse a line from an active file (NNTP LIST command output, local
  spool active file, or active-file cache).
  
  Return ACTIVE_NEW if the line is successfully parsed and caused a
  new active entry to be added, ACTIVE_OLD if it was parsed and
  represented an already-existing entry, ACTIVE_IGNORED if it's
  ignored for some unexceptional reason, ACTIVE_BOGUS if it was
  ignored because it was badly formatted, ACTIVE_NEW if a new
  newsgroup was added to the btree, and ACTIVE_OLD if an old newsgroup
  in the btree was updated.

  The value of from_cache is placed into the from_cache field of the
  newsgroup structure, if a new one is created or if the existing one
  is updated.

  If the return pointer is non-null, the newsgroup structure (if any)
  is filled into it.
  */
  
int parse_active_line(
		      _ANSIDECL(char *,			line),
		      _ANSIDECL(unsigned char,		from_cache),
		      _ANSIDECL(struct newsgroup **,	group_ptr)
		      )
     _KNRDECL(char *,			line)
     _KNRDECL(unsigned char,		from_cache)
     _KNRDECL(struct newsgroup **,	group_ptr)
{
  char *ptr, *ptr2, *group, type[BUFFER_SIZE];
  art_num first, last;
  struct newsgroup *newsgroup;
  int ret;
  /* We create the scanf_format dynamically so that we can put a field
     width into it for the "type" field. */
  static char *scanf_format = NULL;

#define SCANF_FORMAT_FORMAT " %%ld %%ld %%%ds"
  if (! scanf_format) {
    /* Boy, this is astoundingly paranoid. */
    assert(BUFFER_SIZE < 1000000000);
    scanf_format = XtMalloc(sizeof(SCANF_FORMAT_FORMAT) + 10);
    (void) sprintf(scanf_format, SCANF_FORMAT_FORMAT, BUFFER_SIZE);
    memset(type, 0, sizeof(type));
  }
#undef SCANF_FORMAT_FORMAT

  /* server returns: group last first y/m/x/j/=otherGroup */

  /* Is it really necessary to skip leading spaces?  JIK 2/19/95 */
  for ( ptr = line; *ptr == ' ';++ptr);	/* skip leading spaces */
  ptr2 = index(ptr, ' ');
  if (! ptr2)
    return ACTIVE_BOGUS;
  *ptr2 = '\0';
  group = ptr;
  type[sizeof(type)-1] = '\0';
  if (sscanf(ptr2 + 1, scanf_format, &last, &first, type) != 3)
    return ACTIVE_BOGUS;
  /* I don't know how to deal with a line that is so long that it doesn't
     fit in the type buffer.  This really should never happen! */
  assert(! type[sizeof(type)-1]);

  switch (type[0]) {
  case 'x':
  case 'j':
  case '=':
    return ACTIVE_IGNORED;
  }

#ifndef NO_BOGUS_GROUP_HACK
  /* determine if the group name is screwed up - check for jerks who
   * create group names like: alt.music.enya.puke.puke.pukeSender: 
   * - note that there is a ':' in the name of the group... */

  if (strpbrk(group, ":!, \n\t"))
    return ACTIVE_IGNORED;

#endif /* NO_BOGUS_GROUP_HACK */

  if (is_ignored_newsgroup(group))
    return ACTIVE_IGNORED;

  if (first == 0) {
    first = 1;
  }

  if (!avl_lookup(NewsGroupTable, group, &ptr)) {

    /* no entry, create a new group */
    newsgroup = ALLOC(struct newsgroup);
    newsgroup->name = XtNewString(group);
    newsgroup->newsrc = NOT_IN_NEWSRC;
    newsgroup->status = 0;
    SET_NOENTRY(newsgroup);
    SET_UNSUB(newsgroup);
    newsgroup->first = first;
    newsgroup->last = last;
    newsgroup->nglist = 0;
    newsgroup->current = 0;
    newsgroup->from_cache = from_cache;
    newsgroup->fetch = 0;
    newsgroup->thread_table = 0;
    newsgroup->kill_file = 0;
    if (art_sort_need_dates())
      newsgroup->fetch |= FETCH_DATES;
    if (art_sort_need_threads())
      newsgroup->fetch |= FETCH_IDS | FETCH_REFS | FETCH_THREADS;
    artListInit(newsgroup);

    if (avl_insert(NewsGroupTable, newsgroup->name,
		   (char *) newsgroup) < 0) {
      ehErrorExitXRN(ERROR_OUT_OF_MEM_MSG);
    }

    ActiveGroupsCount++;

    ret = ACTIVE_NEW;
  } else {
	    
    /*
     * entry exists, use it; must be a rescanning call
     *
     * just update the first and last values and adjust the
     * articles array
     */
	    
    newsgroup = (struct newsgroup *) ptr;

    /*
     * Only allow last to increase or stay the same.
     * Otherwise, we run into trouble when the NNTP server
     * returns something different in response to LIST than it
     * returns in response to GROUP for the same group.
     *
     * Note that we want to enforce the same restriction on
     * first, but we don't have to do that here since it's
     * enforced by articleArrayResync.
     */
    if (IS_SUBSCRIBED(newsgroup) && last >= newsgroup->last) {
      articleArrayResync(newsgroup, first, last, 1);
      newsgroup->from_cache = from_cache;
    }

    ret = ACTIVE_OLD;
  }

  switch (type[0]) {
  case 'y':
    SET_POSTABLE(newsgroup);
    SET_UNMODERATED(newsgroup);
    break;

  case 'm':
    SET_POSTABLE(newsgroup);
    SET_MODERATED(newsgroup);
    break;

  case 'n':
    SET_UNPOSTABLE(newsgroup);
    SET_UNMODERATED(newsgroup);
    break;

  default:
    return ACTIVE_BOGUS;
  }

  if (group_ptr)
    *group_ptr = newsgroup;
  return ret;
}


Boolean verifyGroup(
		    _ANSIDECL(char *,			group),
		    _ANSIDECL(struct newsgroup **,	struct_ptr),
		    _ANSIDECL(Boolean,			no_cache)
		    )
     _KNRDECL(char *,			group)
     _KNRDECL(struct newsgroup **,	struct_ptr)
     _KNRDECL(Boolean,			no_cache)
{
  char *ptr;
  int ret;

  if (! ((ret = avl_lookup(NewsGroupTable, group, &ptr)) || active_read ||
	 no_cache || is_ignored_newsgroup(group))) {
    mesgPane(XRN_SERIOUS, 0, MISSING_NG_LISTING_MSG, group);
    getactive(False);
    ret = avl_lookup(NewsGroupTable, group, &ptr);
  }
  if (ret) {
    if (struct_ptr)
      *struct_ptr = (struct newsgroup *) ptr;
    return True;
  }
  else
    return False;
}

int active_read = False;

/*
 * get a list of all active newsgroups and create a structure for each one
 *
 *   returns: void
 */
void getactive(
	       _ANSIDECL(Boolean,	do_newgroups)
	       )
     _KNRDECL(Boolean,	do_newgroups)
{
    char command[BUFFER_SIZE], *message;
    char *newgroups_str = 0, *new_newgroups_str = 0;
    char *newgroups_var = NEWGROUPS_VARIABLE;
    char buf[LABEL_SIZE];
    struct newsgroup *newsgroup;
    int ret;

    if (do_newgroups) {
	newgroups_str = var_get_value(cache_variables, newgroups_var);
	new_newgroups_str = newgroupsDate();
	if (! newgroups_str) {
	    var_set_value(&cache_variables, newgroups_var, new_newgroups_str);
	    return;
	}
	(void) strcpy(buf, GETTING_NEWGROUPS_MSG);
    }
    else
	(void) strcpy(buf, GETTING_LIST_MSG);

    infoNow(buf);

    cancelPrefetch();
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
    if (do_newgroups) {
	(void) sprintf(command, "NEWGROUPS %s", newgroups_str);
	XtFree(newgroups_str);
    } else
	(void) strcpy(command, "LIST");
    put_server(command);
    message = get_data_from_server(True);

    check_server_response(command, &message);
    
    if (*message != CHAR_OK) {
      char *mybuf = XtMalloc(strlen(ERROR_REQUEST_FAILED_MSG) +
			     strlen(command) + strlen(message));
      (void) sprintf(mybuf, ERROR_REQUEST_FAILED_MSG, command, message);
      ehErrorExitXRN(mybuf);
    }

    for (;;) {
        message = get_data_from_server(True);
	
	/* the list is ended by a '.' at the beginning of a line */
	if (message[0] == '.' && ! message[1]) {
	    break;
	}

	switch (ret = parse_active_line(message, FALSE, &newsgroup)) {
	case ACTIVE_IGNORED:
	  break;
	case ACTIVE_BOGUS:
	  mesgPane(XRN_SERIOUS, 0, BOGUS_ACTIVE_ENTRY_MSG, message);
	  break;
	case ACTIVE_NEW:
	  if (app_resources.fullNewsrc || do_newgroups)
	    SET_NEW(newsgroup);
	  break;
	case ACTIVE_OLD:
	  if (do_newgroups && IS_NOENTRY(newsgroup))
	    SET_NEW(newsgroup);
	  break;
	default:
	  mesgPane(XRN_SERIOUS, 0, UNKNOWN_FUNC_RESPONSE_MSG, ret,
		   "parse_active_line", "getactive");
	  break;
	}
    }

    if (do_newgroups)
	var_set_value(&cache_variables, newgroups_var, new_newgroups_str);
    else
      active_read++;

    CHECKNEWSRCSIZE(ActiveGroupsCount);
#ifndef FIXED_ACTIVE_FILE
    badActiveFileCheck();
#endif

    (void) strcat(buf, " ");
    (void) strcat(buf, DONE_MSG);
    INFO(buf);
    return;
}

#ifndef FIXED_ACTIVE_FILE
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
	 ehErrorExitXRN(ERROR_OUT_OF_MEM_MSG);
    }

    while (avl_gen(gen, &key, &value)) {
	struct newsgroup *newsgroup = (struct newsgroup *) value;

	if (IS_SUBSCRIBED(newsgroup) &&
	    (newsgroup->first == newsgroup->last) &&
	    (newsgroup->first != 0)) {

	    if (! (getgroup(newsgroup, 0, 0, &number, True) || number)) {
		articleArrayResync(newsgroup, newsgroup->first, newsgroup->last, number);
	    }
	}
    }
    avl_free_gen(gen);

    return;
}
#endif

/*
 * initiate a connection to the news server
 *
 * the server eventually used is remembered, so if this function is called
 * again (for restarting after a timeout), it will use it.
 *
 *   returns: void
 *
 */
void start_server()
{
    static char *server = NIL(char);   /* for restarting */
    int response;
    char buf[LABEL_SIZE+HOST_NAME_SIZE];

    /* Make sure to close a previous server connection, e.g., to avoid
       file descriptor leaks. */
    close_server();

    if (! server) {
      server = nntpServer();
      nntp_port = app_resources.nntpPort;
    }

    if (! server)
	ehErrorExitXRN(NO_SERVER_MSG);

    (void) sprintf(buf, CONNECTING_MSG, server);
    infoNow(buf);

    while (1) {
      response = server_init(server);

      if (response == OK_NOPOST)
	PostingAllowed = False;
      else if (response >= 0)
	response = handle_server_response(response, server);
      if ((response >= 0) &&
	  !(app_resources.authenticateOnConnect && authenticate()))
	break;
      stop_server();
      (void) sprintf(buf, FAILED_CONNECT_MSG, server);
      if (! ehErrorRetryXRN(buf, False)) {
	while (! updatenewsrc())
	  (void) ehErrorRetryXRN(ERROR_CANT_UPDATE_NEWSRC_MSG, True);
	ehErrorExitXRN(0);
      }
    }

    (void) sprintf(buf, CONNECTING_MSG, server);
    (void) strcat(buf, " ");
    (void) strcat(buf, DONE_MSG);
    INFO(buf);

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


/* Which header fields can't be fetched with XHDR? */
#ifdef NO_XHDR_NEWSGROUPS
static fetch_flag_t no_xhdr_fields = FETCH_NEWSGROUPS;
#else
static fetch_flag_t no_xhdr_fields = 0;
#endif

/* Which header fields *can* be fetched with XHDR? */
static fetch_flag_t xhdr_fields = 0;


/*
 * Get a list of lines for a particular field for the specified group in
 * the range 'first' to 'last'.
 *
 * "fixfunction" is a function which takes a newsgroup, an article
 * number and a string and returns a new, allocated string to actually
 * assign to the field.
 *
 * "field_bit", is the prefetch field bit, if any, for this field.  It
 * is used to keep track of which fields the server can't handle XHDR
 * requests for.
 * 
 * "offset" is the offset of the field's pointer in a article
 * structure.
 * 
 * Returns: True if it's done, False to keep going.
 */
typedef char * (*_fixfunction) _ARGUMENTS((struct newsgroup *, art_num, char *));

static Boolean getlist _ARGUMENTS((struct newsgroup *, art_num, art_num,
				   Boolean, int *, char *, fetch_flag_t, unsigned,
				   _fixfunction, unsigned, Boolean));

static Boolean getlist(
		       _ANSIDECL(struct newsgroup *,	newsgroup),
		       _ANSIDECL(art_num,		artfirst),
		       _ANSIDECL(art_num,		artlast),
		       _ANSIDECL(Boolean,		unreadonly),
		       _ANSIDECL(int *,			max),
		       _ANSIDECL(char *,		field),
		       _ANSIDECL(fetch_flag_t,		field_bit),
		       _ANSIDECL(unsigned,		offset),
		       _ANSIDECL(_fixfunction, 		fixfunction),
		       _ANSIDECL(unsigned,		fixed_offset),
		       _ANSIDECL(Boolean,		required)
		       )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			artfirst)
     _KNRDECL(art_num,			artlast)
     _KNRDECL(Boolean,			unreadonly)
     _KNRDECL(int *,			max)
     _KNRDECL(char *,			field)
     _KNRDECL(fetch_flag_t,		field_bit)
     _KNRDECL(unsigned,			offset)
     _KNRDECL(_fixfunction, 		fixfunction)
     _KNRDECL(unsigned,			fixed_offset)
     _KNRDECL(Boolean,			required)
{
  char command[BUFFER_SIZE], *message;
  char *line, *ptr;
  art_num number;
  art_num first, last;
  int count = 0;
  struct article *art;
  int pane_name = newMesgPaneName();
  Boolean trying_again = False;

  if (SETNEWSGROUP(newsgroup))
    return True;
  artListSet(newsgroup);

  first = artfirst;
  while ((first <= artlast) && ((! max) || (count < *max))) {
    art_num sub_count;

    art = artStructGet(newsgroup, first, False);
    if (((offset != (unsigned)-1) && *(char **)((char *) art + offset)) ||
	(fixfunction && *(char **)((char *) art + fixed_offset)) ||
	(unreadonly && IS_READ(art)) ||
	IS_UNAVAIL(art)) {
      first++;
      ART_STRUCT_UNLOCK;
      continue;
    }
    ART_STRUCT_UNLOCK;

    for (sub_count = 1, last = first + 1; last <= artlast; last++) {
      art = artStructGet(newsgroup, last, False);
      if (((offset != (unsigned)-1) && *(char **)((char *) art + offset)) ||
	  (fixfunction && *(char **)((char *) art + fixed_offset)) ||
	  (unreadonly && IS_READ(art)) ||
	  (max && ((count + sub_count) >= *max))) {
	ART_STRUCT_UNLOCK;
	break;
      }
      if (max && IS_AVAIL(art))
	sub_count++;
      ART_STRUCT_UNLOCK;
    }
    last--;

  try_again:
    if (trying_again || (newsgroup->fetch & no_xhdr_fields)) {
      for (number = first; number <= last; number++) {
	art = artStructGet(newsgroup, number, False);
	if (IS_UNAVAIL(art) || (unreadonly && IS_READ(art))) {
	  ART_STRUCT_UNLOCK;
	  continue;
	}
	ART_STRUCT_UNLOCK;
	get_article_headers(newsgroup, number);
	art = artStructGet(newsgroup, number, True);
	if (art->headers && avl_lookup(art->headers, field, &line)) {
	  if (trying_again)
	    no_xhdr_fields |= field_bit;
	  if (offset != (unsigned)-1)
	    *(char **)((char *)art + offset) =
	      XtNewString(line);
	  if (fixfunction)
	    *(char **)((char *) art + fixed_offset) =
	      (*fixfunction)(newsgroup, number, line);
	}
	artStructSet(newsgroup, &art);
      }
    }
    else {
      (void) sprintf(command, "XHDR %s %ld-%ld", field, first, last);
      put_server(command);
      message = get_data_from_server(True);

      check_server_response(command, &message);

      /* check for errors */
      if (*message != CHAR_OK) {
	if (field_bit) {
	  no_xhdr_fields |= field_bit;
	  goto try_again;
	}
	mesgPane(XRN_SERIOUS, pane_name, XHDR_ERROR_MSG);
	return True;
      }

      for(;;) {
	message = get_data_from_server(True);
	if (*message == '.') {
	  break;
	}

	/*
	  The brilliant folks at Microsoft have decided that it's OK for
	  XHDR to return multi-line header fields as multiple lines in
	  the XHDR response, even though that's never been done by any
	  other NNTP server and it's different from what XOVER does.
	  It's really annoying how they invent standards like this.  For
	  the time being, I'm going to assume that someone is going to
	  show them the error of their ways and get them to fix their
	  server, so I'm going to ignore the bogus output for now
	  instead of actually trying to handle it.  If they manage to
	  browbeat the world into doing things their way, as they so
	  often do, I'll consider at some point in the future adding
	  support for multi-line XHDR field response.  Grr.
	  - jik 12/16/97
	*/
	if (isspace((unsigned char)*message))
	  continue;

	count++;

	/*
	 * message is of the form:
	 *
	 *    Number value
	 *
	 * must get the number since not all articles will be returned
	 */

	number = atol(message);
	line = index(message, ' ');
	if (! (number && line)) {
	  mesgPane(XRN_SERIOUS, pane_name, MALFORMED_XHDR_RESPONSE_MSG,
		   command, message);
	  /* Let's hope that even though the server is sending us bogus
	     data, it'll eventually sent a correct ".\r\n" line to
	     terminate the output of the command. */
	  continue;
	}

	/*
	  Strip leading and trailing spaces.
	*/
	while (*line && isspace((unsigned char)*line))
	  line++;
	for (ptr = strchr(line, '\0') - 1; (ptr >= line) &&
	       isspace((unsigned char)*ptr);
	     *ptr-- = '\0') /* empty */;
	if (!required && (strcmp(line, "(none)") == 0))
	  continue;

	xhdr_fields |= field_bit;
	art = artStructGet(newsgroup, number, True);
	if (offset != (unsigned)-1)
	  *(char **)((char *)art + offset) =
	    XtNewString(line);
	if (fixfunction)
	  *(char **)((char *) art + fixed_offset) =
	    (*fixfunction)(newsgroup, number, line);
	artStructSet(newsgroup, &art);
      }

      /* This is really hideous.  The problem is that with some NNTP
	 server implementations, if you send an XHDR request for a
	 field for which they don't support XHDR, instead of giving
	 you an error code indicating that they don't support XHDR,
	 instead they just giveyou a list of article numbers with
	 (none) for each of them.  Therefore, if we've just tried to
	 retrieve a particular header from a set of articles and we
	 didn't find that header in *any* of them, there's a chance
	 that the problem is actually that the NNTP server doesn't
	 support XHDR on that header, rather than that the header
	 doesn't exist in the articles.  So we need to try using HEAD
	 instead of XHDR.  If we succeed, then the bit gets added to
	 no_xhdr_fields and we'll know to do it automatically in the
	 future. */
      if (field_bit && ! (xhdr_fields & field_bit)) {
	trying_again = True;
	goto try_again;
      }
    }

    for (number = first; number <= last; number++) {
      struct article copy;
      Boolean changed = False;

      art = artStructGet(newsgroup, number, False);
      copy = *art;
      if ((offset != (unsigned)-1) && ! *(char **)((char *) art + offset)) {
	if (required)
	  goto unavail;
	else {
	  *(char **)((char *)&copy + offset) = XtNewString("");
	  changed = True;
	}
      }
      if (fixfunction && ! *(char **)((char *)art + fixed_offset)) {
	if (required)
	  goto unavail;
	else {
	  *(char **)((char *)&copy + fixed_offset) = XtNewString("");
	  changed = True;
	}
      }
      if (changed)
	artStructReplace(newsgroup, &art, &copy, number);
      else
	ART_STRUCT_UNLOCK;
      continue;
    unavail:
      CLEAR_ALL_NO_FREE(&copy);
      SET_UNAVAIL(&copy);
      artStructReplace(newsgroup, &art, &copy, number);
    }
    first = last + 1;

    if (max)
      break;
  }
  if (first > artlast) {
    return True;
  }
  else {
    *max = count;
    return False;
  }
}

Boolean getsubjectlist(
		       _ANSIDECL(struct newsgroup *,	newsgroup),
		       _ANSIDECL(art_num,		artfirst),
		       _ANSIDECL(art_num,		artlast),
		       _ANSIDECL(Boolean,		unreadonly),
		       _ANSIDECL(int *,			max)
		       )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			artfirst)
     _KNRDECL(art_num,			artlast)
     _KNRDECL(Boolean,			unreadonly)
     _KNRDECL(int *,			max)
{
    struct article foo;
    unsigned offset;

    offset = (char *)&foo.subject - (char *)&foo;

    return getlist(newsgroup, artfirst, artlast, unreadonly, max,
		   "subject", 0, offset, 0, 0, True);
}

Boolean getnewsgroupslist(
			  _ANSIDECL(struct newsgroup *,	newsgroup),
			  _ANSIDECL(art_num,		artfirst),
			  _ANSIDECL(art_num,		artlast),
			  _ANSIDECL(Boolean,		unreadonly),
			  _ANSIDECL(int *,		max)
			  )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			artfirst)
     _KNRDECL(art_num,			artlast)
     _KNRDECL(Boolean,			unreadonly)
     _KNRDECL(int *,			max)
{
    struct article foo;
    unsigned offset;

    offset = (char *)&foo.newsgroups - (char *)&foo;

    return getlist(newsgroup, artfirst, artlast, unreadonly, max,
		   "newsgroups", FETCH_NEWSGROUPS, offset, 0, 0, True);
}

Boolean getxreflist(
		    _ANSIDECL(struct newsgroup *,	newsgroup),
		    _ANSIDECL(art_num,			artfirst),
		    _ANSIDECL(art_num,			artlast),
		    _ANSIDECL(Boolean,			unreadonly),
		    _ANSIDECL(int *,			max)
		    )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			artfirst)
     _KNRDECL(art_num,			artlast)
     _KNRDECL(Boolean,			unreadonly)
     _KNRDECL(int *,			max)
{
    struct article foo;
    unsigned offset;

    offset = (char *)&foo.xref - (char *)&foo;

    return getlist(newsgroup, artfirst, artlast, unreadonly, max,
		   "xref", FETCH_XREF, offset, 0, 0, False);
}

Boolean getapprovedlist(
		    _ANSIDECL(struct newsgroup *,	newsgroup),
		    _ANSIDECL(art_num,			artfirst),
		    _ANSIDECL(art_num,			artlast),
		    _ANSIDECL(Boolean,			unreadonly),
		    _ANSIDECL(int *,			max)
		    )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			artfirst)
     _KNRDECL(art_num,			artlast)
     _KNRDECL(Boolean,			unreadonly)
     _KNRDECL(int *,			max)
{
    struct article foo;
    unsigned offset;

    offset = (char *)&foo.approved - (char *)&foo;

    return getlist(newsgroup, artfirst, artlast, unreadonly, max,
		   "approved", FETCH_APPROVED, offset, 0, 0, False);
}

Boolean getdatelist(
		    _ANSIDECL(struct newsgroup *,	newsgroup),
		    _ANSIDECL(art_num,			artfirst),
		    _ANSIDECL(art_num,			artlast),
		    _ANSIDECL(Boolean,			unreadonly),
		    _ANSIDECL(int *,			max)
		    )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			artfirst)
     _KNRDECL(art_num,			artlast)
     _KNRDECL(Boolean,			unreadonly)
     _KNRDECL(int *,			max)
{
    struct article foo;
    unsigned offset;

    offset = (char *)&foo.date - (char *)&foo;

    return getlist(newsgroup, artfirst, artlast, unreadonly, max,
		   "date", FETCH_DATES, offset, 0, 0, True);
}

Boolean getidlist(
		  _ANSIDECL(struct newsgroup *,	newsgroup),
		  _ANSIDECL(art_num,		artfirst),
		  _ANSIDECL(art_num,		artlast),
		  _ANSIDECL(Boolean,		unreadonly),
		  _ANSIDECL(int *,		max)
		  )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			artfirst)
     _KNRDECL(art_num,			artlast)
     _KNRDECL(Boolean,			unreadonly)
     _KNRDECL(int *,			max)
{
    struct article foo;
    unsigned offset;

    offset = (char *)&foo.id - (char *)&foo;

    return getlist(newsgroup, artfirst, artlast, unreadonly, max,
		   "message-id", FETCH_IDS, offset, 0, 0, True);
}

Boolean getreflist(
		   _ANSIDECL(struct newsgroup *,	newsgroup),
		   _ANSIDECL(art_num,			artfirst),
		   _ANSIDECL(art_num,			artlast),
		   _ANSIDECL(Boolean,			unreadonly),
		   _ANSIDECL(int *,			max)
		   )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			artfirst)
     _KNRDECL(art_num,			artlast)
     _KNRDECL(Boolean,			unreadonly)
     _KNRDECL(int *,			max)
{
    struct article foo;
    unsigned offset;

    offset = (char *)&foo.references - (char *)&foo;

    return getlist(newsgroup, artfirst, artlast, unreadonly, max,
		   "references", FETCH_REFS, offset, 0, 0, False);
}

static char *authorFixFunction(newsgroup, artnum, message)
    struct newsgroup *newsgroup;
    art_num artnum;
    char *message;
{
    char *author = message, *brackbeg, *brackend, *end;
    char authbuf[BUFFER_SIZE];

    (void) strcpy(authbuf, author);

    if (app_resources.authorFullName) {
	/* Can be made fancyer at the expence of extra cpu time */

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
		while ((brackbeg > message) && (*--brackbeg == ' '))
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
	if ((author = index(author, '<')) == NIL(char)) {
	    author = message;
	    /* first form */
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
    if (! *author)
	author = authbuf;
    return XtNewString(author);
}

Boolean getauthorlist(
		      _ANSIDECL(struct newsgroup *,	newsgroup),
		      _ANSIDECL(art_num,		artfirst),
		      _ANSIDECL(art_num,		artlast),
		      _ANSIDECL(Boolean,		unreadonly),
		      _ANSIDECL(int *,			max)
		      )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			artfirst)
     _KNRDECL(art_num,			artlast)
     _KNRDECL(Boolean,			unreadonly)
     _KNRDECL(int *,			max)
{
    struct article foo;
    unsigned offset, fixed_offset;

    offset = (char *)&foo.from - (char *)&foo;
    fixed_offset = (char *)&foo.author - (char *)&foo;

    return getlist(newsgroup, artfirst, artlast, unreadonly, max,
		   "from", 0, offset, authorFixFunction, fixed_offset, True);
}


static char *linesFixFunction(newsgroup, artnum, numoflines)
    struct newsgroup *newsgroup;
    art_num artnum;
    char *numoflines;
{
    char *buf;

    if (numoflines[0] != '(') {
      buf = XtMalloc(strlen(numoflines) + 3);
      (void) sprintf(buf, "[%s]", numoflines);
    } else {
      buf = XtNewString(numoflines);
      *buf = '[';
      *(strchr(buf, '\0')-1) = ']';
    }
    if (strcmp(buf, "[none]") == 0) {
	(void) strcpy(buf, "[?]");
    }
    return buf;
}

Boolean getlineslist(
		     _ANSIDECL(struct newsgroup *,	newsgroup),
		     _ANSIDECL(art_num,			artfirst),
		     _ANSIDECL(art_num,			artlast),
		     _ANSIDECL(Boolean,			unreadonly),
		     _ANSIDECL(int *,			max)
		     )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			artfirst)
     _KNRDECL(art_num,			artlast)
     _KNRDECL(Boolean,			unreadonly)
     _KNRDECL(int *,			max)
{
    struct article foo;
    unsigned offset;

    if (! app_resources.displayLineCount)
	return True;

    offset = (char *)&foo.lines - (char *)&foo;

    return getlist(newsgroup, artfirst, artlast, unreadonly, max,
		   "lines", 0, (unsigned)-1, linesFixFunction, offset, False);
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
     char **lines = NULL, *this_line;
     unsigned int num_lines = 0;
     int breakAt = app_resources.breakLength;
     int maxLength;

     maxLength = app_resources.lineLength;
     if (app_resources.breakLength > maxLength) {
       maxLength = app_resources.breakLength;
     }

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



/*
  The article string passed into this function *must* have both a
  header and a body, separated by at least one blank line.
*/
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
    char command[BUFFER_SIZE], *message;
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
    tempfile = utTempnam(app_resources.tmpDir, "xrn");
    (void) sprintf(buffer, "%s -h > %s 2>&1",INEWS, tempfile);
    if ((inews = xrn_popen(buffer, "w")) == NULL) {
	mesgPane(XRN_SERIOUS, 0, CANT_EXECUTE_CMD_POPEN_MSG, buffer);
	(void) unlink(tempfile);
	utTempnamFree(tempfile);
	return POST_FAILED;
    }
#else

    (void) strcpy(command, "POST");
    put_server(command);
    message = get_data_from_server(True);

    check_server_response(command, &message);

    if (*message != CHAR_CONT) {
	mesgPane(XRN_SERIOUS, 0, NNTP_ERROR_MSG, message);
	if (atoi(message) == ERR_NOPOST) {
	    return(POST_NOTALLOWED);
	} else {
	    return(POST_FAILED);
	}
    }
#endif

    ptr = article;

    while (ptr) {
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

    /* If this assertion fails, then the article string passed into
       this function didn't have a body.  Shame on you! */
    assert(ptr);

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

	(void) sprintf(temp, "\n\ninews exit value: %d\n", exitstatus);
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
	utTempnamFree(tempfile);
	return(POST_FAILED);
    }

    (void) unlink(tempfile);
    utTempnamFree(tempfile);
#else
    put_server(".");

    message = get_data_from_server(True);

    if (*message != CHAR_OK) {
	*ErrMsg = XtMalloc(strlen(SERVER_POSTING_ERROR_MSG) + strlen(message));
	(void) sprintf(*ErrMsg, SERVER_POSTING_ERROR_MSG, message);
	return(POST_FAILED);
    }
#endif

    return(POST_OKAY);
}

/*
 * get XHDR information for a single article
 */
static char *xhdr_single _ARGUMENTS((struct newsgroup *, char *, long *, Boolean));

static char *xhdr_single(
			 _ANSIDECL(struct newsgroup *,	newsgroup),
			 _ANSIDECL(char *,		buffer),
			 _ANSIDECL(long *,		error_code),
			 _ANSIDECL(Boolean,		silent_error)
			 )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(char *,		buffer)
     _KNRDECL(long *,		error_code)
     _KNRDECL(Boolean,		silent_error)
{
    char *message, *ptr;

    *error_code = 0;

    if (SETNEWSGROUP(newsgroup))
      return NULL;

    put_server(buffer);
    message = get_data_from_server(True);
    
    check_server_response(buffer, &message);
    
    /* check for errors */
    if (*message != CHAR_OK) {
      if ((*error_code = atol(message)) != ERR_NOART) {
	if (! silent_error) {
	  fprintf(stderr, "NNTP error: %s\n", message);
	  mesgPane(XRN_SERIOUS, 0, XHDR_ERROR_MSG);
	}
      }
      return NULL;
    }

    message = get_data_from_server(True);

    /* no information */
    if (*message == '.') {
	return NULL;
    }

    ptr = index(message, ' ');

    /* malformed entry */
    if (ptr == NIL(char)) {
	mesgPane(XRN_SERIOUS, 0, MALFORMED_XHDR_RESPONSE_MSG, buffer, message);
	message = get_data_from_server(True);
	return NULL;
    }

    ptr++;

    /* no information */
    if (STREQ(ptr, "(none)")) {
	/* ending '.' */
	do {
	  message = get_data_from_server(True);
	} while (*message != '.');
	return NULL;
    }

    ptr = XtNewString(ptr);

    /* ending '.' */
    do {
      message = get_data_from_server(True);
    } while (*message != '.');

    return ptr;
}

/*
  If this hasn't already been done:

  Use the HEAD command to retrieve the header of the specified
  article.  Create a new AVL tree to store the parsed headers.  The
  keys in the tree are the header field names, turned to lower case,
  and the values in the tree are the values of the matching header
  lines, with newlines converted into spaces and the final newline
  omitted.  ONLY THE FIRST HEADER FIELD WITH EACH FIELD NAME IS
  STORED, e.g., if there are two "X-Foo:" headers in the message, only
  the first one is stored and retrieved.
*/
  
static void get_article_headers(newsgroup, article)
     struct newsgroup *newsgroup;
     art_num article;
{
  struct article *art;
  char buffer[BUFFER_SIZE], *message, *header;
  int header_len, header_size;
  file_cache_file *cache_file;
  FILE *fp;
  avl_tree *headers;

  art = artStructGet(newsgroup, article, False);
  ART_STRUCT_UNLOCK;
  if (art->headers)
    return;

  art = artStructGet(newsgroup, article, True);
  art->headers = headers = avl_init_table(strcmp);

  header_size = 80;
  header = XtMalloc(header_size);
  header_len = 0;

  artStructSet(newsgroup, &art);

  if (get_base_article(newsgroup, article, &cache_file, False) > 0) {
    Boolean sawnl = False;

    if (! (fp = fopen(file_cache_file_name(FileCache, *cache_file), "r"))) {
      file_cache_file_unlock(FileCache, *cache_file);
      return;
    }

    for (;;) {
      int len;

      if (header_size - header_len < 2) {
	header_size *= 2;
	header = XtRealloc(header, header_size);
      }

      if (! fgets(header + header_len, header_size - header_len, fp))
	break;

      len = strlen(header + header_len);
      header_len += len;

      if (header[header_len-1] != '\n') {
	sawnl = False;
	continue;
      }

      if (sawnl && (len == 1))
	break;
      sawnl = True;
    }

    (void) fclose(fp);
    file_cache_file_unlock(FileCache, *cache_file);
  }
  else {
    if (SETNEWSGROUP(newsgroup)) {
      return;
    }

    (void) sprintf(buffer, "HEAD %ld", article);
    put_server(buffer);
    message = get_data_from_server(True);

    check_server_response(buffer, &message);

    if (*message != CHAR_OK) {
      /* can't get header */
      return;
    }

    for (;;) {
      int len;
      message = get_data_from_server(True);
      if (! (*message && strcmp(message, "."))) {
	break;
      }
      if (*message == '.')
	message++;
      len = strlen(message);
      while (header_len + len + 2 > header_size) {
	header_size *= 2;
	header = XtRealloc(header, header_size);
      }
      (void) strcpy(header + header_len, message);
      *(header + header_len + len) = '\n';
      *(header + header_len + len + 1) = '\0';
      header_len += len + 1;
    }
  }
  
  header = XtRealloc(header, header_len + 1);

  avl_insert(headers, ART_HEADERS_BASE, header);

  while (*header) {
    char *ptr, *name, *value;

    if (! (value = strchr(name = header, ':')))
      break;

    for (ptr = name; ptr < value; ptr++)
      if (isupper(*ptr))
	*ptr = tolower(*ptr);

    *value++ = '\0';

    for (ptr = strchr(value, '\n'); ptr; ptr = strchr(ptr, '\n')) {
      if (ptr[1] != ' ' && ptr[1] != '\t') {
	*ptr = '\0';
	header = ptr + 1;
	if (! avl_lookup(headers, name, 0)) {
	  while (*value == ' ' || *value == '\t')
	    value++;
	  while (--ptr >= value && (*ptr == ' ' || *ptr == '\t'))
	    *ptr = '\0';
	  avl_insert(headers, name, value);
	}
	break;
      }
      *ptr = ' ';
    }
  }
}

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
  struct article *art;
  char *value;

  get_article_headers(newsgroup, article);

  art = artStructGet(newsgroup, article, False);
  ART_STRUCT_UNLOCK;
  if (art->headers && avl_lookup(art->headers, field, &value))
    *string = XtNewString(value);
  else
    *string = 0;
}

/* We're not currently using XHDR to retrieve information about single
   articles, for two reasons: (1) whenever we're doing that, we're
   probably going to need multiple fields for the same article; (2) in
   fact we've probably already retrieved the article, and parsing its
   header fields is cheaper than a round trip to the server; (3) many
   so-called "modern" NNTP servers don't support XHDR for all header
   fields; they only return valid XHDR data for fields that are in the
   overview database.  This sucks, but we have to cope with it
   anyway.

   However, we're preserving this functionality here, in case we end
   up needing to use it later, rather than removing it completely.  If
   we do end up using this functionality at any point in the future,
   the function needs to be updated so that it knows how to fall back
   on checking the header if the XHDR command fails.
*/

#if 0

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
    char buffer[BUFFER_SIZE];
    long error_code;

    (void) sprintf(buffer, "XHDR %s %ld", field, article);
    *string = xhdr_single(newsgroup, buffer, &error_code, False);
    return;
}

#endif

/* Get XHDR information using an article's message ID.  The returned
   data is allocated and should be freed by the caller.  Fails
   silently if the NNTP server doesn't support XHDR by message ID. */
  
char *xhdr_id(newsgroup, id, field, error_code)
     struct newsgroup *newsgroup;
     char *id;
     char *field;
     long *error_code;
{
  char buffer[BUFFER_SIZE];

  (void) sprintf(buffer, "XHDR %s %s", field, id);
  return xhdr_single(newsgroup, buffer, error_code, True);
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
	fprintf(stderr, ERROR_EXEC_FAILED_MSG, command);
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
