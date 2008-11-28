
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: cursor.c,v 1.10 1994-11-23 01:47:09 jik Exp $";
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
 * cursor.c: routines for manipulating the cursor and/or text in a text
 *           window
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <assert.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>

#ifndef MOTIF
#include <X11/Xaw/Text.h>
#else
#include <Xm/Text.h>
#endif

#include "news.h"
#include "mesg.h"
#include "internals.h"
#include "modes.h"
#include "xrn.h"
#include "cursor.h"
#include "error_hnds.h"
#include "cancel.h"
#include "mesg_strings.h"

#ifdef VMS
#define getArticle getArticleFile
#endif /* VMS */

/*
 * Move the cursor to the beginning of the current line.
 * If the cursor is at the end of string marker, leave it there.
 */
void moveBeginning(tstring, point)
    char *tstring;			/* text string */
    XawTextPosition *point;		/* cursor position */
{
    int i = 0;
    while ((*point != 0) && (tstring[*point-1] != '\n') &&
		(tstring[*point] != '\0')) {
	(*point)--;
	i++;
	if (i > 1000) {
	    ehErrorExitXRN("XRN panic: moveBeginning in infinite loop\n");
	}
    }
    return;
}

/*
 * Move the cursor to the end of the current line.
 * If the cursor is at the end of string marker, leave it there.
 */
void moveEnd(tstring, point)
    char *tstring;			/* text string */
    XawTextPosition *point;		/* cursor position */
{
    int i = 0;
    while ((tstring[*point] != '\n') && (tstring[*point] != '\0')) {
	(*point)++;
	i++;
	if (i > 1000) {
	    ehErrorExitXRN("XRN panic: moveEnd in infinite loop\n");
	}
    }
    return;
}

/*
 * Move the cursor forward or backward a line.
 * Return false if 1) the cursor is already at the top line
 * and BACK is requested or 2) the cursor is at the end of
 * string marker and FORWARD is requested.
 */
int moveCursor(direction, tstring, point)
    int direction;		    /* FORWARD or BACK (defined in cursor.h) */
    char *tstring;		    /* text string */
    XawTextPosition *point;	    /* cursor position */
{
    if (direction == BACK) {

	/* if we are at the first group, return false to show we did */
	/* not actually move back */

	if (*point ==  0) {
	    return FALSE;
	}
	
	/* if we are in the middle of a line, move to the first position */

	if (tstring[*point-1] != '\n') {
	    moveBeginning(tstring, point);
	    if (*point == 0) return FALSE;
	}
	/* now we are definitely at the beginning of a line, and it is */
	/* not the first line, scan back to beginning of previous line */

	*point -= 1;
	moveBeginning(tstring, point);
	return TRUE;
    }

    /* the default is to move forward */

    if (tstring[*point] == '\0') {
	return FALSE;
    }
    moveEnd(tstring, point);
    (*point)++;
    return TRUE;
}

/*
 * Move the cursor forward a line, wrapping around if the end of string
 * character is reached.  Return false only if the string is null.
 */
int moveUpWrap(tstring, point)
    char *tstring;			/* text string */
    XawTextPosition *point;		/* cursor position */
{
    if (tstring[*point] == '\0') {
	if (*point == 0) {
	    return FALSE;
	}
	*point = 0;
	return TRUE;
    }
    moveEnd(tstring, point);
    if (tstring[*point+1] == '\0') {
	*point = 0;
    } else {
	(*point)++;
    }
    return TRUE;
}

/*
 * Move the insertion point to the beginning of the last line of the string
 * tstring.
 */
void endInsertionPoint(tstring, point)
    char *tstring;			/* text string */
    XawTextPosition *point;		/* cursor position */
{
    /* move to the end of the string */

    while (tstring[*point] != '\0') {
	(*point)++;
    }

    /* as long as the string is not null, scan back to beginning of */
    /* previous line */

    if (*point != 0) {
	*point -= 1;
	moveBeginning(tstring, point);
    }
    return;
}

/*
 * Gets the selected text or the cursor position, if no selected text.
 * Moves the left and right positions to include
 * whole lines only.  Returns false if there is no selected text and the
 * cursor is sitting on the end of string character.
 */
