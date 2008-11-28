
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: buttons.c,v 1.66 1995-06-08 02:05:07 jik Exp $";
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
 * buttons.c: create and handle the buttons
 *
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <assert.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Dialog.h>

#include "compose.h"
#include "cursor.h"
#include "mesg.h"
#include "dialogs.h"
#include "modes.h"
#include "resources.h"
#include "news.h"
#include "internals.h"
#include "save.h"
#include "xmisc.h"
#include "error_hnds.h"
#include "xthelper.h"
#include "xrn.h"
#include "cancel.h"
#include "buttons.h"
#include "butdefs.h"
#include "mesg_strings.h"
#include "newsrcfile.h"
#include "butexpl.h"
#include "Text.h"
#include "ButtonBox.h"
#include "ngMode.h"
#include "addMode.h"
#include "artMode.h"
#include "allMode.h"

#ifndef O_RDONLY
#define O_RDONLY 0
#endif

/* Action to take when a confirmation box is clicked in */
static void (*ConfirmAction) _ARGUMENTS((void));


int CurrentMode = NO_MODE;            /* current mode                       */
int PreviousMode = NO_MODE;    /* previous mode, what buttons to */
				       /* remove */

#define XRN_NO 0
#define XRN_YES 1

/* the user is in a command - eat type ahead */
int inCommand = 0;
int inSubCommand = 0;

void doTheRightThing _ARGUMENTS((Widget, XEvent *,String *,Cardinal *));
void doPrefetch _ARGUMENTS((Widget, XEvent *, String *, Cardinal *));

static XtActionsRec TopActions[] = {
    {"doTheRightThing",	doTheRightThing},
    {"doPrefetch", doPrefetch},
};

static char TopNonButtonInfo[LABEL_SIZE];
static char BottomNonButtonInfo[LABEL_SIZE];


/*
 * handle the Enter and Leave events for the buttons
 *
 * upon entering a button, get it's info string and put in the Question label
 * upon leaving a button, restore the old info string
 *
 */
/*ARGSUSED*/
#if XtSpecificationRelease > 4
static void topInfoHandler _ARGUMENTS((Widget, XtPointer, XEvent *,
				       Boolean *));

static void topInfoHandler(widget, client_data, event, dispatch)
#else
static void topInfoHandler _ARGUMENTS((Widget, XtPointer, XEvent *));

static void topInfoHandler(widget, client_data, event)
#endif /* XtSpecificationRelease > 4 */
    Widget widget;
    XtPointer client_data;
    XEvent *event;
#if XtSpecificationRelease > 4
    Boolean *dispatch;
#endif /* XtSpecificationRelease > 4 */
{
    if (event->type == LeaveNotify)
	INFO(TopNonButtonInfo);
    else if (event->type == EnterNotify)
	INFO(client_data);

    return;
}

/*
 * handle the Enter and Leave events for the buttons
 *
 * upon entering a button, get it's info string and put in the Question label
 * upon leaving a button, restore the old info string
 *
 */
/*ARGSUSED*/
#if XtSpecificationRelease > 4
static void bottomInfoHandler _ARGUMENTS((Widget, XtPointer, XEvent *,
					  Boolean *));

static void bottomInfoHandler(widget, client_data, event, dispatch)
#else
static void bottomInfoHandler _ARGUMENTS((Widget, XtPointer, XEvent *));

static void bottomInfoHandler(widget, client_data, event)
#endif /* XtSpecificationRelease > 4 */
    Widget widget;
    XtPointer client_data;
    XEvent *event;
#if XtSpecificationRelease > 4
    Boolean *dispatch;
#endif /* XtSpecificationRelease > 4 */
{
    Arg infoLineArg[1];

    if (! BottomInfoLine)
	return;

    if (event->type == LeaveNotify) {
	XtSetArg(infoLineArg[0], XtNlabel, BottomNonButtonInfo);
    } else if (event->type == EnterNotify) {
	XtSetArg(infoLineArg[0], XtNlabel, client_data);
    }
    XtSetValues(BottomInfoLine, infoLineArg, XtNumber(infoLineArg));
    return;
}

