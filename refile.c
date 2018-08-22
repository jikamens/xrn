
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id$";
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

/* MH and RMAIL refiling: 
 *
 *   MH: Bob Ellison <ellison@sei.cmu.edu>
 *   RMAIL:  Michael Thomas <mike@gordian.com>
 */

#include "config.h"
#include "utils.h"
#include <ctype.h>
#if defined(SYSV) || defined(SVR4)
#include <fcntl.h>
#else
#include <sys/file.h>
#endif
#if STDC_HEADERS
#include <stdlib.h>
#else
    char *getenv();
#endif
#include <errno.h>

#include "xrn.h"
#include "dialogs.h"
#include "mesg.h"
#include "resources.h"
#include "error_hnds.h"
#include "mesg_strings.h"
#include "refile.h"

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif


/* replacement for MH routine */
static char * m_find _ARGUMENTS((char *));

static char * m_find(p)
    char *p;
{
    FILE *F;
    char *home, *mhprofile;
    char fullname [100];
    static char buf[512];
    char *q,*s;

    mhprofile = getenv("MH");
    if (!mhprofile) {
	home = getenv("HOME");
	if (!home) {
	    return 0;
	}
	(void) sprintf(fullname, "%s/.mh_profile", home);
    }
    else	/* either an absolute pathname, or relative to current dir */
	(void) strcpy(fullname, mhprofile);

    F = fopen(fullname, "r");
    if(F == NULL) {
	return 0;
    }
    while (fgets(buf, 512, F) != NULL) {
	q = index(buf,':');
        if(q != 0) {
	    *q = '\0';
	    s = ++q;
	    if (strcasecmp(buf, p) == 0) {
	        q += strspn(q," \t");
	        s = strpbrk(q," \n\t");
	        if(s) {
		    *s = '\0';
		}
	        (void) fclose(F);
	        return q;
	    }
        }
    }
    (void) fclose(F);
    return 0;
}


int MHrefile(folder, artfile)
    char *folder;
    char *artfile;
{
    char *p,*q;
    char  *userpath;
    char *getenv();
    char tmp[512];
    char fullpath[512];
    char fullpath2[512];
    char newfolders[512];
    char msg[512];
    struct stat st;
    extern int errno;

   
    p = getenv("HOME");
    if(p == 0) {
	return 0;
    }
    userpath = m_find("path");
    if(userpath == 0) {
	(void) strcpy(fullpath, p);
    } else if (userpath[0] == '/') {
	(void) strcpy(fullpath, userpath);
    } else {
	if (userpath[0] == '/')
	    (void) strcpy(fullpath, userpath);
	else
	    (void) sprintf(fullpath, "%s/%s", p, userpath);
    }
    if((stat(fullpath,&st) == -1) || !S_ISDIR(st.st_mode)) {
	mesgPane(XRN_SERIOUS, 0, NO_MAIL_DIR_MSG, fullpath);      
	return 0;
    }
    (void) sprintf(fullpath2, "%s/%s", fullpath, (folder+1));
    if (stat(fullpath2,&st) == -1 && errno == ENOENT) {
	(void) sprintf(msg,NO_SUCH_MAIL_DIR_MSG ,fullpath2);
	if (ConfirmationBox(TopLevel, msg, 0, 0, False)  == XRN_CB_ABORT) {
	    return 0;
	}
	(void) strcpy(newfolders, (folder+1));
	q = strtok(newfolders, "/");
	while (q) {
	    (void) strcat(fullpath, "/");
	    (void) strcat(fullpath, q);
	    if(stat(fullpath, &st) == -1) {
	        if(errno == ENOENT){
		    mkdir(fullpath, 0777);
	        } else {
		    mesgPane(XRN_SERIOUS, 0, CANT_STAT_MAIL_DIR_MSG, fullpath,
			     errmsg(errno));
		    return 0;
	        }
	     } else if (!S_ISDIR(st.st_mode)) {
		 mesgPane(XRN_SERIOUS, 0, MAIL_DIR_NOT_DIR_MSG, fullpath);
		 return 0;
	    }
	    q = strtok(0, "/");
        }
    } else if (!S_ISDIR(st.st_mode)) {
        mesgPane(XRN_SERIOUS, 0, FOLDER_NOT_DIR_MSG, fullpath2);
        return 0;
    }
    if (stat(artfile, &st) == -1) {
        return 0;
    }

    if (app_resources.mhPath)
	(void) sprintf(tmp, "%s/refile -f %s %s", app_resources.mhPath,
		       artfile, folder);
    else
	(void) sprintf(tmp, "refile -f %s %s", artfile, folder);

    (void) system(tmp);
    return 1;
}