int getSelection(twin, tstring, left, right)
    Widget twin;			/* text window */
    char *tstring;			/* text string */
    XawTextPosition *left, *right;	/* beginning and end of selection */
				    /* (to be determined here) */
{
    XawTextGetSelectionPos(twin, left, right);
    XawTextUnsetSelection(twin);

    if (*left == *right) {

	/* nothing was selected */

	*left = *right = XawTextGetInsertionPoint(twin);
	if (tstring[*left] == '\0') {

	    /* cursor is at end of string character */

	    return FALSE;
	}

	/* return the line the cursor was on */

	moveEnd(tstring, right);
	return TRUE;
    }

    /* adjust the left marker to the beginning of a string */

    moveBeginning(tstring, left);
    if (tstring[*right-1] == '\n') {

	/* adjust the right marker to the end of a string */

	(*right)--;
    }
    moveEnd(tstring, right);

    return TRUE;
}

/*
 * Remove the line the cursor is on (beginning at point)
 * from the given string.  Update the source tsource and
 * the text window twin.  Set the 
 * cursor to the first position in the next line.
 */
void removeLine(tstring, twin, tsource, point, ttop)
    char *tstring;			/* text string */
    Widget twin;			/* text window */
    Widget *tsource;
    XawTextPosition point, *ttop;	/* beginning of line to be removed, */
				    /* position of top of text window */
{
    XawTextPosition right;	/* end of line to be removed */

    right = point;
    moveEnd(tstring, &right);

    (void) strcpy(&tstring[point], &tstring[right+1]);

    /* update the text in the subject window */

    if (*tsource != 0) {
	*ttop = XawTextTopPosition(twin);
    } else {
#ifndef MOTIF
	Arg sargs[5];

	*ttop = (XawTextPosition) 0;
	XtSetArg(sargs[0], XtNstring, tstring);
	XtSetArg(sargs[1], XtNlength, utStrlen(tstring) + 1);
	*tsource = XtCreateWidget("subjectTextSource",
				  asciiSrcObjectClass,
				  twin, sargs, XtNumber(sargs));
	XawTextSetSource(twin, *tsource, (XawTextPosition) *ttop);
#else
	XawTextSetMotifString(twin, tstring);
#endif
    }
    XawTextSetInsertionPoint(twin, point);
    return;
}

/*
 * Leave the cursor at the current position unless it is at the
 * end of string marker, in which case move it to the beginning.
 * If the string is empty, return FALSE.
 */
int setCursorCurrent(tstring, point)
    char *tstring;			/* text string */
    XawTextPosition *point;		/* cursor position */
{
    if (tstring[*point] == '\0') {
	if (*point == 0) {
	    return FALSE;
	} else {
	    *point = (XawTextPosition) 0;
	}
    }
    return TRUE;
}

/*
 * Return the name of the group on the current line.
 * Assume there is a group there and the cursor is at the beginning
 * of the line.
 */
void currentGroup(mode, tstring, groupName, point)
    int mode;			/* xrn Mode */
    char *tstring;			/* text string */
    char *groupName;		/* string to return group name in */
    XawTextPosition point;		/* cursor position */
{
    if ((mode != ALL_MODE) && (mode != ADD_MODE)) {
	if (sscanf(&tstring[point], NEWS_GROUP_LINE, groupName) == 1) {
	    return;
	} else {
	    *groupName = '\0';
	    return;
	}
    } else {
	(void) sscanf(&tstring[point], "%s", groupName);
    }
    return;
}

/*
 * Return the status of the group on the current line.
 */
void currentMode(tstring, groupName, mode, point)
    char *tstring;
    char *groupName;
    int *mode;
    XawTextPosition point;
{
    char status[100];

    (void) sscanf(&tstring[point], "%s %s", groupName, status);
    if (strcmp(status, "unsubscribed") == 0) {
	*mode = UNSUBSCRIBE;
    } else {
	*mode = SUBSCRIBE;
    }
    return;
}

/*
 * Mark the article at the current ArticlePosition as read,if it is
 * not already marked. Return the article number of the article
 * that was marked.  This subroutine only marks the text string;
 * the article is marked internally as read by calling 
 * markArticleAsRead().
 */
int markStringRead(tstring, point)
    char *tstring;			/* text string */
    XawTextPosition point;		/* cursor position */
{
    if (tstring[point] == ' ') {
	tstring[point] = '+';
    }
    return atoi(&tstring[point+2]);
}

/*
 * Mark all groups in the region between the left and right bytes
 * of the text string 'tstring' as subscribed or unsubscribed,
 * depending on the contents of the string status.
 */
void markAllString(tstring, left, right, status)
    char *tstring;			/* text string */
    XawTextPosition left, right;	/* boundaries of groups to be marked */
    char *status;			/* "subscribed" or "unsubscribed" */
{
    XawTextPosition temp, point;	/* positions used to move through string */
    int i;
    
    temp = left;
    while (temp < right) {
	moveEnd(tstring, &temp);
	point = temp;
	for (i = 0; i < utStrlen(status); i++) {
	    tstring[point-1] = status[utStrlen(status) - i - 1];
	    point--;
	}
	temp++;
    }
    return;
}