static void setTopInfoLineHandler _ARGUMENTS((Widget, char *));

static void setTopInfoLineHandler(widget, message)
    Widget widget;
    char *message;
{
    XtAddEventHandler(widget,
		      (EventMask) (EnterWindowMask|LeaveWindowMask),
		      False,
		      (XtEventHandler) topInfoHandler,
		      (XtPointer) message);
    return;
}


static void setBottomInfoLineHandler _ARGUMENTS((Widget, char *));

static void setBottomInfoLineHandler(widget, message)
    Widget widget;
    char *message;
{
    XtAddEventHandler(widget,
		      (EventMask) (EnterWindowMask|LeaveWindowMask),
		      False,
		      (XtEventHandler) bottomInfoHandler,
		      (XtPointer) message);
    return;
}


#ifdef SWITCH_TOP_AND_BOTTOM
#define setTopInfoLine setBottomInfoLine
#endif

void setTopInfoLine(message)  
    char *message;
{
    INFO(message);
    (void) strcpy(TopNonButtonInfo, message);
    return;
}

#undef setTopInfoLine

#ifdef SWITCH_TOP_AND_BOTTOM
#define setBottomInfoLine setTopInfoLine
#endif

void setBottomInfoLine(message)  
    char *message;
{
    (void) strcpy(BottomNonButtonInfo, message);

    if (! BottomInfoLine)
	return;
    
    XtVaSetValues(BottomInfoLine, XtNlabel, message, 0);
    return;
}

#undef setBottomInfoLine

void setButtonSensitive(box, name, sensitive)
    Widget box;
    char *name;
    Boolean sensitive;
{
    Widget w;

    if (! (w = XtNameToWidget(box, name)))
	return;

    XtVaSetValues(w, XtNsensitive, sensitive, 0);
}

    
void doButtons(resource, box, buttonList, size, infoLine)
    char *resource;
    Widget box;
    ButtonList *buttonList;
    int *size;
    int infoLine;
{
    char *ptr, *token;
    int j, i = 0;
    Widget button;

    if (resource) {
	ptr = resource;

	while ((token = strtok(ptr, ", \t\n")) != NIL(char)) {
	    /* find name */
	    for (j = 0; j < *size; j++) {
		if (STREQ(token, (char *) buttonList[j].name)) {
		    button = ButtonBoxAddButton(buttonList[j].name,
						buttonList[j].callbacks, box);
		    if (infoLine == TOP) {
			setTopInfoLineHandler(button, buttonList[j].message);
		    } else {
			setBottomInfoLineHandler(button,
						 buttonList[j].message);
		    }
		    i++;
		    break;
		}
	    }
	    if (j == *size) {
		mesgPane(XRN_SERIOUS, 0, BAD_BUTTON_NAME_MSG, token);
	    }
	    ptr = NIL(char);
	}
	*size = i;
	
    } else {
	for (i = 0; i < *size; i++) {
	    button = ButtonBoxAddButton(buttonList[i].name,
					buttonList[i].callbacks, box);
	    if (infoLine == TOP) {
		setTopInfoLineHandler(button, buttonList[i].message);
	    } else {
		setBottomInfoLineHandler(button, buttonList[i].message);
	    }
	}
    }
    ButtonBoxDoneAdding(box);
    return;
}


void createButtons()  
{
#define SETTRANSLATIONS(w, index, mode, bind) \
    Translations[index].widget = w; \
    Translations[index].unparsed[mode] = bind;

    XtAppAddActions(TopContext, TopActions, XtNumber(TopActions));
    XtAppAddActions(TopContext, AllActions, AllActionsCount);
    XtAppAddActions(TopContext, NgActions, NgActionsCount);
    XtAppAddActions(TopContext, ArtActions, ArtActionsCount);
    XtAppAddActions(TopContext, AddActions, AddActionsCount);

    return;
}


void hideGenericWidgets()
{
    XtDestroyWidget(Frame);
}

