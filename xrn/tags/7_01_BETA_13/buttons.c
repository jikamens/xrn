
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: buttons.c,v 1.54 1995-01-25 03:17:52 jik Exp $";
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
#include "ngMode.h"
#include "addMode.h"
#include "allMode.h"
#include "artMode.h"

#ifndef O_RDONLY
#define O_RDONLY 0
#endif

static Widget *AddButtons = NIL(Widget);
static Widget *NgButtons = NIL(Widget);
static Widget *AllButtons = NIL(Widget);
static Widget *ArtButtons = NIL(Widget);
static Widget *ArtSpecButtons = NIL(Widget);

long First, Last;	/* keeps the beginning and end of the */
				/* selected text for the "move groups" command */

/* Action to take when a confirmation box is clicked in */
static void (*ConfirmAction) _ARGUMENTS((void));


int CurrentMode = NO_MODE;            /* current mode                       */
int PreviousMode = NO_MODE;    /* previous mode, what buttons to */
				       /* remove */

#define XRN_NO 0
#define XRN_YES 1

/* the user is in a command - eat type ahead */
int inCommand = 0;

static struct _translations {
    Widget widget;
    XtTranslations tables[MAX_MODE];
    char *unparsed[MAX_MODE];
} Translations[6];

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
	info(TopNonButtonInfo);
    else if (event->type == EnterNotify)
	info(client_data);

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

    if (event->type == LeaveNotify) {
	XtSetArg(infoLineArg[0], XtNlabel, BottomNonButtonInfo);
    } else if (event->type == EnterNotify) {
	XtSetArg(infoLineArg[0], XtNlabel, client_data);
    }
    XtSetValues(BottomInfoLine, infoLineArg, XtNumber(infoLineArg));
    return;
}

void resetSelection()
/*
 * Reset First and Last to zero, so the user doesn't accidentally
 * move groups
 */
{
    First = 0;
    Last = 0;
    
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
    info(message);
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
    Arg infoLineArg[1];

    XtSetArg(infoLineArg[0], XtNlabel, message);
    (void) strcpy(BottomNonButtonInfo, (char *) infoLineArg[0].value);
    XtSetValues(BottomInfoLine, infoLineArg, XtNumber(infoLineArg));
    return;
}

#undef setBottomInfoLine

#define TOP	0
#define BOTTOM	1

static void doButtons _ARGUMENTS((char *, Widget, Widget *, ButtonList *,
				  int *, int));

