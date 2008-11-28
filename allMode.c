#include <X11/Intrinsic.h>
#include "buttons.h"
#include "butdefs.h"
#include "butexpl.h"
#include "allMode.h"
#include "ngMode.h"
#include "modes.h"
#include "Text.h"
#include "news.h"
#include "internals.h"
#include "cursor.h"
#include "mesg_strings.h"
#include "dialogs.h"
#include "mesg.h"
#include "artMode.h"
#include "compose.h"

static char *AllGroupsString = 0;
static int AllStatus = 1;	/* keeps track of which order to put the */
				/* groups in in all groups mode */

BUTTON(allQuit,quit);
BUTTON(allSub,subscribe);
BUTTON(allFirst,subscribe first);
BUTTON(allLast,subscribe last);
BUTTON(allAfter,subscribe after group);
BUTTON(allUnsub,unsubscribe);
BUTTON(allGoto,goto group);
BUTTON(allSelect,select groups);
BUTTON(allMove,move);
BUTTON(allToggle,toggle order);
BUTTON(allScroll,scroll forward);
BUTTON(allScrollBack,scroll backward);
BUTTON(allPost,post to group);

XtActionsRec AllActions[] = {
    {"allQuit",		allQuitAction},
    {"allSub",		allSubAction},
    {"allFirst",	allFirstAction},
    {"allLast",		allLastAction},
    {"allAfter",	allAfterAction},
    {"allUnsub",	allUnsubAction},
    {"allGoto",		allGotoAction},
    {"allSelect",	allSelectAction},
    {"allMove",		allMoveAction},
    {"allToggle",	allToggleAction},
    {"allScroll",	allScrollAction},
    {"allScrollBack",	allScrollBackAction},
    {"allPost",		allPostAction},
};

int AllActionsCount = XtNumber(AllActions);

ButtonList AllButtonList[] = {
    {allQuitArgs, XtNumber(allQuitArgs),
    ALLQUIT_EXSTR},
    {allSubArgs, XtNumber(allSubArgs),
    ALLSUB_EXSTR},
    {allFirstArgs, XtNumber(allFirstArgs),
    ALLFIRST_EXSTR},
    {allLastArgs, XtNumber(allLastArgs),
    ALLLAST_EXSTR},
    {allAfterArgs, XtNumber(allAfterArgs),
    ALLAFTER_EXSTR},
    {allUnsubArgs, XtNumber(allUnsubArgs),
    ALLUNSUB_EXSTR},
    {allGotoArgs, XtNumber(allGotoArgs),
    ALLGOTO_EXSTR},
    {allSelectArgs, XtNumber(allSelectArgs),
    ALLSELECT_EXSTR},
    {allMoveArgs, XtNumber(allMoveArgs),
    ALLMOVE_EXSTR},
    {allToggleArgs, XtNumber(allToggleArgs),
    ALLTOGGLE_EXSTR},
    {allScrollArgs, XtNumber(allScrollArgs),
    ALLSCROLL_EXSTR},
    {allScrollBackArgs, XtNumber(allScrollBackArgs),
    ALLSCROLLBACK_EXSTR},
    {allPostArgs, XtNumber(allPostArgs),
    ALLPOST_EXSTR},
};

int AllButtonListCount = XtNumber(AllButtonList);

/*
 * Redraw the all groups window, assuming it has changed
 */
void redrawAllWidget()
{
    String new;
    long top;

    if (CurrentMode != ALL_MODE) {
	return;
    }

    new = getStatusString(AllStatus);

    if (!AllGroupsString || strcmp(AllGroupsString, new)) {
	top = TextGetTopPosition(ArticleText);
	TextDisableRedisplay(ArticleText);
	TextSetString(ArticleText, new);
	TextSetTopPosition(ArticleText, top);
	TextEnableRedisplay(ArticleText);
	FREE(AllGroupsString);
	AllGroupsString = new;
    }
    else {
	FREE(new);
    }
}


void updateAllWidget(string, left, right)
    String string;
    long left, right;
{
    long current, top = TextGetTopPosition(ArticleText);

    TextDisableRedisplay(ArticleText);

    TextInvalidate(ArticleText, string, left - 1, right + 1);
    current = right+1;
    (void) setCursorCurrent(string, &current);
    TextSetInsertionPoint(ArticleText, current);
    TextSetTopPosition(ArticleText, top);

    TextEnableRedisplay(ArticleText);

    return;
}

void switchToAllMode()
{
    FREE(AllGroupsString);

    PreviousMode = CurrentMode;
    CurrentMode = ALL_MODE;
    /* switch buttons */
    swapMode();
    
    setBottomInfoLine(VIEW_ALLNG_SUB_MSG);
    /* create the screen */
    AllGroupsString = getStatusString(AllStatus);

    TextSetString(ArticleText, AllGroupsString);

    return;
}

