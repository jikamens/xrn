
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: newsrcfile.c,v 1.37 1997-01-12 03:41:22 jik Exp $";
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
 * newsrcfile.c: routines for reading and updating the newsrc file
 *
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <errno.h>
#ifdef _XOPEN_SOURCE
#include <stdlib.h>
#endif
#include "news.h"
#include "artstruct.h"
#include "error_hnds.h"
#include "mesg.h"
#include "xrn.h"
#include "dialogs.h"
#include "resources.h"
#include "newsrcfile.h"
#include "mesg_strings.h"
#include "varfile.h"
#include "internals.h"
#include "activecache.h"
#include "killfile.h"

#ifndef R_OK
#define R_OK 4
#endif

char *NewsrcFile;    /* newsrc file name         */
FILE *Newsrcfp;      /* newsrc file FILE pointer */
char *optionsLine;   /* `options' line           */
struct stat fbuf;
struct newsgroup **Newsrc = 0;
static ng_num Newsrc_size = 0;

void checkNewsrcSize(
		     _ANSIDECL(ng_num,	size)
		     )
     _KNRDECL(ng_num,	size)
{
  if (size > Newsrc_size) {
    Newsrc = (struct newsgroup **) XtRealloc((char *) Newsrc,
					     sizeof(*Newsrc) * size);
    Newsrc_size = size;
  }
}

#define OKAY  1
#define FATAL 0

/*
 * copy newsrc file to .oldnewsrc file
 */

static int copyNewsrcFile _ARGUMENTS((char *, char *));

static int copyNewsrcFile(old, save)
    char *old;   /* name of file to save */
    char *save;  /* name of file to save to */
{
    FILE *orig, *new;
    char buf[BUFSIZ];
    char *newFile;
    int num_read;

    if ((orig = fopen(old, "r")) == NULL) {
	mesgPane(XRN_SERIOUS, 0, CANT_OPEN_NEWSRC_COPYING_MSG, old,
		 errmsg(errno));
	return FATAL;
    }

    if ((newFile = utNameExpand(save)) == NIL(char)) {
	mesgPane(XRN_SERIOUS, 0, CANT_EXPAND_MSG, save);
	return FATAL;
    }

    if (STREQ(newFile, "")) {
	 mesgPane(XRN_SERIOUS, 0, EMPTY_NEWSRC_SAVE_NAME_MSG);
	 return FATAL;
    }

    /* if .oldnewsrc is a link to .newsrc we could have trouble, so unlink it */
    (void) unlink(newFile);
    
    if ((new = fopen(newFile, "w")) == NULL) {
	mesgPane(XRN_SERIOUS, 0, CANT_OPEN_NEWSRC_SAVE_MSG, save, errmsg(errno));
	return FATAL;
    }

    while ((num_read = fread(buf, sizeof(char), BUFSIZ, orig))) {
	 if (! fwrite(buf, sizeof(char), num_read, new)) {
	     mesgPane(XRN_SERIOUS, 0, NEWSRC_SAVE_FILE_WRITE_ERR_MSG, save,
		      errmsg(errno));
	      return FATAL;
	 }
    }

    (void) fclose(orig);
    if (fclose(new) == EOF) {
	mesgPane(XRN_SERIOUS, 0, NEWSRC_SAVE_FILE_WRITE_ERR_MSG, save,
		 errmsg(errno));
	 return FATAL;
    }

    return OKAY;
}

/*
 * read, parse, and process the .newsrc file
 *
 *   returns: 0 for fatal error, non-zero for okay
 *
 */