/*
 * Mark a group of articles between left and right as read or unread.
 * Marks the articles in the text string, and marks them internally.
 */
void markArticles(tstring, left, right, marker)
    char *tstring;		      /* text string */
    XawTextPosition left, right;      /* boundaries of articles to be marked */
    char marker;		      /* either '+' or ' ' */
{
    long artNum;		/* number of current article to be marked */

    while (left < right) {
	tstring[left] = marker;
	artNum = atol(&tstring[left+2]);
	if (marker == '+') {
	    markArticleAsRead(artNum);
	} else {
	    markArticleAsUnread(artNum);
	}
	(void) moveCursor(FORWARD, tstring, &left);
    }
    (void) moveCursor(BACK, tstring, &left);
    return;
}

/*
 * Build a new string from a portion of the old string.
 */
void buildString(newString, first, last, oldString)
    char **newString;		       /* New text string */
    XawTextPosition first, last;       /* portion of old string to be copied */
    char *oldString;		       /* old text string */
{
    *newString = ARRAYALLOC(char, (last - first + 2));
    (void) strncpy(*newString, &oldString[first], (int) (last - first + 1));
    (*newString)[last - first + 1] = '\0';
}

/*
 * Move the cursor to the position of the article "num"
 */
void findArticle(tstring, num, position)
    char *tstring;			/* text string */
    art_num num;			/* article number to search for */
    XawTextPosition *position;		/* cursor position */
{
    long artNum;		/* number of current article */
    long pos = *position + 1;

    /* move over S[aved] / P[rinted] marking */
    if ((tstring[pos] == 'S') || (tstring[pos] == 'P')) {
	pos++;
    }
    artNum = atol(&tstring[pos]);
    while (artNum != num) {
	if (!moveCursor(FORWARD, tstring, position)) {
	    ehErrorExitXRN("Valid article number not found in findArticle (cursor.c)\n");
	}
	pos = *position + 1;
	/* move over S[aved] / P[rinted] marking */
	if ((tstring[pos] == 'S') || (tstring[pos] == 'P')) {
	    pos++;
	}
	artNum = atol(&tstring[pos]);
    }
    return;
}


