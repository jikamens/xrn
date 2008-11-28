
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Header: /d/src/cvsroot/xrn/save.c,v 1.11 1994-11-18 14:34:01 jik Exp $";
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
 * save.c: routines for saving articles and sending articles to processes
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef VMS
#ifndef R_OK
#define F_OK            0       /* does file exist */
#define X_OK            1       /* is it executable by caller */
#define W_OK            2       /* writable by caller */
#define R_OK            4       /* readable by caller */
#endif /* R_OK */
#endif /* VMS */

#include "news.h"
#include "resources.h"
#include "error_hnds.h"
#include "server.h"
#include "mesg.h"
#include "save.h"
#include "mesg_strings.h"
#include "refile.h"

extern int errno;

#define BUFFER_SIZE 1024

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

/*
 * send the current article to 'command'
 *
 *   returns: the exit status of the command
 *
 */
#ifndef VMS
static int processArticle _ARGUMENTS((char *, char *));

static int processArticle(command, artfile)
    char *command;
    char *artfile;
{
    FILE *process, *fromfp;
    int c, status;

    if ((fromfp = fopen(artfile, "r")) == NULL) {
	mesgPane(XRN_SERIOUS, CANT_OPEN_ART_MSG, artfile, errmsg(errno));
	return(0);
    }
    
    if ((process = xrn_popen(command, "w")) == NULL) {
	mesgPane(XRN_SERIOUS, CANT_EXECUTE_CMD_POPEN_MSG, command);
	(void) fclose(fromfp);
	return(0);
    }

    while ((c = getc(fromfp)) != EOF) {
	(void) putc((char) c, process);
    }

    status = xrn_pclose(process);
    (void) fclose(fromfp);

    return(status);
}
#endif /* VMS */

/*
 * make sure that the news directory exists before trying to update it
 *
 *   returns: 0 for failure, 1 for okay
 */
int createNewsDir()
{
    static int done = 0;
    char *newdir;
    struct stat statbuf;

    if (done) {
	return(1);
    }
    if ((newdir = utTildeExpand(app_resources.saveDir)) == NIL(char)) {
	mesgPane(XRN_SERIOUS, CANT_EXPAND_SAVE_DIR_MSG, app_resources.saveDir);
	return(0);
    }
    /*
     * Originally, this code did mkdir() and, if it failed, checked if
     * errno was equal to EEXIST, in order to determine whether the
     * directory already existed.
     *
     * Unfortunately, that does not always result in a correct error,
     * because (for example) in AFS, if the directory already exists
     * AND the user is over quota, then EDQUOT, rather than EEXIST,
     * gets returned.  The user then gets a "Disc quota exceeded"
     * error in his message pane when there's really no reason for it.
     * 
     * Yes, it might be argued that this is AFS being stupid, and that
     * it should check if the directory exists *before* it checks if
     * the user is over quota.  However, unfortunately, applications
     * sometimes have to work around stupid operating system bugs.
     *
     * Therefore, I check to see if the directory exists and is a
     * directory, with stat, before calling mkdir().
     */

    if (! ((stat(newdir, &statbuf) == 0) && S_ISDIR(statbuf.st_mode))) {
	if ((mkdir(newdir, 0777) == -1) && (errno != EEXIST)) {
	    mesgPane(XRN_SERIOUS, CANT_CREATE_SAVE_DIR_MSG,
		     app_resources.saveDir, errmsg(errno));
	    return(0);
	}
    }

    done = 1;
    app_resources.expandedSaveDir = XtNewString(newdir);

    return(1);
}

#ifdef VMS
static void VmxMakeDirName _ARGUMENTS((char *, char *, char *));

