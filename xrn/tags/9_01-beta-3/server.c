
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: server.c,v 1.149 1998-03-23 01:51:48 jik Exp $";
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
     
  while (*ptr && isspace(*ptr))
    ptr++;

  return(! strncasecmp(ptr, "user/pass", sizeof("user/pass")-1));
}

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

    while (*user && isspace(*user))
      user++;

    user += sizeof("user/pass") - 1;

    while (*user && isspace(*user))
      user++;

    if (! (ptr = strchr(user, '/')))
      ptr = strchr(user, '\0');

    while ((ptr > user) && isspace(*(ptr - 1)))
      ptr--;

    *ptr = '\0';

#if ALLOW_RESOURCE_PASSWORDS
    pass = app_resources.authenticator;
    if ((pass = strchr(pass, '/')) && (pass = strchr(pass + 1, '/'))) {
      pass++;
      while (*pass && isspace(*pass))
	pass++;
      ptr = strchr(pass, '\0');
      while ((ptr > pass) && isspace(*(ptr - 1)))
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
    pass = PasswordBox(TopLevel, NNTP_PASSWORD_MSG);
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
    return (system(cmdbuf));
}

static int check_authentication _ARGUMENTS((char *, char **));

static int
check_authentication(command, response)
    char *command;   /* command to resend           */
    char **response; /* response from the command   */
{
  if (STREQN(*response, "480 ", 4)) {
    if (authenticate()) {
      *response = "502 Authentication failed";
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
     *   Message is:
     *     503 Timeout ...
     */

    if (check_authentication(command, response))
      return;

    if (ServerDown || STREQN(*response, "503 Timeout", 11)) {

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
  article structure for the article.

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
					file_cache_file **));

static int get_base_article(newsgroup, artnum, ret_cache_file)
     struct newsgroup *newsgroup;
     art_num artnum;
     file_cache_file **ret_cache_file;
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
    return 1;
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

  (void) sprintf(command, "ARTICLE %ld", artnum);
  put_server(command);
  message = get_data_from_server(True);

  check_server_response(command, &message);

  if (*message != CHAR_OK) {
    artStructSet(newsgroup, &art);
    return 0;
  }

  cache_file = (file_cache_file *) XtMalloc(sizeof(*cache_file));

  if (! (fp = file_cache_file_open(FileCache, cache_file))) {
    sprintf(error_buffer, FILE_CACHE_OPEN_MSG, file_cache_dir_get(FileCache),
	    errmsg(errno));
    FREE(cache_file);
    artStructSet(newsgroup, &art);
    if (ehErrorRetryXRN(error_buffer, True))
      return get_base_article(newsgroup, artnum, ret_cache_file);
  }

  do_chmod(fp, file_cache_file_name(FileCache, *cache_file), 0600);

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
	return get_base_article(newsgroup, artnum, ret_cache_file);
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
 *
 * MODIFIES THE ARTICLE STRUCTURE FOR THE ARTICLE BEING RETRIEVED.
 * Therefore, the caller should not have the article structure
 * allocated (i.e., returned by artStructGet()) and expect the
 * allocated structure to be valid after the call to getarticle().  If
 * the caller *does* have the structure allocated, he should
 * re-retrieve it after calling getarticle().
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
  static char *buf = NULL;
  static int buf_size = 0;
  char *buf_ptr, *ptr;
  int byteCount = 0, lineCount = 0, error = 0;
  Boolean found_sep = False, last_stripped = False;
  static char *field = NULL;
  static int field_size = 0;
  long position WALL(= 0);

  if (SETNEWSGROUP(newsgroup))
    return NULL;

  if ((ret = get_base_article(newsgroup, artnumber, &base_cache_file)) < 0) {
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
    buf = XtMalloc(BUFFER_SIZE);
    buf_size = BUFFER_SIZE;
  }

  buf_ptr = buf;

  while (fgets(buf_ptr, buf_size - (buf_ptr - buf), basefp)) {
    if ((strlen(buf_ptr) == buf_size - (buf_ptr - buf) - 1) &&
	(buf_ptr[buf_size - (buf_ptr - buf) - 2] != '\n')) {
      buf_size *= 2;
      buf = XtRealloc(buf, buf_size);
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
    if (found_sep && (flags & PAGEBREAKS) && (*buf == '\014')) {
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
	    if (atoi(message) != ERR_NOGROUP) {
	      char *mybuf = XtMalloc(strlen(ERROR_REQUEST_FAILED_MSG) +
				     strlen(command) +
				     strlen(message));
	      (void) sprintf(mybuf, ERROR_REQUEST_FAILED_MSG,
			     command, message);
	      ehErrorExitXRN(mybuf);
	    }
	    if (display_error)
		mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG,
			 newsgroup->name);

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
		       curtime->tm_year, curtime->tm_mon + 1, curtime->tm_mday,
		       curtime->tm_hour, curtime->tm_min, curtime->tm_sec);
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

  /* server returns: group last first y/m/x/j/=otherGroup */

  /* Is it really necessary to skip leading spaces?  JIK 2/19/95 */
  for ( ptr = line; *ptr == ' ';++ptr);	/* skip leading spaces */
  ptr2 = index(ptr, ' ');
  if (! ptr2)
    return ACTIVE_BOGUS;
  *ptr2 = '\0';
  group = ptr;
  if (sscanf(ptr2 + 1, " %ld %ld %s", &last, &first, type) != 3)
    return ACTIVE_BOGUS;

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
	if (*message == '.') {
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

    checkNewsrcSize(ActiveGroupsCount);
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
      if (response >= 0)
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


/*
 * Get a list of lines for a particular field for the specified group in
 * the range 'first' to 'last'.
 *
 * "fixfunction" is a function which takes a newsgroup, an article
 * number and a string and returns a new, allocated string to actually
 * assign to the field.
 *
 * "offset" is the offset of the field's pointer in a article
 * structure.
 * 
 * Returns: True if it's done, False to keep going.
 */
typedef char * (*_fixfunction) _ARGUMENTS((struct newsgroup *, art_num, char *));

static Boolean getlist _ARGUMENTS((struct newsgroup *, art_num, art_num,
				   Boolean, int *, char *, unsigned,
				   _fixfunction, unsigned, Boolean));

static Boolean getlist(
		       _ANSIDECL(struct newsgroup *,	newsgroup),
		       _ANSIDECL(art_num,		artfirst),
		       _ANSIDECL(art_num,		artlast),
		       _ANSIDECL(Boolean,		unreadonly),
		       _ANSIDECL(int *,			max),
		       _ANSIDECL(char *,		field),
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
      continue;
    }

    for (sub_count = 1, last = first + 1; last <= artlast; last++) {
      art = artStructGet(newsgroup, last, False);
      if (((offset != (unsigned)-1) && *(char **)((char *) art + offset)) ||
	  (fixfunction && *(char **)((char *) art + fixed_offset)) ||
	  (unreadonly && IS_READ(art)) ||
	  (max && ((count + sub_count) >= *max)))
	break;
      if (max && IS_AVAIL(art))
	sub_count++;
    }
    last--;

    (void) sprintf(command, "XHDR %s %ld-%ld", field, first, last);
    put_server(command);
    message = get_data_from_server(True);

    check_server_response(command, &message);

    /* check for errors */
    if (*message != CHAR_OK) {
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
      if (isspace(*message))
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
	goto next_iteration;
      }

      /*
	Strip leading and trailing spaces.
	*/
      while (*line && isspace(*line))
	line++;
      for (ptr = strchr(line, '\0') - 1; (ptr >= line) && isspace(*ptr);
	   *ptr-- = '\0') /* empty */;
      if (!required && (strcmp(line, "(none)") == 0))
	continue;

      art = artStructGet(newsgroup, number, True);
      if (offset != (unsigned)-1)
	*(char **)((char *)art + offset) =
	  XtNewString(line);
      if (fixfunction)
	*(char **)((char *) art + fixed_offset) =
	  (*fixfunction)(newsgroup, number, line);
      artStructSet(newsgroup, &art);
    }
    for (number = first; number <= last; number++) {
      struct article copy;
      Boolean changed = False;

      art = artStructGet(newsgroup, number, False);
      copy = *art;
      if ((offset != (unsigned)-1) && ! *(char **)((char *) art + offset))
	if (required)
	  goto unavail;
	else {
	  *(char **)((char *)&copy + offset) = XtNewString("");
	  changed = True;
	}
      if (fixfunction && ! *(char **)((char *)art + fixed_offset))
	if (required)
	  goto unavail;
	else {
	  *(char **)((char *)&copy + fixed_offset) = XtNewString("");
	  changed = True;
	}
      if (changed)
	artStructReplace(newsgroup, &art, &copy, number);
      continue;
    unavail:
      CLEAR_ALL_NO_FREE(&copy);
      SET_UNAVAIL(&copy);
      artStructReplace(newsgroup, &art, &copy, number);
    }
    first = last + 1;

  next_iteration:
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
		   "subject", offset, 0, 0, True);
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
		   "newsgroups", offset, 0, 0, True);
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
		   "xref", offset, 0, 0, False);
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
		   "date", offset, 0, 0, True);
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
		   "message-id", offset, 0, 0, True);
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
		   "references", offset, 0, 0, False);
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
		   "from", offset, authorFixFunction, fixed_offset, True);
}