void swapMode()
/*
 * change the buttons displayed in the TopButtonBox (switch modes)
 */
{
    if (PreviousMode == CurrentMode) {
	return;
    }

    /*
     * NONE -> ADD
     *    manage add in top box
     *    manage art in bottom box
     *    desensitize bottom box
     *    install add actions in top box
     */
    if ((PreviousMode == NO_MODE) && (CurrentMode == ADD_MODE)) {
	hideGenericWidgets();

	displayAddWidgets();
    /*    
     * NONE -> NG
     *    manage ng in top box
     *    manage art in bottom box
     *    desensitize bottom box
     *    install ng actions in top box
     */
    } else if ((PreviousMode == NO_MODE) && (CurrentMode == NEWSGROUP_MODE)) {
	hideGenericWidgets();

	displayNewsgroupWidgets();
    /*
     * ADD -> NG
     *    unmanage add in top box
     *    manage ng in top box
     *    install ng actions in top box
     */
    } else if ((PreviousMode == ADD_MODE) && (CurrentMode == NEWSGROUP_MODE)) {
	hideAddWidgets();
	
	displayNewsgroupWidgets();
    /*
     * NG -> ART
     *    unmanage ng in top box
     *    manage art in top box
     *    sensitize bottom box
     *    install art actions in top box
     *    install art actions in bottom box
     */
    } else if ((PreviousMode == NEWSGROUP_MODE) && (CurrentMode == ARTICLE_MODE)) {
	hideNewsgroupWidgets();

	displayArticleWidgets();
    /*
     * NG -> ADD
     *    unmanage ng in top box
     *    manage add in top box
     *    install add actions in top box
     */
    } else if ((PreviousMode == NEWSGROUP_MODE) && (CurrentMode == ADD_MODE)) {
	hideNewsgroupWidgets();

	displayAddWidgets();
    /*
     * NG -> ALL
     *    unmanage ng in top box
     *    unmanage ng in bottom box
     *    manage all in bottom box
     *    sensitize bottom box
     *    install all actions in bottom box
     */
    } else if ((PreviousMode == NEWSGROUP_MODE) && (CurrentMode == ALL_MODE)) {
	hideNewsgroupWidgets();

	displayAllWidgets();
    /*     
     * ART -> NG
     *    desensitize bottom box
     *    unmanage art in top box
     *    manage ng in top box
     *    install ng actions in top box
     */
    } else if ((PreviousMode == ARTICLE_MODE) && (CurrentMode == NEWSGROUP_MODE)) {
	hideArticleWidgets();

	displayNewsgroupWidgets();
    /*
     * ALL -> NG
     *    manage ng in top box
     *    unmanage all in bottom box
     *    manage art in bottom box
     *    desensitize bottom box
     */
    } else if ((PreviousMode == ALL_MODE) && (CurrentMode == NEWSGROUP_MODE)) {
	hideAllWidgets();

	displayNewsgroupWidgets();
    /*
     * ART -> ALL (going back to previous ALL_MODE)
     *    unmanage art in bottom box
     *    unmanage art in top box
     *    manage all in bottom box
     *    manage ng in top box
     *    desensitize top box
     *    install all actions in bottom box
     */
    } else if ((PreviousMode == ARTICLE_MODE) && (CurrentMode == ALL_MODE)) {
	hideArticleWidgets();

	displayAllWidgets();
    /*	
     * ALL -> ART
     *    manage art in top box
     *    unmanage all in bottom box
     *    manage art in bottom box
     *    install art actions in bottom box
     */
    } else if ((PreviousMode == ALL_MODE) && (CurrentMode == ARTICLE_MODE)) {
	hideAllWidgets();

	displayArticleWidgets();
    } else {
      (void) sprintf(error_buffer, ERROR_UNSUP_TRANS_MSG ,
			       PreviousMode, CurrentMode);
	ehErrorExitXRN(error_buffer);
    }

    return;
}

static int XRNAbort = 0;

int abortP()
{
    xthHandleAllPendingEvents();
    return XRNAbort;
}

void abortSet()
{
    XRNAbort = 1;
    return;
}

void abortClear()
{
    XRNAbort = 0;
    return;
}