int subjectSearch(dir, tstring, tsource, position, ttop, expr, file, ques,
		  artNum)
    int dir;			     /* direction, either FORWARD or BACK */
    char **tstring;		     /* text string (may be changed here) */
    Widget tsource;
    XawTextPosition *position, ttop; /* cursor position, position of
					top of text */
    char *expr;			     /* regular expression to search for */
    char **file, **ques;	     /* filename and status line for
					new article */
    long *artNum;		     /* number of new article */
{
    static char *reRet = 0;	/* returned by re_comp/regcmp */
#ifndef NDEBUG
    static int done_search = 0;
#endif
    char *newsubject;		/* subject of current line */
    char *oldString, *newString; /* strings used to build up new text string */
    char *newLine;
    char *newSubjectString;
    extern void abortClear();
    extern int abortP();

    oldString = NIL(char);

    abortClear();
    cancelCreate();

    if (expr != NIL(char)) {
#ifdef SYSV_REGEX
	FREE(reRet);
	if ((reRet = regcmp(expr, NULL)) == NULL)
#else
	if ((reRet = re_comp(expr)) != NULL)
#endif
        {
	    /* bad regular expression */
#ifdef SYSV_REGEX
	    mesgPane(XRN_SERIOUS, UNKNOWN_REGEXP_ERROR_MSG, expr);
#else
	    mesgPane(XRN_SERIOUS, KNOWN_REGEXP_ERROR_MSG, expr, reRet);
#endif
	    failedSearch();
	    cancelDestroy();
	    return ERROR;
	}
#ifndef NDEBUG
	done_search++;
#endif
    }
#ifndef NDEBUG
    else {
	assert(done_search);
    }
#endif

    if (dir == FORWARD) {
	for (;;) {
	    if (abortP()) {
		cancelDestroy();
		return ABORT;
	    }
	    if ((*tstring)[*position] == '\0') {
		cancelDestroy();
		if (*position == 0) {

		    /* the string is null, no more articles are left */

		    return EXIT;
		}
		return NOMATCH;
	    }
	    (void) moveCursor(FORWARD, *tstring, position);
	    if ((*tstring)[*position] == '\0') {

		/* reached end of string */
		cancelDestroy();
		return NOMATCH;
	    }
	    *artNum = atol(&(*tstring)[*position + 2]);
	    newsubject = getSubject(*artNum);

#ifdef SYSV_REGEX
	    if (regex(reRet, newsubject) != NULL)
#else
	    if (re_exec(newsubject))
#endif
            {
		/* found a match to the regular expression */

		gotoArticle(*artNum);
		if (getArticle(file, ques) != XRN_OKAY) {
		    /* the matching article was invalid */

		    mesgPane(XRN_SERIOUS, ART_NOT_AVAIL_MSG, *artNum);

		    removeLine(*tstring, Text, &tsource, *position, &ttop);
		    continue;
		}
		cancelDestroy();
		return MATCH;
	    }
	}
    } else {
	startSearch();
	for (;;) {
	    if (abortP()) {

		/* reset pointers back to where we began, since the */
		/* search was aborted */

		failedSearch();
		cancelDestroy();
		return ABORT;
	    }
	    if ((*position == 0) && ((*tstring)[*position] == '\0')) {

		/* no more articles remain, return to Newgroup mode */
		cancelDestroy();
		return EXIT;
	    }
	    if (*position != 0) {

		/* we are still within the subject list */

		(void) moveCursor(BACK, *tstring, position);
		*artNum = atol(&(*tstring)[*position + 2]);
		newsubject = getSubject(*artNum);

#ifdef SYSV_REGEX
		if (regex(reRet, newsubject) != NULL)
#else
		if (re_exec(newsubject))
#endif
                {
		    /* an article matching the regular expression was found */

		    gotoArticle(*artNum);
		    if (getArticle(file, ques) != XRN_OKAY) {
			/* article is invalid, remove it from the text string*/

			mesgPane(XRN_SERIOUS, ART_NOT_AVAIL_MSG, *artNum);

			removeLine(*tstring, Text, &tsource, *position, &ttop);
			continue;
		    }
		    cancelDestroy();
		    return MATCH;
		}
	    } else {

		/* must query the news server for articles not shown */
		/* on the current subject screen */

		if ((newLine = getPrevSubject()) == NIL(char)) {
		    
		    /* all articles have been exhausted, reset variables */
		    /* to what they were before the search was started */

		    failedSearch();
		    cancelDestroy();
		    return NOMATCH;
		}
		newLine[0] = '+';
		*artNum = atol(&newLine[2]);
		newsubject = getSubject(*artNum);
		if (oldString != NIL(char)) {

		    /* add the newest subject line (newLine) to the */
		    /* list of new subject lines (oldString) we are */
		    /* building up.  Put the result in newString.   */

		    newString = ARRAYALLOC(char, (utStrlen(oldString) + utStrlen(newLine) + 2));
		    (void) strcpy(newString, newLine);
		    (void) strcat(newString, "\n");
		    (void) strcat(newString, oldString);
		    FREE(oldString);
		} else {

		    /* the first new subject line has been obtained, */
		    /* allocate space and save it */

		    newString = ARRAYALLOC(char, (utStrlen(newLine) + 2));
		    (void) strcpy(newString, newLine);
		    (void) strcat(newString, "\n");
		}

#ifdef SYSV_REGEX
		if (regex(reRet, newsubject) != NULL)
#else
		if (re_exec(newsubject))
#endif
                {
		    /* the new article (subjectline) obtained here */
		    /* does not need to be checked for validity, since */
		    /* getPrevSubject() only returns valid articles */

		    /* re-build the entire subject string, adding the */
		    /* original subject string to the newly obtained */
		    /* subjects. Return an indication that the subject */
		    /* window must be updated. */

		    newSubjectString = ARRAYALLOC(char, (utStrlen(newString) + utStrlen(*tstring)+ 1));
		    (void) strcpy(newSubjectString, newString);
		    (void) strcat(newSubjectString, *tstring);
		    FREE(*tstring);
		    *tstring = newSubjectString;
		    gotoArticle(*artNum);
		    if (getArticle(file, ques) != XRN_OKAY) {
			mesgPane(XRN_SERIOUS, ART_NOT_AVAIL_MSG, *artNum);
		    }
		    cancelDestroy();
		    return WINDOWCHANGE;
		}
		oldString = newString;
		continue;
	    }
	}
    }
}


int moveToArticle(artNum, file, ques)
    long artNum;			/* number of new article */
    char **file, **ques;		/* filename and status line for new article */
{
    fillUpArray(artNum);

    if (checkArticle(artNum) != XRN_OKAY) {
	return NOMATCH;
    }

    gotoArticle(artNum);
    if (getArticle(file, ques) != XRN_OKAY) {
	return ERROR;
    }

    return MATCH;
}