static char *linesFixFunction(newsgroup, artnum, numoflines)
    struct newsgroup *newsgroup;
    art_num artnum;
    char *numoflines;
{
    char *end;
    int lcv;

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
    return XtNewString(numoflines);
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
		   "lines", (unsigned)-1, linesFixFunction, offset, False);
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
static char *xhdr_single _ARGUMENTS((struct newsgroup *, char *, long *));

static char *xhdr_single(newsgroup, buffer, error_code)
    struct newsgroup *newsgroup;
    char *buffer;
    long *error_code;
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
	fprintf(stderr, "NNTP error: %s\n", message);
	mesgPane(XRN_SERIOUS, 0, XHDR_ERROR_MSG);
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
    char buffer[BUFFER_SIZE], *message, *ptr, *cmp, *found = 0;

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
    message = get_data_from_server(True);

    check_server_response(buffer, &message);

    if (*message != CHAR_OK) {
	/* can't get header */
	*string = NIL(char);
	return;
    }

    for (;;) {
        message = get_data_from_server(True);

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
    char buffer[BUFFER_SIZE];
    long error_code;

    (void) sprintf(buffer, "XHDR %s %ld", field, article);
    *string = xhdr_single(newsgroup, buffer, &error_code);
    return;
}
#endif

/* Get XHDR information using an article's message ID.  The returned
   data is allocated and should be freed by the caller. */
  
char *xhdr_id(newsgroup, id, field, error_code)
     struct newsgroup *newsgroup;
     char *id;
     char *field;
     long *error_code;
{
  char buffer[BUFFER_SIZE];

  (void) sprintf(buffer, "XHDR %s %s", field, id);
  return xhdr_single(newsgroup, buffer, error_code);
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
