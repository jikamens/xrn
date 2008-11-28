#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <errno.h>
#include <X11/Intrinsic.h>
#include "mesg.h"
#include "mesg_strings.h"
#include "error_hnds.h"
#include "server.h"
#include "resources.h"
#include "news.h"
#include "activecache.h"
#include "cache.h"

void active_cache_read(filename)
char *filename;
{
  FILE *input;
  static char *line = 0, *ptr;
  static int line_size;
  int ret, len;

  if (! (input = fopen(filename, "r"))) {
    if (errno != ENOENT)
      mesgPane(XRN_SERIOUS, 0, CANT_OPEN_FILE_MSG, filename, errmsg(errno));
    return;
  }

  if (! line) {
    line_size = 80; /* arbitrary starting value */
    line = XtMalloc(line_size);
    ptr = line;
  }

  while (fgets(ptr, line_size - (ptr - line), input)) {
    if (ptr[(len = strlen(ptr))-1] != '\n') {
      if ((len + (ptr - line)) == (line_size - 1)) {
	len += ptr - line;
	line_size *= 2;
	line = XtRealloc(line, line_size);
	ptr = line + len;
      }
      else {
	ptr += len;
      }
      continue;
    }
    ptr = line;
    if (*line == CACHE_ACTIVE_CHAR) {
      switch (ret = parse_active_line(line + 1, TRUE, 0)) {
      case ACTIVE_IGNORED:
      case ACTIVE_NEW:
      case ACTIVE_OLD:
	break;
      case ACTIVE_BOGUS:
	mesgPane(XRN_SERIOUS, 0, BOGUS_ACTIVE_CACHE_MSG, line);
	break;
      default:
	mesgPane(XRN_SERIOUS, 0, UNKNOWN_FUNC_RESPONSE_MSG, ret,
		 "parse_active_line", "active_cache_read");
	break;
      }
    }
  }

  (void) fclose(input);
}

/*
  Write the active cache based on the newsgroup array.  Ignored
  newsgroups are not written.  Returns 0 on success, 1 on failure.
  */

#define WRITE_LINE { \
  if (fputs(line, output) == EOF) { \
    mesgPane(XRN_SERIOUS, 0, ERROR_WRITING_SAVE_FILE_MSG, tmpfile, errmsg(errno)); \
    (void) fclose(input); \
    (void) fclose(output); \
    XtFree(tmpfile); \
    return -1; \
  } \
}


int active_cache_write(
		       _ANSIDECL(char *,		filename),
		       _ANSIDECL(struct newsgroup **,	Newsrc),
		       _ANSIDECL(ng_num,		num_groups),
		       _ANSIDECL(Boolean,		write_entries)
		       )
     _KNRDECL(char *,			filename)
     _KNRDECL(struct newsgroup **,	Newsrc)
     _KNRDECL(ng_num,			num_groups)
     _KNRDECL(Boolean,			write_entries)
{
  FILE *input, *output;
  char *tmpfile;
  char last_chopped = 0, last_active = 0;
  char line[BUFSIZ];
  ng_num i;

  tmpfile = utTempFile(filename);

  if (! (output = fopen(tmpfile, "w"))) {
    mesgPane(XRN_SERIOUS, 0, CANT_OPEN_TEMP_MSG, tmpfile, errmsg(errno));
    XtFree(tmpfile);
    return -1;
  }

  if ((input = fopen(filename, "r"))) {
    while (fgets(line, sizeof(line), input)) {
      if (line[strlen(line) - 1] != '\n') {
	if (last_chopped) {
	  if (! last_active) {
	    WRITE_LINE;
	  }
	}
	else {
	  if (*line == CACHE_ACTIVE_CHAR) {
	    last_active = 1;
	  }
	  else {
	    WRITE_LINE;
	  }
	  last_chopped = 1;
	}
	continue;
      }
      if (*line != CACHE_ACTIVE_CHAR)
	WRITE_LINE;
    }
    (void) fclose(input);
  }
  else {
    if (errno != ENOENT)
      mesgPane(XRN_SERIOUS, 0, CANT_OPEN_FILE_MSG, filename, errmsg(errno));
  }

  if (write_entries) {
    for (i = 0; i < num_groups; i++) {
      (void) fprintf(output, "%c%s\n", CACHE_ACTIVE_CHAR,
		     unparse_active_line(Newsrc[i]));
      if (ferror(output)) {
	mesgPane(XRN_SERIOUS, 0, ERROR_WRITING_SAVE_FILE_MSG, tmpfile,
		 errmsg(errno));
	(void) fclose(output);
	XtFree(tmpfile);
	return -1;
      }
    }
  }

  if (fclose(output) == EOF) {
    mesgPane(XRN_SERIOUS, 0, ERROR_WRITING_SAVE_FILE_MSG, tmpfile,
	     errmsg(errno));
    XtFree(tmpfile);
    return -1;
  }

  if (rename(tmpfile, filename)) {
    mesgPane(XRN_SERIOUS, 0, ERROR_RENAMING_MSG, tmpfile, filename,
	     errmsg(errno));
    XtFree(tmpfile);
    return -1;
  }

  XtFree(tmpfile);
  return 0;
}

#undef WRITE_LINE