/* 
 * Refile an article in EMACS RMAIL format
 */

/*ARGSUSED*/
int RMAILrefile(fullpath, folder, artfile, pos)
    char *fullpath, *folder, *artfile;
    long pos;
{
    char msg[512];
    struct stat st;
    extern int errno;
    FILE * fp;
    int artfd, rv, n;

    if (stat(fullpath,&st) == -1 && errno == ENOENT) {
	(void) sprintf(msg, NO_SUCH_RMAIL_MSG, fullpath);
	if (ConfirmationBox (TopLevel, msg, 0, 0, False)  == XRN_CB_ABORT) {
	    return 0;
	}
	if ((fp = fopen (fullpath, "w")) == NULL) {
	    mesgPane(XRN_SERIOUS, 0, CANT_OPEN_RMAIL_MSG, fullpath,
		     errmsg(errno));
	    return 0;
	}
	/* Produce the header */
	fprintf (fp, "BABYL OPTIONS: -*- rmail -*-\n");
	fprintf (fp, "Version: 5\n");
	fprintf (fp, "Labels:\n");
	fprintf (fp, "Note: This is RMAIL file produced by XRN:\n");
	fprintf (fp, "Note: if your are seeing this it means the file\n");
	fprintf (fp, "Note: has no messages in it.\n\037");
    } else {
	if ((fp = fopen (fullpath, "a")) == NULL) {
	    mesgPane(XRN_SERIOUS, 0, CANT_OPEN_RMAIL_MSG, fullpath,
		     errmsg(errno));
	    return 0;
	}
    }
    if ((artfd = open(artfile, 0)) < 0) {
	mesgPane(XRN_SERIOUS, 0, CANT_OPEN_TEMP_MSG, artfile, errmsg(errno));
	return(0);
    }
    /* Format the header */
    fprintf (fp, "\f\n1,,\n");
    /* insert from 0 to pos (from getarticle) for the header */
    n = 0;
    while ((rv = read (artfd, msg, n + 512 > pos ? pos - n : 512)) > 0) {
	if (! fwrite (msg, n+512 > pos ? pos - n : 512, 1, fp)) {
	    mesgPane(XRN_SERIOUS, 0, CANT_WRITE_RMAIL_MSG, fullpath,
		     errmsg(errno));
	    break;
	}
	n += rv;
    }
    /* reseek start of file */
    lseek (artfd, (off_t) 0, 0);
    if (fprintf (fp, "\n*** EOOH ***\n") == EOF) {
	mesgPane(XRN_SERIOUS, 0, CANT_WRITE_RMAIL_MSG, fullpath, errmsg(errno));
	(void) close(artfd);
	(void) fclose(fp);
	return(0);
    }
    /* insert the article */
    while ((rv = read (artfd, msg, 512)) > 0) {
	if (! fwrite (msg, rv, 1, fp)) {
	    mesgPane(XRN_SERIOUS, 0, CANT_WRITE_RMAIL_MSG, fullpath,
		     errmsg(errno));
	    (void) close(artfd);
	    (void) fclose(fp);
	    return(0);
	}
    }
    (void) close (artfd);
    /* insert the article end mark */
    if (fprintf (fp, "\037") == EOF) {
	mesgPane(XRN_SERIOUS, 0, CANT_WRITE_RMAIL_MSG, fullpath, errmsg(errno));
	(void) fclose(fp);
	return(0);
    }
    if (fclose(fp) == EOF) {
	mesgPane(XRN_SERIOUS, 0, CANT_WRITE_RMAIL_MSG, fullpath, errmsg(errno));
	return(0);
    }
    return 1;
}

