#include <X11/Intrinsic.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Paned.h>

#include "buttons.h"
#include "butdefs.h"
#include "butexpl.h"
#include "allMode.h"
#include "ngMode.h"
#include "modes.h"
#include "Text.h"
#include "InfoLine.h"
#include "ButtonBox.h"
#include "news.h"
#include "internals.h"
#include "cursor.h"
#include "mesg_strings.h"
#include "dialogs.h"
#include "mesg.h"
#include "artMode.h"
#include "compose.h"
#include "resources.h"

static char *AllGroupsString = 0;
static int AllStatus = 1;	/* keeps track of which order to put the */
				/* groups in in all groups mode */
static Widget AllFrame, AllText, AllInfoLine, AllButtonBox;
static long First, Last;

BUTTON(allQuit,quit);
BUTTON(allNext,next);
BUTTON(allPrev,prev);
BUTTON(allScroll,scroll forward);
BUTTON(allScrollBack,scroll backward);
BUTTON(allSub,subscribe);
BUTTON(allFirst,subscribe first);
BUTTON(allLast,subscribe last);
BUTTON(allAfter,subscribe after group);
BUTTON(allUnsub,unsubscribe);
BUTTON(allIgnore,ignore);
BUTTON(allGoto,goto group);
BUTTON(allSelect,select groups);
BUTTON(allMove,move);
BUTTON(allToggle,toggle order);
BUTTON(allPost,post to group);
BUTTON(allPostAndMail,post to group and mail);
BUTTON(allMail,mail);

XtActionsRec AllActions[] = {
    {"allQuit",		allQuitAction},
    {"allNext",		allNextAction},
    {"allPrev",		allPrevAction},
    {"allScroll",	allScrollAction},
    {"allScrollBack",	allScrollBackAction},
    {"allSub",		allSubAction},
    {"allFirst",	allFirstAction},
    {"allLast",		allLastAction},
    {"allAfter",	allAfterAction},
    {"allUnsub",	allUnsubAction},
    {"allIgnore",	allIgnoreAction},
    {"allGoto",		allGotoAction},
    {"allSelect",	allSelectAction},
    {"allMove",		allMoveAction},
    {"allToggle",	allToggleAction},
    {"allPost",		allPostAction},
    {"allPostAndMail",	allPostAndMailAction},
    {"allMail",		allMailAction},
};

int AllActionsCount = XtNumber(AllActions);

static ButtonList AllButtonList[] = {
    {"allQuit",		allQuitCallbacks,		ALLQUIT_EXSTR},
    {"allNext",		allNextCallbacks,		NGNEXT_EXSTR},
    {"allPrev",		allPrevCallbacks,		NGPREV_EXSTR},
    {"allScroll",	allScrollCallbacks,		ALLSCROLL_EXSTR},
    {"allScrollBack",	allScrollBackCallbacks,		ALLSCROLLBACK_EXSTR},
    {"allSub",		allSubCallbacks,		ALLSUB_EXSTR},
    {"allFirst",	allFirstCallbacks,		ALLFIRST_EXSTR},
    {"allLast",		allLastCallbacks,		ALLLAST_EXSTR},
    {"allAfter",	allAfterCallbacks,		ALLAFTER_EXSTR},
    {"allUnsub",	allUnsubCallbacks,		ALLUNSUB_EXSTR},
    {"allIgnore",	allIgnoreCallbacks,		ALLIGNORE_EXSTR},
    {"allGoto",		allGotoCallbacks,		ALLGOTO_EXSTR},
    {"allSelect",	allSelectCallbacks,		ALLSELECT_EXSTR},
    {"allMove",		allMoveCallbacks,		ALLMOVE_EXSTR},
    {"allToggle",	allToggleCallbacks,		ALLTOGGLE_EXSTR},
    {"allPost",		allPostCallbacks,		ALLPOST_EXSTR},
    {"allPostAndMail",	allPostAndMailCallbacks,	ALLPOST_AND_MAIL_EXSTR},
    {"allMail",		allMailCallbacks,		MAIL_EXSTR},
};

int AllButtonListCount = XtNumber(AllButtonList);

static void allResetSelection()
{
    setButtonSensitive(AllButtonBox, "allMove", False);
    First = Last = 0;
}

/*
 * Redraw the all groups window, assuming it has changed
 */