/*ARGSUSED*/
void doTheRightThing(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (inCommand) {
	return;
    }
    inCommand = 1;
    xrnBusyCursor();
    switch (CurrentMode) {
    case ALL_MODE:
	allDoTheRightThing(widget, event, string, count);
	break;

    case NEWSGROUP_MODE:
	ngDoTheRightThing(widget, event, string, count);
	break;

    case ARTICLE_MODE:
	artDoTheRightThing(widget, event, string, count);
	break;
    }
    xrnUnbusyCursor();
    inCommand = 0;
    return;
}

    

Boolean watchingGroup(newsgroup)
    char *newsgroup;
{
    static int inited = 0;
#ifdef POSIX_REGEX
    static regex_t *GroupList;
#else
    static char **GroupList;
#endif
    static int GroupListCount;
    int p;

    if (! inited) {
	GroupList = parseRegexpList(app_resources.watchList, "watchUnread",
				    &GroupListCount);
	inited++;
    }

    if (newsgroup == 0)
	return False;
    if (! GroupList)
	return True;

    for (p = 0; p < GroupListCount; p++) {
#ifdef POSIX_REGEX
	if (! regexec(&GroupList[p], newsgroup, 0, 0, 0))
	    return True;
#else
# ifdef SYSV_REGEX
	if (regex(GroupList[p], newsgroup))
	    return True;
# else
	re_comp(GroupList[p]);
	if (re_exec(newsgroup))
	    return True;
# endif
#endif
    }

    return False;
}


String anyIterator(w, string, group, start, delete, out_left)
    Widget w;
    String string;
    Boolean group, start, delete;
    long *out_left;
{
    static char *name = 0;
    static long left, right;
    Boolean ret;

    if (start) {
	ret = TextGetSelectedOrCurrentLines(w, &left, &right);
	if (out_left)
	    *out_left = left;
	if (ret) {
	    TextUnsetSelection(w);
	    if (! name)
		name = XtRealloc(name, 1);
	    return name;
	}
	return 0;
    }

    if (out_left)
	*out_left = left;

    if (left >= right)
	return 0;

    if (group)
	currentGroup(CurrentMode, string, &name, left);
    else {
	/* Will we ever have an article number longer than 10 chars long?
	   Yeah, right. */
	name = XtRealloc(name, 11); 
	(void) sprintf(name, "%ld", atol(&string[left] + 2));
    }

    if (delete) {
	long new = left;

	if (moveCursor(FORWARD, string, &new))
	    right -= new - left;
	else
	    right = left;
	TextRemoveLine(w, left);
	removeLine(string, &left);
    }
    else if (! moveCursor(FORWARD, string, &left))
	left = right + 1;
    return name;
}

static Widget ConfirmBox = 0;

/*ARGSUSED*/
static void generalHandler _ARGUMENTS((Widget, XtPointer, XtPointer));

static void generalHandler(widget, client_data, call_data)
    Widget widget;
    XtPointer client_data;
    XtPointer call_data;
{
    if (inCommand) {
	return;
    }
    inCommand = 1;
    xrnBusyCursor();
    PopDownDialog(ConfirmBox);
    ConfirmBox = 0;

    if ((int) client_data == XRN_YES)
	(*ConfirmAction)();

    xrnUnbusyCursor();
    inCommand = 0;
    return;
}

void confirmBox(message, mode, flag, handler)
    String message;
    int mode, flag;
    void (*handler) _ARGUMENTS((void));
{
    static struct DialogArg args[] = {
        {NO_STRING,               generalHandler, (XtPointer) XRN_NO},
	{YES_STRING,              generalHandler, (XtPointer) XRN_YES},
    };

    if (CurrentMode != mode)
	return;
    
    if (app_resources.confirmMode & flag) {
	ConfirmAction = handler;
	if (! ConfirmBox) {
	    ConfirmBox = CreateDialog(TopLevel, message, DIALOG_NOTEXT,
				      args, XtNumber(args));
	    PopUpDialog(ConfirmBox);
	}

	return;
    }
    (*handler)();
}


void determineMode()
/*
 * determine the initial mode and set up Text, TopButtonBox, and Question
 */
{
    String string;

    if ((string = newGroups())) {
	switchToAddMode(string);
	FREE(string);
    }
    else
	switchToNewsgroupMode(False);

    return;
}
