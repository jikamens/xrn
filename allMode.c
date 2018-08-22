#include <X11/Intrinsic.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Paned.h>

#include "config.h"
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

static char *LimitString = NULL;

BUTTON(allQuit,quit);
BUTTON(allNext,next);
BUTTON(allPrev,prev);
BUTTON(allScroll,scroll forward);
BUTTON(allScrollBack,scroll backward);
BUTTON(allSearch,search);
BUTTON(allLimit,limit);
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
    {"allSearch",	allSearchAction},
    {"allLimit",	allLimitAction},
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
  {"allQuit",	     allQuitCallbacks,	      ALLQUIT_EXSTR,	      True},
  {"allNext",	     allNextCallbacks,	      NGNEXT_EXSTR,	      True},
  {"allPrev",	     allPrevCallbacks,	      NGPREV_EXSTR,	      True},
  {"allScroll",	     allScrollCallbacks,      ALLSCROLL_EXSTR,	      True},
  {"allScrollBack",  allScrollBackCallbacks,  ALLSCROLLBACK_EXSTR,    True},
  {"allSearch",      allSearchCallbacks,      ALLSEARCH_EXSTR,        True},
  {"allLimit",       allLimitCallbacks,       ALLLIMIT_EXSTR,         True},
  {"allSub",	     allSubCallbacks,	      ALLSUB_EXSTR,	      True},
  {"allFirst",	     allFirstCallbacks,	      ALLFIRST_EXSTR,	      True},
  {"allLast",	     allLastCallbacks,	      ALLLAST_EXSTR,	      True},
  {"allAfter",	     allAfterCallbacks,	      ALLAFTER_EXSTR,	      True},
  {"allUnsub",	     allUnsubCallbacks,	      ALLUNSUB_EXSTR,	      True},
  {"allIgnore",	     allIgnoreCallbacks,      ALLIGNORE_EXSTR,	      True},
  {"allGoto",	     allGotoCallbacks,	      ALLGOTO_EXSTR,	      True},
  {"allSelect",	     allSelectCallbacks,      ALLSELECT_EXSTR,	      True},
  {"allMove",	     allMoveCallbacks,	      ALLMOVE_EXSTR,	      True},
  {"allToggle",	     allToggleCallbacks,      ALLTOGGLE_EXSTR,	      True},
  {"allPost",	     allPostCallbacks,	      ALLPOST_EXSTR,	      True},
  {"allPostAndMail", allPostAndMailCallbacks, ALLPOST_AND_MAIL_EXSTR, True},
  {"allMail",	     allMailCallbacks,	      MAIL_EXSTR,	      True},
};

static int AllButtonListCount = XtNumber(AllButtonList);

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

    while (! (new = getStatusString(TextGetColumns(AllText),
				    AllStatus, LimitString))) {
      FREE(LimitString);
    }

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
    FREE(LimitString); /* also done in allQuitFunction, but it
			  doesn't hurt to be cautious */
    PreviousMode = CurrentMode;
    CurrentMode = ALL_MODE;
    /* switch buttons */
    swapMode();

    setBottomInfoLine(VIEW_ALLNG_SUB_MSG);
    /* create the screen */
    AllGroupsString = getStatusString(TextGetColumns(AllText), AllStatus,
				      LimitString);

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
    FREE(LimitString); /* also done in switchToAllMode, but it doesn't
			  hurt to be cautious */

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
		  if (! subscribe())
		    ret = BAD_GROUP;
		}
		else {
		    unsubscribe();
		}
		exitNewsgroup();
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

    if ((POINTER_NUM_TYPE) client_data != XRN_CB_ABORT)
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
  Search the group list, using the search functionality built into the Text
  widget.
  */
void allSearchFunction(widget, event, string, count)
     Widget widget;
     XEvent *event;
     String *string;
     Cardinal *count;
{
  if (CurrentMode != ALL_MODE)
    return;

  TextSearchInteractive(AllText,
			event ? event : XtLastEventProcessed(XtDisplay(widget)),
			-1, TextSearchRight,
			(count && *count) ? string[0] : NULL);
}

static Widget LimitBox = (Widget) 0;

static void limitHandler _ARGUMENTS((Widget, XtPointer, XtPointer));

/*ARGSUSED*/
static void limitHandler(widget, client_data, call_data)
    Widget widget;
    XtPointer client_data;
    XtPointer call_data;
{
    if (inCommand)
      return;

    inCommand = 1;
    xrnBusyCursor();

    if (strcmp(DOIT_STRING, (char *) client_data) == 0) {
      FREE(LimitString);

      if ((LimitString = GetDialogValue(LimitBox))) {
	if (! *LimitString)
	  LimitString = NULL;
	else
	  LimitString = XtNewString(LimitString);
      }

      redrawAllWidget();
    }
    
    if (LimitBox) {
      PopDownDialog(LimitBox);
      LimitBox = 0;
    }

    inCommand = 0;
    xrnUnbusyCursor();
    return;
}

/*ARGSUSED*/
void allLimitFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
      {ABORT_STRING,limitHandler, (XtPointer) ABORT_STRING},
      {DOIT_STRING, limitHandler, (XtPointer) DOIT_STRING},
    };

    if (CurrentMode != ALL_MODE) {
	return;
    }
    if (LimitBox == (Widget) 0) {
      LimitBox = CreateDialog(TopLevel, REGULAR_EXPR_MSG,
			      DIALOG_TEXT, args, XtNumber(args));
    }
    PopUpDialog(LimitBox);
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
	exitNewsgroup();
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
	exitNewsgroup();
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

static void resizeAllText _ARGUMENTS((Widget, XtPointer, XEvent *,
				      Boolean *));

static void resizeAllText(widget, client_data, event,
			  continue_to_dispatch)
     Widget widget;
     XtPointer client_data;
     XEvent *event;
     Boolean *continue_to_dispatch;
{
  if (event->type == ConfigureNotify) {
    redrawAllWidget();
  }
}

void displayAllWidgets()
{
    if (! AllFrame) {
	AllFrame = XtCreateManagedWidget("allFrame", panedWidgetClass,
					 TopLevel, 0, 0);

	XawPanedSetRefigureMode(AllFrame, False);

	setButtonActive(AllButtonList, "allPost", PostingAllowed);
	setButtonActive(AllButtonList, "allPostAndMail", PostingAllowed);
	if (app_resources.fullNewsrc)
	  setButtonActive(AllButtonList, "allIgnore", False);

#define BUTTON_BOX() {\
	  AllButtonBox = ButtonBoxCreate("buttons", AllFrame);\
	  doButtons(app_resources.allButtonList, AllButtonBox,\
		    AllButtonList, &AllButtonListCount, TOP);\
	}

#define INFO_LINE() {\
	  AllInfoLine = InfoLineCreate("info", 0, AllFrame);\
	}

	if (app_resources.buttonsOnTop) {
	  BUTTON_BOX();
	  INFO_LINE();
	}

	AllText = TextCreate("list", True, AllFrame);

	if (! app_resources.buttonsOnTop) {
	  INFO_LINE();
	  BUTTON_BOX();
	}

#undef BUTTON_BOX
#undef INFO_LINE

	TextSetLineSelections(AllText);
	TextDisableWordWrap(AllText);

	TopInfoLine = AllInfoLine;

	XawPanedSetRefigureMode(AllFrame, True);

	XtSetKeyboardFocus(AllFrame, AllText);

	XtAddEventHandler(AllText, StructureNotifyMask, FALSE,
			  resizeAllText, NULL);
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