/*
 * Quit all groups mode.
 */
/*ARGSUSED*/
void allQuitFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ALL_MODE) {
	return;
    }
    resetSelection();

    FREE(AllGroupsString);
    switchToNewsgroupMode(False);
}

static void doAll(status, first, last, group)
    int status;
    Boolean first, last;
    String group;
{
    String newGroup;
    char oldGroup[GROUP_NAME_SIZE];
    long first_left, left;
    Boolean in_place = (AllStatus || (! (first || last || group)));

    if (CurrentMode != ALL_MODE) {
	return;
    }
    resetSelection();

    if (newsgroupIterator(AllGroupsString, True, False, &first_left)) {
	while ((newGroup = newsgroupIterator(AllGroupsString, False, False,
					     &left))) {
	    if (first || last || group) {
		if (*oldGroup)
		    (void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
		else if (first)
		    (void) addToNewsrcBeginning(newGroup, status);
		else if (last)
		    (void) addToNewsrcEnd(newGroup, status);
		else
		    (void) addToNewsrcAfterGroup(newGroup, group, status);
		(void) strcpy(oldGroup, newGroup);
	    }
	    else if (status == SUBSCRIBE) {
		subscribe();
	    }
	    else {
		unsubscribe();
	    }
	    if (in_place)
		markAllString(AllGroupsString, first_left, left,
			      (status == SUBSCRIBE) ? SUBED_MSG : UNSUBED_MSG);
	}
	if (in_place) {
	    updateAllWidget(AllGroupsString, first_left, left);
	}
	else
	    redrawAllWidget();
    }
}

    
/*
 * Make the selected group(s) subscribed to, and leave them in
 * their current position in the newsrc file.
 */
/*ARGSUSED*/
void allSubFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    doAll(SUBSCRIBE, False, False, 0);
}

/*
 * Mark the selected group(s) as subscribed to, and move them to the
 * beginning of the newsrc file.
 */
/*ARGSUSED*/
void allFirstFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    doAll(SUBSCRIBE, True, False, 0);
}


/*
 * Mark the selected group(s) as subscribed to, and move them
 * to the end of the newsrc file.
 */
/*ARGSUSED*/
void allLastFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    doAll(SUBSCRIBE, False, True, 0);
}

static Widget AllBox = (Widget) 0;

/*
 * Mark the selected group(s) as subscribed to, and place them
 * after the group name (entered in the dialog box) in the newsrc file.
 */
static void allHandler _ARGUMENTS((Widget, XtPointer, XtPointer));

/*ARGSUSED*/
static void allHandler(widget, client_data, call_data)
    Widget widget;
    XtPointer client_data;
    XtPointer call_data;
{
    if (inCommand) {
	return;
    }
    inCommand = 1;
    busyCursor();

    if ((int) client_data != XRN_CB_ABORT)
	doAll(SUBSCRIBE, False, False, GetDialogValue(AllBox));

    PopDownDialog(AllBox);
    AllBox = 0;
    unbusyCursor();
    inCommand = 0;
    return;
}

/*
 * Put up a dialog box for the user to enter a group name after which
 * the selected articles should be placed.
 */
/*ARGSUSED*/
void allAfterFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
      {ABORT_STRING,     allHandler, (XtPointer) XRN_CB_ABORT},
      {SUB_STRING, allHandler, (XtPointer) XRN_CB_CONTINUE},
    };

    if (CurrentMode != ALL_MODE) {
	return;
    }
    resetSelection();
    
    if (AllBox == (Widget) 0) {
      AllBox = CreateDialog(TopLevel, BEHIND_WHAT_GROUP_MSG ,
				  DIALOG_TEXT, args, XtNumber(args));
    }
    PopUpDialog(AllBox);
    return;
}

/*
 * Mark the selected group(s) as unsubscribed, leaving their position
 * in the newsrc file unchanged.
 */
/*ARGSUSED*/
void allUnsubFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    doAll(UNSUBSCRIBE, False, False, 0);
}

/*
 * called when the user wants to scroll the all groups window
 */
/*ARGSUSED*/
void allScrollFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ALL_MODE) {
	return;
    }
    TextScrollPage(ArticleText, FORWARD);
    return;
}

/*
 * called when the user wants to scroll the all groups window
 */
/*ARGSUSED*/
void allScrollBackFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ALL_MODE) {
	return;
    }
    TextScrollPage(ArticleText, BACK);
    return;
}

/*
 * Go to the current newsgroup.  The current
 * group is either the first group of a selection,
 * or, if there is no selection, the group the cursor
 * is currently on (if any).
 */