int readnewsrc(newsrcfile, savenewsrcfile)
    char *newsrcfile;
    char *savenewsrcfile;
{
    struct stat buf;
    extern int yyparse _ARGUMENTS((void));
    extern int newsrc_mesg_name;

    checkNewsrcSize(ActiveGroupsCount);

    optionsLine = NIL(char);

    if (! (NewsrcFile = findServerFile(newsrcfile, False))) {
      mesgPane(XRN_SERIOUS, 0, CANT_EXPAND_MSG, newsrcfile);
      return FATAL;
    }

    if (access(NewsrcFile, R_OK) != 0) {
	if (errno != ENOENT) {
	    mesgPane(XRN_SERIOUS, 0, CANT_READ_NEWSRC_MSG, NewsrcFile,
		     errmsg(errno));
	    return FATAL;
	}
	mesgPane(XRN_INFO, 0, CREATING_NEWSRC_MSG, NewsrcFile);
	if ((Newsrcfp = fopen(NewsrcFile, "w")) == NULL) {
	    mesgPane(XRN_SERIOUS, 0, CANT_CREATE_NEWSRC_MSG, NewsrcFile,
		     errmsg(errno));
	    return FATAL;
	}
	if (NEWUSER_GROUPS[0] == '/') {
          /* if NEWUSER_GROUPS begins with '/', we assume it's a filename */
          char tmpbuf[BUFSIZ];
          FILE *NewRCfp;

          if(! (NewRCfp = fopen(NEWUSER_GROUPS, "r"))) {
	    mesgPane(XRN_SERIOUS, 0, CANT_READ_NEWSRC_MSG, 
		     NEWUSER_GROUPS, errmsg(errno));
	    return FATAL;
          }
          while(fgets(tmpbuf, BUFSIZ-1, NewRCfp)) {
	    (void) fprintf(Newsrcfp, "%s", tmpbuf);
          }
          (void) fclose(NewRCfp);
	}
	else {
	  (void) fprintf(Newsrcfp, NEWUSER_GROUPS);
	}

	(void) fstat((int) fileno(Newsrcfp), &fbuf);
	do_chmod(Newsrcfp, 0, fbuf.st_mode);
	(void) fclose(Newsrcfp);
	do_chmod(0, NewsrcFile, fbuf.st_mode);
    }

    if (stat(NewsrcFile, &buf) == -1) {
	mesgPane(XRN_SERIOUS, 0, CANT_STAT_NEWSRC_MSG, NewsrcFile,
		 errmsg(errno));
	return FATAL;
    }
    
    if (buf.st_size == 0) {
	mesgPane(XRN_SERIOUS, 0, ZERO_LENGTH_NEWSRC_MSG, NewsrcFile);
	return FATAL;
    }

    if ((Newsrcfp = fopen(NewsrcFile, "r")) == NULL) {
	mesgPane(XRN_SERIOUS, 0, CANT_OPEN_NEWSRC_MSG, NewsrcFile, errmsg(errno));
	return FATAL;
    }

    newsrc_mesg_name = newMesgPaneName();
    if (yyparse() != 0) {
	mesgPane(XRN_SERIOUS, 0, CANT_PARSE_NEWSRC_MSG, NewsrcFile,
		 MaxGroupNumber +1);
	return FATAL;
    }

    if (!copyNewsrcFile(NewsrcFile, savenewsrcfile))
	 return FATAL;

    (void) fstat((int) fileno(Newsrcfp), &fbuf);

    do_chmod(Newsrcfp, 0, fbuf.st_mode);
    (void) fclose(Newsrcfp);
    do_chmod(0, NewsrcFile, fbuf.st_mode);

    Newsrcfp = NIL(FILE);

    return(OKAY);
}

static int ngEntryFprintf _ARGUMENTS((FILE *, struct newsgroup *));

static int ngEntryFprintf(newsrcfp, newsgroup)
    FILE *newsrcfp;
    struct newsgroup *newsgroup;
{
    int first = 1;
    struct list *item;

    /* process the .newsrc line */

    for (item = newsgroup->nglist; item != NIL(struct list); item = item->next) {
	if (first)
	    first = 0;
	else
	    if (fprintf(newsrcfp, ",") == EOF) {
		return 0;
	    }

	switch (item->type) {
	    case SINGLE:
	    if (fprintf(newsrcfp, "%ld", item->contents.single) == EOF) {
		return 0;
	    }
	    break;

	    case RANGE:
	    if (item->contents.range.start != item->contents.range.end) {
		if (fprintf(newsrcfp, "%ld-%ld",
				       item->contents.range.start,
				       item->contents.range.end) == EOF) {
		    return 0;
		}
	    } else {
		if (fprintf(newsrcfp, "%ld", item->contents.range.start) == EOF) {
		    return 0;
		}
	    }
	    break;
	}
    }
    return 1;
}

/*
 * write out an up to date copy of the .newsrc file
 *
 *   returns: 0 for fatal error, non-zero for okay
 *
 */