static void VmsMakeDirName(dummy, savedir, Group)
    char *dummy;
    char *savedir;
    char *Group;
{
    int temp;
    (void) strcpy(dummy, savedir);	/* Copy save directory name */
    temp = strlen(dummy);		/* Fetch the length */
    if (dummy[temp-1] == ']') {		/* If a directory spec, */
        dummy[temp-1] = '.';		/*  Convert to a .  */
    } else {
	(void) strcat(dummy, "[.");	/* Else, start directory */
    }
    (void) strcat(dummy, Group);	/* Add the group name */
    (void) strcat(dummy, "]");		/* Now terminate the string */
}
#endif /* VMS */

/*
 * expand a file name for 'saving' a file
 *
 *   returns: the name of the file or NIL(char) if the filename is bad
 *            (i.e. ~user/xx where 'user' is not a valid user name)
 *            the area returned is static
 *
 */
static char * buildFileName _ARGUMENTS((char *, char *, char *));

static char * buildFileName(filename, savedir, group)
    char *filename;    /* file name, possibly with a '~' */
    char *savedir;     /* save directory                 */
    char *group;       /* name of the news group         */
{
    char Group[GROUP_NAME_SIZE];
#ifdef aiws
    static char dummy[MAXPATH];
#else
    static char dummy[MAXPATHLEN];
#endif /* aiws */
#ifdef VMS
    char *dot;	   /* Pointer to dots to convert to underscores */
#endif

#ifndef VMS
    /* Make a local copy of the group name for modification */
    (void) strncpy(Group, group, sizeof(Group));
    /* upcase the first letter of the group name (same as 'rn') */
    if (islower(Group[0])) {
	Group[0] = toupper(Group[0]);
    }
#else /* VMS */
    /* First, use the generic group to filename conversion routine and
       convert the group name to a filename */
    (void) utGroupToVmsFilename(Group, group);

    /* If we are in ONEDIR_SAVE mode, find the first underscore character
       and convert it (back) to a dot.  This is done for compatibility with
       VNEWS. */
    if (app_resources.saveMode & ONEDIR_SAVE) {
	if (dot = index(Group, '_')) {
	    *dot = '.';
	}
    }
#endif

    if ((filename == NIL(char)) || (*filename == '\0')) {
	if (app_resources.saveMode & ONEDIR_SAVE){
#ifndef VMS
	    (void) sprintf(dummy, "%s/%s", savedir, Group);
#else
	    (void) sprintf(dummy, "%s%s", savedir, Group);
#endif
	} else {
	    /* use "saveDir/group" */
#ifndef VMS
	    (void) sprintf(dummy, "%s/%s", savedir, group);
#else
	    VmsMakeDirName(dummy, savedir, Group);
#endif /* VMS */
	    (void) mkdir(utTildeExpand(dummy), 0777);
#ifndef VMS
	    (void) strcat(dummy, "/");
#endif /* VMS */
	    (void) strcat(dummy, Group);
#ifdef VMS
	    if (!(app_resources.saveMode & ONEDIR_SAVE)) {
		(void) strcat(dummy, ".ART");
	    }
#endif /* VMS */
	}
	return(utTildeExpand(dummy));
    }

#ifndef VMS
    if ((filename[0] == '/') || (filename[0] == '~')) {
	return(utTildeExpand(filename));
    }
#else
    if ((index(filename, ':')) || (index(filename, '['))
		|| (index(filename, '<'))) {
	return(utTildeExpand(filename));
    }
#endif /* VMS */

    if (app_resources.saveMode & ONEDIR_SAVE) {
#ifndef VMS
	(void) sprintf(dummy, "%s/%s", savedir, filename);
#else /* VMS */
	(void) sprintf(dummy, "%s%s", savedir, filename);
#endif /* VMS */
    } else {
	/* use "saveDir/group/filename" */
#ifndef VMS
	(void) sprintf(dummy, "%s/%s", savedir, group);
#else /* VMS */
	VmsMakeDirName(dummy, savedir, Group);
#endif /* VMS */
	(void) mkdir(utTildeExpand(dummy), 0777);
#ifndef VMS
	(void) strcat(dummy, "/");
#endif /* VMS */
	(void) strcat(dummy, filename);
    }
    return(utTildeExpand(dummy));
}