/*ARGSUSED*/
void allGotoFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    int ret;
    String newGroup;

    if (CurrentMode != ALL_MODE) {
	return;
    }
    resetSelection();
    
    /* get the current group name */

    if (! (newsgroupIterator(AllGroupsString, True, False, 0) &&
	   (newGroup = newsgroupIterator(AllGroupsString, False, False, 0))))
	goto done;

    /* jump to the newsgroup */

    ret = enterNewsgroup(newGroup, ENTER_SETUP | ENTER_UNSUBBED);
    if (ret == XRN_NOUNREAD) {
	mesgPane(XRN_INFO, 0, DISPLAYING_LAST_UNREAD_MSG, newGroup);
	ret = GOOD_GROUP;
    }

    if (ret == GOOD_GROUP) {
	(void) strcpy(LastGroup, newGroup);

	FREE(AllGroupsString);
	switchToArticleMode();
    }
    else if (ret == BAD_GROUP) {
	mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, newGroup);
    }
    else if (ret == XRN_NOMORE) {
	mesgPane(XRN_SERIOUS, 0, NO_ARTICLES_MSG, newGroup);
    }
    else {
	mesgPane(XRN_SERIOUS, 0, UNKNOWN_ENTER_NG_RESPONSE_MSG, ret,
		 "allGotoFunction");
    }

  done:
    return;
}


/*
 * Post to the current newsgroup.  The current
 * group is either the first group of a selection,
 * or, if there is no selection, the group the cursor
 * is currently on (if any).
 */
/*ARGSUSED*/
void allPostFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    String newGroup;
    int ret;

    if (CurrentMode != ALL_MODE) {
	goto done;
    }
    resetSelection();
    
    /* get the current group name */

    if (! (newsgroupIterator(AllGroupsString, True, False, 0) &&
	   (newGroup = newsgroupIterator(AllGroupsString, False, False, 0))))
	goto done;

    if ((ret = enterNewsgroup(newGroup, ENTER_UNSUBBED)) == GOOD_GROUP) {
	post(1);
    }
    else if (ret == BAD_GROUP) {
	mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, newGroup);
    }
    else {
	mesgPane(XRN_SERIOUS, 0, UNKNOWN_ENTER_NG_RESPONSE_MSG, ret,
		 "allPostFunction");
    }

  done:
    return;
}


/*
 * Make note of the groups that were selected
 * to be moved.
 */
/*ARGSUSED*/
void allSelectFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ALL_MODE) {
	return;
    }
    resetSelection();
    
    if (TextGetSelectedOrCurrentLines(ArticleText, &First, &Last))
	TextUnsetSelection(ArticleText);

    return;
}

/*
 * Move the groups in the last selection to
 * the current cursor position (before the
 * current selection).
 */
/*ARGSUSED*/
void allMoveFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char newGroup[GROUP_NAME_SIZE];
    char oldGroup[GROUP_NAME_SIZE];
    int status = SUBSCRIBE;
    int dummy;
    char *newString;
    long left, right;
    long stringPoint;
    long cursorSpot;
    long ngGroupPosition;

    if (CurrentMode != ALL_MODE) {
	return;
    }
    if (First == Last) {
	mesgPane(XRN_INFO, 0, NO_GROUPS_SELECTED_MSG);
	return;
    }

    buildString(&newString, First, Last, AllGroupsString);
    stringPoint = 0;
    if (TextGetSelectedOrCurrentLines(ArticleText, &left, &right))
	TextUnsetSelection(ArticleText);
    if ((left >= First) && (left <= Last+1)) {
	mesgPane(XRN_SERIOUS, 0, NG_NOT_MOVED_MSG);
	resetSelection();
	return;
    }
    ngGroupPosition = cursorSpot = left;
    currentMode(newString, newGroup, &status, stringPoint);
    if (!moveCursor(BACK, AllGroupsString, &left)) {
	(void) addToNewsrcBeginning(newGroup, status);
	(void) strcpy(oldGroup, newGroup);
	(void) moveCursor(FORWARD, newString, &stringPoint);
    } else {
	currentMode(AllGroupsString, oldGroup, &dummy, left);
	(void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
	(void) strcpy(oldGroup, newGroup);
	(void) moveCursor(FORWARD, newString, &stringPoint);
    }
    while (newString[stringPoint] != '\0') {
	currentMode(newString, newGroup, &status, stringPoint);
	(void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
	(void) strcpy(oldGroup, newGroup);
	if (!moveCursor(FORWARD, newString, &stringPoint)) {
	    break;
	}
    }
    redrawAllWidget();
    resetSelection();
    return;
}

/* 
 * Change the order the groups appear on the screen.
 */
/*ARGSUSED*/
void allToggleFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ALL_MODE) {
	return;
    }
    resetSelection();

    AllStatus = (AllStatus == 0) ? 1 : 0;

    redrawAllWidget();

    return;
}