static void doButtons(resource, box, buttons, buttonList, size, infoLine)
    char *resource;
    Widget box;
    Widget *buttons;
    ButtonList *buttonList;
    int *size;
    int infoLine;
{
    char *ptr, *token;
    int j, i = 0;

    if (resource) {
	ptr = resource;

	while ((token = strtok(ptr, ", \t\n")) != NIL(char)) {
	    /* find name */
	    for (j = 0; j < *size; j++) {
		if (STREQ(token, (char *) buttonList[j].buttonArgs[0].value)) {
		    buttons[i] = XtCreateWidget((char *) buttonList[j].buttonArgs[0].value,
						  commandWidgetClass,
						  box,
						  buttonList[j].buttonArgs,
						  buttonList[j].size);
		    if (infoLine == TOP) {
			setTopInfoLineHandler(buttons[i], buttonList[j].message);
		    } else {
			setBottomInfoLineHandler(buttons[i], buttonList[j].message);
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
	    buttons[i] = XtCreateWidget((char *) buttonList[i].buttonArgs[0].value,
					  commandWidgetClass,
					  box,
					  buttonList[i].buttonArgs,
					  buttonList[i].size);
	    if (infoLine == TOP) {
		setTopInfoLineHandler(buttons[i], buttonList[i].message);
	    } else {
		setBottomInfoLineHandler(buttons[i], buttonList[i].message);
	    }
	}
    }
    return;
}


void createButtons()  
{
#define SETTRANSLATIONS(w, index, mode, bind) \
    Translations[index].widget = w; \
    Translations[index].unparsed[mode] = bind;

    SETTRANSLATIONS(TopButtonBox, 0, ADD_MODE, app_resources.addBindings);
    SETTRANSLATIONS(BottomButtonBox, 1, ADD_MODE, app_resources.addBindings);
    SETTRANSLATIONS(TopInfoLine, 2, ADD_MODE, app_resources.addBindings);
    SETTRANSLATIONS(BottomInfoLine, 3, ADD_MODE, app_resources.addBindings);
    SETTRANSLATIONS(Text, 4, ADD_MODE, app_resources.addBindings);
    SETTRANSLATIONS(ArticleText, 5, ADD_MODE, app_resources.addBindings);

    SETTRANSLATIONS(TopButtonBox, 0, ALL_MODE, app_resources.allBindings);
    SETTRANSLATIONS(BottomButtonBox, 1, ALL_MODE, app_resources.allBindings);
    SETTRANSLATIONS(TopInfoLine, 2, ALL_MODE, app_resources.allBindings);
    SETTRANSLATIONS(BottomInfoLine, 3, ALL_MODE, app_resources.allBindings);
    SETTRANSLATIONS(Text, 4, ALL_MODE, app_resources.allBindings);
    SETTRANSLATIONS(ArticleText, 5, ALL_MODE, app_resources.allBindings);

    SETTRANSLATIONS(TopButtonBox, 0, NEWSGROUP_MODE, app_resources.ngBindings);
    SETTRANSLATIONS(BottomButtonBox, 1, NEWSGROUP_MODE, app_resources.ngBindings);
    SETTRANSLATIONS(TopInfoLine, 2, NEWSGROUP_MODE, app_resources.ngBindings);
    SETTRANSLATIONS(BottomInfoLine, 3, NEWSGROUP_MODE, app_resources.ngBindings);
    SETTRANSLATIONS(Text, 4, NEWSGROUP_MODE, app_resources.ngBindings);
    SETTRANSLATIONS(ArticleText, 5, NEWSGROUP_MODE, app_resources.ngBindings);

    SETTRANSLATIONS(TopButtonBox, 0, ARTICLE_MODE, app_resources.artBindings);
    SETTRANSLATIONS(BottomButtonBox, 1, ARTICLE_MODE, app_resources.artBindings);
    SETTRANSLATIONS(TopInfoLine, 2, ARTICLE_MODE, app_resources.artBindings);
    SETTRANSLATIONS(BottomInfoLine, 3, ARTICLE_MODE, app_resources.artBindings);
    SETTRANSLATIONS(Text, 4, ARTICLE_MODE, app_resources.artBindings);
    SETTRANSLATIONS(ArticleText, 5, ARTICLE_MODE, app_resources.artBindings);

    XtAppAddActions(TopContext, TopActions, XtNumber(TopActions));
    
    AddButtons = ARRAYALLOC(Widget, AddButtonListCount);
    XtAppAddActions(TopContext, AddActions, AddActionsCount);

    doButtons(app_resources.addButtonList, TopButtonBox, AddButtons, AddButtonList, &AddButtonListCount, TOP);

    NgButtons = ARRAYALLOC(Widget, NgButtonListCount);
    XtAppAddActions(TopContext, NgActions, NgActionsCount);
    
    doButtons(app_resources.ngButtonList, TopButtonBox, NgButtons, NgButtonList, &NgButtonListCount, TOP);

    AllButtons = ARRAYALLOC(Widget, AllButtonListCount);
    XtAppAddActions(TopContext, AllActions, AllActionsCount);
    
    doButtons(app_resources.allButtonList, BottomButtonBox, AllButtons, AllButtonList, &AllButtonListCount, BOTTOM);
    
    ArtButtons = ARRAYALLOC(Widget, ArtButtonListCount);
    XtAppAddActions(TopContext, ArtActions, ArtActionsCount);
    
    doButtons(app_resources.artButtonList, TopButtonBox, ArtButtons, ArtButtonList, &ArtButtonListCount, TOP);
    
    ArtSpecButtons = ARRAYALLOC(Widget, ArtSpecButtonListCount);
    
    doButtons(app_resources.artSpecButtonList, BottomButtonBox, ArtSpecButtons, ArtSpecButtonList, &ArtSpecButtonListCount, BOTTOM);

    return;
}


#ifndef DONT_FORCE_BUTTON_TRANSLATIONS
/* mikey: This is a hack to make key bindings work correctly when the */
/* buttons have focus, but I only support the top button box */

static void matchTranslations _ARGUMENTS((Widget, XtTranslations));

static void matchTranslations(w, t)
    Widget w;
    XtTranslations t;
{
    int i;

    if (w == TopButtonBox) {
	for (i=0; i<ArtButtonListCount; i++) {
	    XtOverrideTranslations(ArtButtons[i], t);
	}
	for (i=0; i<NgButtonListCount; i++) {
	    XtOverrideTranslations(NgButtons[i], t);
	}
    }
}
#endif /* ! DONT_FORCE_BUTTON_TRANSLATIONS */

static void setTranslations _ARGUMENTS((int));

static void setTranslations(mode)
    int mode;
{
#if defined(TRANSLATIONBUG) || defined(TRANSLATIONS_NOT_FREED)
    Arg args[1];
#endif
    static int init[MAX_MODE] = {0,0,0,0};
    int i;

    if (!init[mode]) {
	/*
	 * first time:
	 *   parse table
	 *   override
	 *   XXX gone - get table back and store
	 */
	for (i = 0; i < sizeof(Translations) / sizeof(struct _translations); i++) {
	    XtTranslations table;

	    if (Translations[i].unparsed[mode] == NIL(char)) {
		table = 0;
	    } else {
		table = XtParseTranslationTable(Translations[i].unparsed[mode]);
	    }
#ifdef TRANSLATIONBUG
	    if (table) {
		XtTranslations translations;

		XtSetArg(args[0], XtNtranslations, (XtPointer) &translations);
		XtGetValues(Translations[i].widget, args, XtNumber(args));
		if (! translations) {
		    XtSetArg(args[0], XtNtranslations, table);
		    XtSetValues(Translations[i].widget, args, XtNumber(args));
		}
	    }
#endif
	    if (table) {
		XtOverrideTranslations(Translations[i].widget, table);
	    }
#ifdef TRANSLATIONS_NOT_FREED
            XtSetArg(args[0], XtNtranslations,
		     (XtPointer) &Translations[i].tables[mode]);
            XtGetValues(Translations[i].widget, args, XtNumber(args));
            /* instead of the previous two lines:
             * Translations[i].tables[mode] = table;
             * however, this seems to lose bindings...
             */
#else
            Translations[i].tables[mode] = table;
#endif
#ifndef DONT_FORCE_BUTTON_TRANSLATIONS
	    matchTranslations(Translations[i].widget,
			      Translations[i].tables[mode]);
#endif
	}
	init[mode] = 1;
    } else {
	/*
	 * second and future times:
	 *   install translations
	 */
	for (i = 0; i < sizeof(Translations) / sizeof(struct _translations); i++) {
	    if (Translations[i].tables[mode]) {
#ifdef TRANSLATIONS_NOT_FREED
		XtSetArg(args[0], XtNtranslations, Translations[i].tables[mode]);
		XtSetValues(Translations[i].widget, args, XtNumber(args));
#else
		XtOverrideTranslations(Translations[i].widget, 
				       Translations[i].tables[mode]);
#endif
#ifndef DONT_FORCE_BUTTON_TRANSLATIONS
	    matchTranslations(Translations[i].widget,
			      Translations[i].tables[mode]);
#endif
	    }
	}
    }
    return;
}

void swapMode()
/*
 * change the buttons displayed in the TopButtonBox (switch modes)
 */
{
    if (PreviousMode == CurrentMode) {
	return;
    }

    XawPanedSetRefigureMode(Frame, False);
    
    /*
     * NONE -> ADD
     *    manage add in top box
     *    manage art in bottom box
     *    desensitize bottom box
     *    install add actions in top box
     */
    if ((PreviousMode == NO_MODE) && (CurrentMode == ADD_MODE)) {
	XtManageChildren(AddButtons, AddButtonListCount);
	XtManageChildren(ArtSpecButtons, ArtSpecButtonListCount);
	XtSetSensitive(BottomButtonBox, False);
	setTranslations(ADD_MODE);
	TextSetLineSelections(Text);
    /*    
     * NONE -> NG
     *    manage ng in top box
     *    manage art in bottom box
     *    desensitize bottom box
     *    install ng actions in top box
     */
    } else if ((PreviousMode == NO_MODE) && (CurrentMode == NEWSGROUP_MODE)) {
	XtManageChildren(NgButtons, NgButtonListCount);
	XtManageChildren(ArtSpecButtons, ArtSpecButtonListCount);
	XtSetSensitive(BottomButtonBox, False);
	setTranslations(NEWSGROUP_MODE);
	TextSetLineSelections(Text);
    /*
     * ADD -> NG
     *    unmanage add in top box
     *    manage ng in top box
     *    install ng actions in top box
     */
    } else if ((PreviousMode == ADD_MODE) && (CurrentMode == NEWSGROUP_MODE)) {
	XtUnmanageChildren(AddButtons, AddButtonListCount);
	XtManageChildren(NgButtons, NgButtonListCount);
	setTranslations(NEWSGROUP_MODE);
	TextSetLineSelections(Text);
    /*
     * NG -> ART
     *    unmanage ng in top box
     *    manage art in top box
     *    sensitize bottom box
     *    install art actions in top box
     *    install art actions in bottom box
     */
    } else if ((PreviousMode == NEWSGROUP_MODE) && (CurrentMode == ARTICLE_MODE)) {
	XtUnmanageChildren(NgButtons, NgButtonListCount);
	XtManageChildren(ArtButtons, ArtButtonListCount);
	XtSetSensitive(BottomButtonBox, True);
	setTranslations(ARTICLE_MODE);
	ArticleNewsGroupsString = TextGetString(Text);
	TextSetLineSelections(Text);
	TextSetAllSelections(ArticleText);
    /*
     * NG -> ADD
     *    unmanage ng in top box
     *    manage add in top box
     *    install add actions in top box
     */
    } else if ((PreviousMode == NEWSGROUP_MODE) && (CurrentMode == ADD_MODE)) {
	XtUnmanageChildren(NgButtons, NgButtonListCount);
	XtManageChildren(AddButtons, AddButtonListCount);
	setTranslations(ADD_MODE);
	TextSetLineSelections(Text);
    /*
     * NG -> ALL
     *    unmanage ng in top box
     *    unmanage ng in bottom box
     *    manage all in bottom box
     *    sensitize bottom box
     *    install all actions in bottom box
     */
    } else if ((PreviousMode == NEWSGROUP_MODE) && (CurrentMode == ALL_MODE)) {
	XtUnmanageChildren(NgButtons, NgButtonListCount);
	XtUnmanageChildren(ArtSpecButtons, ArtSpecButtonListCount);
	XtManageChildren(AllButtons, AllButtonListCount);
	XtSetSensitive(BottomButtonBox, True);
	setTranslations(ALL_MODE);
	TextClear(Text);
	TextSetLineSelections(ArticleText);
    /*     
     * ART -> NG
     *    desensitize bottom box
     *    unmanage art in top box
     *    manage ng in top box
     *    install ng actions in top box
     */
    } else if ((PreviousMode == ARTICLE_MODE) && (CurrentMode == NEWSGROUP_MODE)) {
	XtSetSensitive(BottomButtonBox, False);
	XtUnmanageChildren(ArtButtons, ArtButtonListCount);
	XtManageChildren(NgButtons, NgButtonListCount);
	setTranslations(NEWSGROUP_MODE);
	FREE(ArticleNewsGroupsString);
	TextClear(ArticleText);
	TextSetLineSelections(Text);
    /*
     * ALL -> NG
     *    manage ng in top box
     *    unmanage all in bottom box
     *    manage art in bottom box
     *    desensitize bottom box
     */
    } else if ((PreviousMode == ALL_MODE) && (CurrentMode == NEWSGROUP_MODE)) {
	XtManageChildren(NgButtons, NgButtonListCount);
	XtUnmanageChildren(AllButtons, AllButtonListCount);
	XtManageChildren(ArtSpecButtons, ArtSpecButtonListCount);
	XtSetSensitive(BottomButtonBox, False);
	setTranslations(NEWSGROUP_MODE);
	TextClear(ArticleText);
	TextSetLineSelections(Text);
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
	XtUnmanageChildren(ArtSpecButtons, ArtSpecButtonListCount);
	XtUnmanageChildren(ArtButtons, ArtButtonListCount);
	XtManageChildren(AllButtons, AllButtonListCount);
	XtManageChildren(NgButtons, NgButtonListCount);
	XtSetSensitive(TopButtonBox, False);
	setTranslations(ALL_MODE);
	FREE(ArticleNewsGroupsString);
	TextClear(Text);
	TextSetLineSelections(ArticleText);
    /*	
     * ALL -> ART
     *    manage art in top box
     *    unmanage all in bottom box
     *    manage art in bottom box
     *    install art actions in bottom box
     */
    } else if ((PreviousMode == ALL_MODE) && (CurrentMode == ARTICLE_MODE)) {
	XtManageChildren(ArtButtons, ArtButtonListCount);
	XtUnmanageChildren(AllButtons, AllButtonListCount);
	XtManageChildren(ArtSpecButtons, ArtSpecButtonListCount);
	setTranslations(ARTICLE_MODE);
	TextSetLineSelections(Text);
	TextSetAllSelections(ArticleText);
    } else {
      (void) sprintf(error_buffer, ERROR_UNSUP_TRANS_MSG ,
			       PreviousMode, CurrentMode);
	ehErrorExitXRN(error_buffer);
    }

    XawPanedSetRefigureMode(Frame, True);
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
    busyCursor();
    switch (CurrentMode) {
    case ALL_MODE:
	allScrollFunction(NULL, event, NULL, NULL);
	break;

    case NEWSGROUP_MODE:
	if (count && *count == 1 && strcmp(string[0], "jump") == 0) {
	    NewsgroupEntryMode = NG_ENTRY_JUMP;
	}
	ngReadFunction(NULL, NULL, NULL, NULL);
	break;

    case ARTICLE_MODE:
	if (event &&
	    (event->type == ButtonPress || event->type == ButtonRelease)) {
	    artNextFunction(NULL, NULL, NULL, NULL);
	    break;
	}
	if (!app_resources.pageArticles) {
	    if (app_resources.subjectRead == False) {
		artNextUnreadFunction(NULL, NULL, NULL, NULL);
	    } else {
		artSubNextFunction(NULL, NULL, NULL, NULL);
	    }
	} else {
	    if (TextLastPage(ArticleText)) {
	      next_article:
		if (app_resources.subjectRead == False) {
		    artNextUnreadFunction(NULL, NULL, NULL, NULL);
		} else {
		    artSubNextFunction(NULL, NULL, NULL, NULL);
		}
	    }
	    else {
		artScrollFunction(0, 0, 0, 0);
		if (TextPastLastPage(ArticleText))
		    goto next_article;
	    }
	}
	break;
    }
    unbusyCursor();
    inCommand = 0;
    return;
}

/*
  Adjust the upper text window so that the number of lines above the
  cursor is greater than or equal to minLines and less than or equal
  to maxLines, if possible.  If the number of lines is within the
  valid range, don't do anything; otherwise, scroll so that
  defaultLines are above the cursor, if defaultLines is within the
  valid range, or just enough to put us within the valid range,
  otherwise.  In any case, the cursor will be visible when done.
  */
void adjustMinMaxLines(IndexString)
    String IndexString;
{
    long CursorPosition = TextGetInsertionPoint(Text);
    long top = TextGetTopPosition(Text), TopPosition = top, end;
    int height = TextGetLines(Text);
    int min = app_resources.minLines, max = app_resources.maxLines;
    int def = app_resources.defaultLines;
    int numLines, below;

    if (height == 0)
	return;
    if (max < 0)
	max += height;

    /*
      Sanitize the parameters.
      */
    if (min < 1)
	min = 1;
    if (max > height)
	max = height;
    if (min > max)
	min = max;
    if (def < min)
	def = min;
    if (def > max)
	def = max;
    /*
      If the cursor isn't even currently visible, it should be.
      */
    if (top > CursorPosition)
	top = CursorPosition;

    /*
      Figure out how many lines are above the cursor.
      */
    for (numLines = 1; top < CursorPosition; numLines++) {
	assert(moveCursor(FORWARD, IndexString, &top));
    }

    /*
      Figure out how many lines are below the cursor.
      */
    below = 0;
    end = top;
    while (moveCursor(FORWARD, IndexString, &end) && (below < height))
	below++;

    /*
      If necessary, reposition.
      */
    if ((numLines < min) || (max < numLines)) {
	for (numLines = 1; numLines < def; numLines++) {
	    if (! moveCursor(BACK, IndexString, &top))
		break;
	    
	}
	for (below += numLines; below <= height; below++) {
	    if (! moveCursor(BACK, IndexString, &top))
		break;
	}
	if (TopPosition != top)
	    TextSetTopPosition(Text, top);
    }
}

    

Boolean watchingGroup(newsgroup)
    char *newsgroup;
{
    static int inited = 0;
    static char **GroupList;
    char **p;

    if (! inited) {
	GroupList = parseRegexpList(app_resources.watchList, "watchUnread");
	inited++;
    }

    if (newsgroup == 0)
	return False;
    if (! GroupList)
	return True;

    for (p = GroupList; *p; p++) {
#ifdef SYSV_REGEX
	if (regex(*p, newsgroup))
	    return True;
#else
	re_comp(*p);
	if (re_exec(newsgroup))
	    return True;
#endif
    }

    return False;
}


static String anyIterator(string, group, start, delete, out_left)
    String string;
    Boolean group, start, delete;
    long *out_left;
{
    static char name[GROUP_NAME_SIZE];
    static long left, right;
    Widget w;

    if (CurrentMode == ALL_MODE)
	w = ArticleText;
    else
	w = Text;

    if (start) {
	if (TextGetSelectedOrCurrentLines(w, &left, &right)) {
	    TextUnsetSelection(w);
	    if (out_left)
		*out_left = left;
	    return name;
	}
	return 0;
    }

    if (out_left)
	*out_left = left;

    if (left >= right)
	return 0;

    if (group)
	currentGroup(CurrentMode, string, name, left);
    else
	(void) sprintf(name, "%ld", atol(&string[left] + 2));

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

String newsgroupIterator(string, start, delete, left)
    String string;
    Boolean start, delete;
    long *left;
{
    return anyIterator(string, True, start, delete, left);
}

String articleIterator(string, start, delete, left)
    String string;
    Boolean start, delete;
    long *left;
{
    return anyIterator(string, False, start, delete, left);
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
    busyCursor();
    PopDownDialog(ConfirmBox);
    ConfirmBox = 0;

    if ((int) client_data == XRN_YES)
	(*ConfirmAction)();

    unbusyCursor();
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
    resetSelection();
    
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

    setBottomInfoLine("");

    if ((string = newGroups())) {
	switchToAddMode(string);
	FREE(string);
    }
    else
	switchToNewsgroupMode(False);

    return;
}