int updatenewsrc()
{
    ng_num indx;

    static FILE *newsrcfp;       /* file pointer for the newsc file      */
    static struct stat lastStat; /* last stat done on the file           */
    struct stat currentStat;     /* current stat                         */
    static char *tempfile;
    static int retval;

    if (! MaxGroupNumber)
	/* hasn't been read in yet */
	return 1;

    if (! var_write_file(cache_variables, cache_file))
	return(FATAL);
    if (active_cache_write(cache_file, Newsrc, MaxGroupNumber,
			   app_resources.cacheActive))
	return(FATAL);

    (void) stat(NewsrcFile, &currentStat);

    if (lastStat.st_mtime && (currentStat.st_mtime > lastStat.st_mtime)) {
	if (ConfirmationBox(TopLevel, ASK_UPDATE_NEWSRC_MSG, 0, 0, False)
	    == XRN_CB_ABORT) {
	    ehNoUpdateExitXRN();
	}
    }

    tempfile = utTempFile(NewsrcFile);

    if ((newsrcfp = fopen(tempfile, "w")) == NULL) {
	mesgPane(XRN_SERIOUS, 0, CANT_OPEN_NEWSRC_TEMP_MSG, tempfile,
		 errmsg(errno));
	XtFree(tempfile);
	return(FATAL);
    }

#define FAILIF(cond) \
    if (cond) { \
      (void) fclose(newsrcfp); \
      (void) unlink(tempfile); \
      XtFree(tempfile); \
      return(FATAL); \
    }

    /*
     * handle outputing the options line
     */
    if (optionsLine != NIL(char)) {
      FAILIF(fprintf(newsrcfp, "%s\n", optionsLine) == EOF);
    }

    for (indx = 0; indx < MaxGroupNumber; indx++) {
	struct newsgroup *newsgroup = Newsrc[indx];

	write_kill_file(newsgroup, KILL_LOCAL);
	write_kill_file(newsgroup, KILL_GLOBAL);

	FAILIF(fprintf(newsrcfp, "%s%c", newsgroup->name,
		       (IS_SUBSCRIBED(newsgroup) ? ':' : '!')) == EOF);

	if (newsgroup->last == 0) {
	  FAILIF(fprintf(newsrcfp, "\n") == EOF);
	  continue;
	}

	if (! artListFirst(newsgroup, 0, 0)) {
	    if (newsgroup->nglist) {
	      FAILIF(fprintf(newsrcfp, " ") == EOF);
	      FAILIF(!ngEntryFprintf(newsrcfp, newsgroup));
	      FAILIF(fprintf(newsrcfp, "\n") == EOF);
	    } else {
	      FAILIF(fprintf(newsrcfp, " 1-%ld\n", newsgroup->last) == EOF);
	    }
	    continue;
	}
	
	if (newsgroup->last >= newsgroup->first) {
	    struct article *art;
	    Boolean comma = False;
	    art_num first, last;
	    art_num last_first = 0, last_last = 0;

	    for (art = artListFirst(newsgroup, &first, &last);
		 art; art = artStructNext(newsgroup, art, &first, &last)) {
		if (IS_READ(art) || IS_UNAVAIL(art)) {
		    if (last_last) {
			if (first == last_last + 1) {
			    last_last = last;
			    continue;
			}
		      do_output:
			FAILIF(putc(comma ? ',' : ' ', newsrcfp) == EOF);
			if (last_first == last_last) {
			  FAILIF(fprintf(newsrcfp, "%ld", last_first) == EOF);
			}
			else {
			  FAILIF(fprintf(newsrcfp, "%ld-%ld",
					 last_first, last_last) == EOF);
			}
			last_first = last_last = 0;
			comma = True;
			if (art)
			    continue;
			else
			    break;
		    }
		    else {
			last_first = first;
			last_last = last;
			continue;
		    }
		}
		else if (last_last)
		    goto do_output;
	    }
	    if (last_last)
		goto do_output;
	} else {
	    if (newsgroup->last > 1) {
	      FAILIF(fprintf(newsrcfp, " 1-%ld", newsgroup->last) == EOF);
	    }
	}
	    
	FAILIF(fprintf(newsrcfp, "\n") == EOF);
    }

#undef FAILIF

    do_chmod(newsrcfp, 0, fbuf.st_mode);
    retval = fclose(newsrcfp);
    do_chmod(0, tempfile, fbuf.st_mode);

    if (retval == EOF) {
      (void) unlink(tempfile);
      XtFree(tempfile);
      return(FATAL);
    }

#ifdef ISC_TCP
    /* 
     * the following added to fix a bug in ISC TCP/IP rename() 
     * - jrh@dell.dell.com (James Howard) 
     */
    if (unlink(NewsrcFile) != 0) {
	mesgPane(XRN_SERIOUS, 0, ERROR_UNLINKING_NEWSRC_MSG, NewsrcFile,
		 errmsg(errno));
	(void) unlink(tempfile);
	XtFree(tempfile);
	return(FATAL);
    }
#endif /* ISC_TCP */

    if (rename(tempfile, NewsrcFile) != 0) {
	mesgPane(XRN_SERIOUS, 0, ERROR_RENAMING_MSG, tempfile, NewsrcFile,
		 errmsg(errno));
	(void) unlink(tempfile);
	XtFree(tempfile);
	return(FATAL);
    }

    XtFree(tempfile);

    (void) stat(NewsrcFile, &lastStat);

    return(OKAY);
}