void redrawAllWidget()
{
    String new;

    if (CurrentMode != ALL_MODE) {
	return;
    }

    new = getStatusString(TextGetColumns(AllText), AllStatus);

    if (!AllGroupsString || strcmp(AllGroupsString, new)) {
	allResetSelection();
	TextSetString(AllText, new);
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
    long current, top = TextGetTopPosition(AllText);

    TextDisableRedisplay(AllText);

    TextInvalidate(AllText, string, left, right);
    current = right;
    (void) setCursorCurrent(string, &current);
    TextSetInsertionPoint(AllText, current);
    TextSetTopPosition(AllText, top);

    TextEnableRedisplay(AllText);

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
    AllGroupsString = getStatusString(TextGetColumns(AllText), AllStatus);

    TextSetString(AllText, AllGroupsString);

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

    FREE(AllGroupsString);
    switchToNewsgroupMode(False);
}

static String allNewsgroupIterator(start, out_left)
    Boolean start;
    long *out_left;
{
    return anyIterator(AllText, AllGroupsString, True, start, False,
		       out_left);
}

static void doAll(status, first, last, group)
    int status;
    Boolean first, last;
    String group;
{
    String newGroup;
    char *oldGroup = 0;
    int oldGroupSize = 0;
    long first_left, left;
    Boolean in_place = (AllStatus || (! (first || last || group)));
    int ret;
    int len;
    /* Args for the call to currentMode */
    int old_status;
    char *current_group = 0;

    if (CurrentMode != ALL_MODE) {
	return;
    }

    if (allNewsgroupIterator(True, &first_left)) {
	while ((newGroup = allNewsgroupIterator(False, &left))) {
	  if (in_place) {
	    currentMode(AllGroupsString, &current_group, &old_status, left);
	    if ((old_status != status) &&
		((old_status == IGNORE) || (status == IGNORE)) &&
		(! AllStatus))
	      in_place = False;
	  }
	    ret = GOOD_GROUP;
	    if (status == IGNORE)
		ret = ignoreGroup(newGroup);
	    else if (first || last || group) {
		if (oldGroup)
		    (void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
		else if (first)
		    (void) addToNewsrcBeginning(newGroup, status);
		else if (last)
		    (void) addToNewsrcEnd(newGroup, status);
		else
		    (void) addToNewsrcAfterGroup(newGroup, group, status);
		if (oldGroupSize < (len = (strlen(newGroup) + 1))) {
		    oldGroupSize = len;
		    oldGroup = XtRealloc(oldGroup, oldGroupSize);
		}
		(void) strcpy(oldGroup, newGroup);
	    }
	    else {
		if ((ret = enterNewsgroup(newGroup, ENTER_UNSUBBED))
		    == BAD_GROUP) {
		    mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, newGroup);
		}
		else if (ret != GOOD_GROUP) {
		    mesgPane(XRN_SERIOUS, 0, UNKNOWN_FUNC_RESPONSE_MSG,
			     ret, "enterNewsgroup", "doAll");
		}
		else if (status == SUBSCRIBE) {
		    subscribe();
		}
		else {
		    unsubscribe();
		}
	    }
	    if ((ret == GOOD_GROUP) && in_place)
		markAllString(AllGroupsString, left,
			      (status == IGNORE) ? IGNORED_MSG :
			      ((status == SUBSCRIBE) ? SUBED_MSG : UNSUBED_MSG));
	}
	if (in_place) {
	    updateAllWidget(AllGroupsString, first_left, left);
	}
	else
	    redrawAllWidget();
    }

    XtFree(oldGroup);
    XtFree(current_group);
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
    xrnBusyCursor();

    if ((int) client_data != XRN_CB_ABORT)
	doAll(SUBSCRIBE, False, False, GetDialogValue(AllBox));

    PopDownDialog(AllBox);
    AllBox = 0;
    xrnUnbusyCursor();
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
 * Mark the selected group(s) as ignored, removing them from the
 * newsrc file.
 */
/*ARGSUSED*/
void allIgnoreFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (! app_resources.fullNewsrc)
	doAll(IGNORE, False, False, 0);
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
    TextScrollPage(AllText, FORWARD);
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
    TextScrollPage(AllText, BACK);
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
    
    /* get the current group name */

    if (! (allNewsgroupIterator(True, 0) &&
	   (newGroup = allNewsgroupIterator(False, 0))))
	goto done;

    /* jump to the newsgroup */

    ret = enterNewsgroup(newGroup, ENTER_SETUP | ENTER_UNSUBBED);
    if (ret == XRN_NOUNREAD) {
	mesgPane(XRN_INFO, 0, DISPLAYING_LAST_UNREAD_MSG, newGroup);
	ret = GOOD_GROUP;
    }

    if (ret == GOOD_GROUP) {
	LastGroup = XtRealloc(LastGroup, strlen(newGroup) + 1);
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
	mesgPane(XRN_SERIOUS, 0, UNKNOWN_FUNC_RESPONSE_MSG, ret,
		 "enterNewsgroup", "allGotoFunction");
    }

  done:
    return;
}


/*
 * Post to the current newsgroup, possibly with mailing as well.  The
 * current group is either the first group of a selection, or, if
 * there is no selection, the group the cursor is currently on (if
 * any).
 */
/*ARGSUSED*/
static void my_post_function(mail_too)
    Boolean mail_too;
{
    String newGroup;
    int ret;

    if (CurrentMode != ALL_MODE) {
	goto done;
    }
    
    /* get the current group name */

    if (! (allNewsgroupIterator(True, 0) &&
	   (newGroup = allNewsgroupIterator(False, 0))))
	goto done;

    if ((ret = enterNewsgroup(newGroup, ENTER_UNSUBBED)) == GOOD_GROUP) {
	if (mail_too)
	    post_and_mail(True);
	else
	    post(True);
    }
    else if (ret == BAD_GROUP) {
	mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, newGroup);
    }
    else {
	mesgPane(XRN_SERIOUS, 0, UNKNOWN_FUNC_RESPONSE_MSG, ret,
		 "enterNewsgroup", "my_post_function");
    }

  done:
    return;
}


void allPostFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ALL_MODE)
	return;

    my_post_function(False);
}

void allPostAndMailFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ALL_MODE)
	return;
    my_post_function(True);
}


/*
  Send mail.
  */
void allMailFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ALL_MODE)
	return;

    mail();
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
    
    if (TextGetSelectedOrCurrentLines(AllText, &First, &Last))
	TextUnsetSelection(AllText);

    setButtonSensitive(AllButtonBox, "allMove", First != Last);

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
    char *newGroup = 0;
    char *oldGroup = 0;
    int oldGroupSize = 0;
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
    if (TextGetSelectedOrCurrentLines(AllText, &left, &right))
	TextUnsetSelection(AllText);
    if ((left >= First) && (left <= Last+1)) {
	mesgPane(XRN_SERIOUS, 0, NG_NOT_MOVED_MSG);
	return;
    }
    ngGroupPosition = cursorSpot = left;
    do {
	currentMode(newString, &newGroup, &status, stringPoint);
    } while ((status == IGNORE) &&
	     moveCursor(FORWARD, newString, &stringPoint));
    if (status != IGNORE) {
	if (!moveCursor(BACK, AllGroupsString, &left)) {
	    (void) addToNewsrcBeginning(newGroup, status);
	    if (oldGroupSize < (dummy = (strlen(newGroup) + 1))) {
		oldGroupSize = dummy;
		oldGroup = XtRealloc(oldGroup, oldGroupSize);
	    }
	    (void) strcpy(oldGroup, newGroup);
	    (void) moveCursor(FORWARD, newString, &stringPoint);
	} else {
	    currentMode(AllGroupsString, &oldGroup, &dummy, left);
	    oldGroupSize = strlen(oldGroup) + 1;
	    (void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
	    if (oldGroupSize < (dummy = (strlen(newGroup) + 1))) {
		oldGroupSize = dummy;
		oldGroup = XtRealloc(oldGroup, oldGroupSize);
	    }
	    (void) strcpy(oldGroup, newGroup);
	    (void) moveCursor(FORWARD, newString, &stringPoint);
	}
	while (newString[stringPoint] != '\0') {
	    currentMode(newString, &newGroup, &status, stringPoint);
	    if (status != IGNORE) {
		(void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
		if (oldGroupSize < (dummy = (strlen(newGroup) + 1))) {
		    oldGroupSize = dummy;
		    oldGroup = XtRealloc(oldGroup, oldGroupSize);
		}
		(void) strcpy(oldGroup, newGroup);
	    }
	    if (!moveCursor(FORWARD, newString, &stringPoint)) {
		break;
	    }
	}
    }
    redrawAllWidget();
    XtFree(oldGroup);
    XtFree(newGroup);
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

    AllStatus = (AllStatus == 0) ? 1 : 0;

    redrawAllWidget();

    return;
}

void displayAllWidgets()
{
    if (! AllFrame) {
	AllFrame = XtCreateManagedWidget("allFrame", panedWidgetClass,
					 TopLevel, 0, 0);

	XawPanedSetRefigureMode(AllFrame, False);

	AllText = TextCreate("list", True, AllFrame);
	TextSetLineSelections(AllText);
	TextDisableWordWrap(AllText);

	AllInfoLine = InfoLineCreate("info", 0, AllFrame);
	TopInfoLine = AllInfoLine;

	AllButtonBox = ButtonBoxCreate("buttons", AllFrame);
	doButtons(app_resources.allButtonList, AllButtonBox,
		  AllButtonList, &AllButtonListCount, TOP);
	XtManageChild(AllButtonBox);

	setButtonSensitive(AllButtonBox, "allPost", PostingAllowed);
	setButtonSensitive(AllButtonBox, "allPostAndMail", PostingAllowed);
	if (app_resources.fullNewsrc)
	    setButtonSensitive(AllButtonBox, "allIgnore", False);

	XawPanedSetRefigureMode(AllFrame, True);

	XtSetKeyboardFocus(AllFrame, AllText);
    }
    else {
	TopInfoLine = AllInfoLine;
	XtManageChild(AllFrame);
    }
}

void hideAllWidgets()
{
    XtUnmanageChild(AllFrame);
}

void allDoTheRightThing(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    allScrollFunction(widget, event, string, count);
}

void allNextFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ALL_MODE)
	return;

    TextMoveLine(AllText, FORWARD);
}

void allPrevFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ALL_MODE)
	return;

    TextMoveLine(AllText, BACK);
}
