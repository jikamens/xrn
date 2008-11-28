
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: cursor.c,v 1.18 1995-01-28 23:00:06 jik Exp $";
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

#include "news.h"
#include "mesg.h"
#include "internals.h"
#include "modes.h"
#include "xrn.h"
#include "cursor.h"
#include "error_hnds.h"
#include "cancel.h"
#include "mesg_strings.h"
#include "Text.h"
#include "buttons.h"

/*
 * Move the cursor to the beginning of the current line.
 * If the cursor is at the end of string marker, leave it there.
 */
void moveBeginning(tstring, point)
    char *tstring;			/* text string */
    long *point;		/* cursor position */
{
    int i = 0;
    while ((*point != 0) && (tstring[*point-1] != '\n') &&
		(tstring[*point] != '\0')) {
	(*point)--;
	i++;
	if (i > 1000) {
	    ehErrorExitXRN( ERROR_INFINITE_LOOP_MSG );
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
    long *point;		/* cursor position */
{
    int i = 0;
    while ((tstring[*point] != '\n') && (tstring[*point] != '\0')) {
	(*point)++;
	i++;
	if (i > 1000) {
	    ehErrorExitXRN( ERROR_INFINITE_LOOP_MSG );
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
    long *point;	    /* cursor position */
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
    long *point;		/* cursor position */
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
    long *point;		/* cursor position */
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
  Remove the line the cursor is on from the given string.  Leaves point
  at the beginning of the next line.
  */
void removeLine(tstring, point)
    char *tstring;			/* text string */
    long *point;	/* position on line to be removed, */
{
    long right;	/* end of line to be removed */

    moveBeginning(tstring, point);

    right = *point;
    moveEnd(tstring, &right);

    if (! tstring[right])
	tstring[*point] = '\0';
    else
	(void) strcpy(&tstring[*point], &tstring[right+1]);
}

/*
 * Leave the cursor at the current position unless it is at the
 * end of string marker, in which case move it to the beginning.
 * If the string is empty, return FALSE.
 */
int setCursorCurrent(tstring, point)
    char *tstring;			/* text string */
    long *point;		/* cursor position */
{
    if (tstring[*point] == '\0') {
	if (*point == 0) {
	    return FALSE;
	} else {
	    *point = (long) 0;
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
    long point;		/* cursor position */
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
    long point;
{
    char status[100];

    (void) sscanf(&tstring[point], "%s %s", groupName, status);
    if (strcmp(status, UNSUBED_MSG ) == 0) {
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
    long point;		/* cursor position */
{
    if (tstring[point] == ' ') {
	tstring[point] = READ_MARKER;
    }
    return atoi(&tstring[point+2]);
}

/*
 * Mark all groups on the line beginning at left subscribed or
 * unsubscribed, depending on the contents of the string status.
 */
void markAllString(tstring, left, status)
    char *tstring;			/* text string */
    long left;
    char *status;			/* "subscribed" or "unsubscribed" */
{
    int len = utStrlen(status);

    moveEnd(tstring, &left);
    left -= len;
    strncpy(&tstring[left], status, len);
    return;
}

/*
 * Mark a group of articles between left and right as read or unread.
 * Marks the articles in the text string, and marks them internally.
 */
void markArticles(tstring, left, right, marker)
    char *tstring;		      /* text string */
    long left, right;      /* boundaries of articles to be marked */
    char marker;
{
    long artNum;		/* number of current article to be marked */

    while (left < right) {
	tstring[left] = marker;
	artNum = atol(&tstring[left+2]);
	if (marker == READ_MARKER) {
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
    long first, last;       /* portion of old string to be copied */
    char *oldString;		       /* old text string */
{
    *newString = ARRAYALLOC(char, (last - first + 2));
    (void) strncpy(*newString, &oldString[first], (int) (last - first + 1));
    (*newString)[last - first + 1] = '\0';
}

/*
 Move the cursor to the position of the article "num".  "position"
 is an input/output variable; it should contain the position to start
 searching at when it is called, and will contain the new position
 when the function returns.
 */
void findArticle(tstring, num, position)
    char *tstring;			/* text string */
    art_num num;			/* article number to search for */
    long *position;		/* cursor position */
{
    long artNum;		/* number of current article */
    long pos = *position + 1;

    /* move over S[aved] / P[rinted] marking */
    if ((tstring[pos] == SAVED_MARKER) || (tstring[pos] == PRINTED_MARKER)) {
	pos++;
    }
    artNum = atol(&tstring[pos]);
    while (artNum != num) {
	if (!moveCursor(FORWARD, tstring, position)) {
	    ehErrorExitXRN( ERROR_FINDARTICLE_MSG );
	}
	pos = *position + 1;
	/* move over S[aved] / P[rinted] marking */
	if ((tstring[pos] == SAVED_MARKER) || (tstring[pos] == PRINTED_MARKER)) {
	    pos++;
	}
	artNum = atol(&tstring[pos]);
    }
    return;
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