int saveArticleByNumber(filename, art)
    char *filename;
    art_num art;
{
    return saveArticle(filename, CurrentGroup, art);
}

int saveArticle(filename, newsgroup, art)
    char *filename;
    struct newsgroup *newsgroup;
    art_num art;
{
    char timeString[BUFFER_SIZE];
    char inputbuf[BUFSIZ];
    int error = 0;
    extern char *ctime _ARGUMENTS((CONST time_t *));
    extern time_t time _ARGUMENTS((time_t *));
    time_t clock;
    long pos;
    char *artfile, *fullName;
    FILE *fpart, *fpsave;
    int xlation = 0;
    int rotation;
    char mode[2], string[256];
    int c;
    struct stat buf;

    if ((filename != NIL(char)) && (*filename != '\0')) {
	filename = utTrimSpaces(filename);
    }

    /* get the FULL article */

    rotation = (IS_ROTATED(newsgroup->articles[INDEX(art)]) ? ROTATED : NOT_ROTATED);
#ifdef XLATE
    xlation = (IS_XLATED(newsgroup->articles[INDEX(art)]) ? XLATED : NOT_XLATED);
#endif
    artfile = getarticle(newsgroup, art, &pos, FULL_HEADER, rotation, xlation);

#ifndef VMS
    /* 
     * check a few special cases before actually saving the article
     * to a plain text file:
     *
     * - pipe it to a command
     * - file it to an mh folder
     * - file it to an RMAIL folder
     */
     
    /* see if the file name is actually a command specification */
    if ((filename != NIL(char)) && (filename[0] == '|')) {
	int status;
	(void) sprintf(error_buffer, "Piping article %ld into command `%s'...    ",
		       art, &filename[1]);
	infoNow(error_buffer);
    	status = processArticle(utTrimSpaces(&filename[1]), artfile);
	(void) unlink(artfile);
	FREE(artfile);
	if (status) {
	    (void) sprintf(error_buffer, "`%s' exited with status %d",
				&filename[1], status);
	} else {
	    (void) strcpy(&error_buffer[strlen(error_buffer) - 4], "done");
	}
	info(error_buffer);
	return(status == 0);
    }
#endif

    if ((filename != NIL(char)) && (filename[0] == '+')) {
	int status = MHrefile(filename, artfile);
	FREE(artfile);
	(void) sprintf(error_buffer, "MH refile to folder %s done", filename);
	infoNow(error_buffer);
	return(status);
    }

    /* XXX not quite right, don't want to try to create it if not used... */
    if (!createNewsDir()) {
	(void) unlink(artfile);
	FREE(artfile);
	return(0);
    }

    if (filename != NIL(char) && filename [0] == '@') {
	int status;
	if ((fullName = buildFileName(filename+1, app_resources.saveDir,
				      newsgroup->name)) == NIL(char)) {
	    mesgPane(XRN_SERIOUS, CANT_FIGURE_FILE_NAME_MSG, filename+1);
	    (void) unlink(artfile);
	    FREE(artfile);
	    return(0);
	}
	status = RMAILrefile(fullName, filename+1, artfile, pos);
	(void) sprintf(error_buffer, "RMAIL refile to folder %s done",
			filename+1);
	infoNow(error_buffer);
	(void) unlink(artfile);
	FREE(artfile);
	return(status);
    }
    
    if ((fullName = buildFileName(filename, app_resources.saveDir, newsgroup->name)) == NIL(char)) {
	mesgPane(XRN_SERIOUS, CANT_FIGURE_FILE_NAME_MSG, filename);
	(void) unlink(artfile);
	FREE(artfile);
	return(0);
    }

    if ((fpart = fopen(artfile, "r")) == NULL) {
	mesgPane(XRN_SERIOUS, CANT_OPEN_ART_MSG, artfile, errmsg(errno));
	(void) unlink(artfile);
	FREE(artfile);
	return(0);
    }

    if (stat(fullName, &buf) == 0) {
	(void) strcpy(mode, "a");
    } else {
	(void) strcpy(mode, "w");
    }
    
    if ((fpsave = fopen(fullName, mode)) == NULL) {
	(void) fclose(fpart);
	(void) unlink(artfile);
	FREE(artfile);
	mesgPane(XRN_SERIOUS, CANT_CREAT_APPEND_SAVE_FILE_MSG,
		 (mode[0] == 'w') ? "create" : "append to",
		 fullName, errmsg(errno));
	return(0);
    }

    if (mode[0] == 'w') {
	(void) sprintf(error_buffer, "Saving article %ld in file `%s'...     ",
			   art, fullName);
    } else {
	(void) sprintf(error_buffer, "Appending article %ld to file `%s'...     ",
		       art, fullName);
    }	
     
    infoNow(error_buffer);

    if ((app_resources.saveMode & MAILBOX_SAVE) == MAILBOX_SAVE) {
	(void) time(&clock);
	(void) strcpy(timeString, ctime(&clock));
	timeString[strlen(timeString) - 1] = '\0';  /* get rid of the newline */

	while (fgets(string, sizeof(string), fpart) != NULL) {
	    if (STREQN(string, "Path:", 5)) {
		string[strlen(string) - 1] = '\0';    /* get rid of the newline */
		if (fprintf(fpsave, "From %s %s\n", &string[6], timeString)
		    == EOF) {
		     error++;
		     goto finished;
		}
		break;
	    }
	}
	(void) rewind(fpart);
    }

    if (fprintf(fpsave, "Article: %ld of %s\n", art, newsgroup->name) == EOF) {
	 error++;
	 goto finished;
    }

    if ((app_resources.saveMode & HEADERS_SAVE) != HEADERS_SAVE) {
	(void) fseek(fpart, (off_t) pos, 0);
    }
    if ((app_resources.saveMode & MAILBOX_SAVE) == MAILBOX_SAVE) {
	while (fgets(string, sizeof(string), fpart) != NULL) {
	    if (STREQN(string, "From ", 5)) {
		if (fprintf(fpsave, ">From %s", &string[5]) == EOF) {
		    error++;
		    goto finished;
		}
	    } else {
		if (fputs(string, fpsave) == EOF) {
		    error++;
		    goto finished;
		}
	    }
	}
    } else {
	while ((c = fread(inputbuf, sizeof(char), BUFSIZ, fpart))) {
	    if (! fwrite(inputbuf, sizeof(char), c, fpsave)) {
		error++;
		goto finished;
	    }
	}
    }

    if (fprintf(fpsave, "\n\n") == EOF) {
	error++;
	goto finished;
    }

finished:
    (void) fclose(fpart);
    (void) unlink(artfile);
    FREE(artfile);

    if (fclose(fpsave) == EOF) {
	error++;
    }

    (void) strcpy(&error_buffer[strlen(error_buffer) - 4], 
		    error ? "aborted" : "done");
    if (error) {
	info(error_buffer);
	mesgPane(XRN_SERIOUS, ERROR_WRITING_SAVE_FILE_MSG, fullName,
		 errmsg(errno));
	return 0;
    } else {
	info(error_buffer);
	SET_SAVED(newsgroup->articles[INDEX(art)]);
	return 1;
    }
}

/*
 * save the current article in 'filename' (if it begins with '|', send it to
 *   a process)
 *
 *   returns: 1 for OKAY, 0 for FAILURE
 *
 */
int saveCurrentArticle(filename)
    char *filename;
{
     struct newsgroup *newsgroup = CurrentGroup;
     return saveArticle(filename, newsgroup, CurrentGroup->current);
}
