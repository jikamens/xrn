#include <X11/Intrinsic.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Box.h>

#include "buttons.h"
#include "butdefs.h"
#include "addMode.h"
#include "ngMode.h"
#include "butexpl.h"
#include "news.h"
#include "Text.h"
#include "InfoLine.h"
#include "ButtonBox.h"
#include "cursor.h"
#include "internals.h"
#include "modes.h"
#include "dialogs.h"
#include "mesg_strings.h"
#include "resources.h"

static String AddString;
static Widget AddFrame;
static Widget AddText, AddInfoLine, AddButtonBox;

BUTTON(addQuit,quit);
BUTTON(addIgnoreRest,ignore rest);
BUTTON(addFirst,add first);
BUTTON(addLast,add last);
BUTTON(addAfter,add after group);
BUTTON(addUnsub,add unsubscribed);
BUTTON(addIgnore,ignore);

XtActionsRec AddActions[] = {
    {"addQuit",		addQuitAction},
    {"addIgnoreRest",	addIgnoreRestAction},
    {"addFirst",	addFirstAction},
    {"addLast",		addLastAction},
    {"addAfter",	addAfterAction},
    {"addUnsub",	addUnsubAction},
    {"addIgnore",	addIgnoreAction},
};

int AddActionsCount = XtNumber(AddActions);

ButtonList AddButtonList[] = {
    {"addQuit",		addQuitCallbacks,	ADDQUIT_EXSTR},
    {"addIgnoreRest",	addIgnoreRestCallbacks,	ADDIGNORE_REST_EXSTR},
    {"addFirst",	addFirstCallbacks,	ADDFIRST_EXSTR},
    {"addLast",		addLastCallbacks,	ADDLAST_EXSTR},
    {"addAfter",	addAfterCallbacks,	ADDAFTER_EXSTR},
    {"addUnsub",	addUnsubCallbacks,	ADDUNSUB_EXSTR},
    {"addIgnore",	addIgnoreCallbacks,	ADDIGNORE_EXSTR},
};

int AddButtonListCount = XtNumber(AddButtonList);

/*
 * release storage associated with add mode and go to newsgroup mode
 */
static void exitAddMode()
{
    FREE(AddString);
    switchToNewsgroupMode(False);
}

void redrawAddTextWidget(string, insertPoint)
    String string;
    long insertPoint;
{
    int unread = 0;
    int left, right, nbytes;
    char *newGroup = 0;
    int newGroupSize = 0;

    if (CurrentMode != ADD_MODE)
	return;

    TextDisableRedisplay(AddText);

    if (!AddString || strcmp(AddString, string)) {
	FREE(AddString);
	AddString = XtNewString(string);
	TextSetString(AddText, AddString);
    }

    (void) setCursorCurrent(string, &insertPoint);

    TextSetInsertionPoint(AddText, insertPoint);

    TextEnableRedisplay(AddText);

    for (left = 0; string[left]; left = right + 1) {
	for (right = left; string[right] != '\n'; right++) /* empty */;
	nbytes = right - left;
	if (newGroupSize < nbytes + 1) {
	    newGroupSize = nbytes + 1;
	    newGroup = XtRealloc(newGroup, newGroupSize);
	}
	(void) strncpy(newGroup, &string[left], nbytes);
	newGroup[nbytes] = '\0';
	if (watchingGroup(newGroup)) {
	    unread++;
	    break;
	}
    }

    if (! unread)
	unread = unreadNews();
    if (unread)
	xmSetIconAndName(UnreadIcon);
    else
	xmSetIconAndName(ReadIcon);

    XtFree(newGroup);
    return;
}


static void addFunction(first, newsgroup, status)
    Boolean first;
    String newsgroup;
    int status;
{
    String oldGroup = 0, newGroup = 0;
    int oldGroupSize = 0;
    long left, right;

    if (CurrentMode != ADD_MODE) {
	return;
    }

    if (! TextGetSelectedOrCurrentLines(AddText, &left, &right))
	return;

    TextDisableRedisplay(AddText);

    while (left < right) {
	int add_ret, len;

	currentGroup(CurrentMode, AddString, &newGroup, left);
	if (! *newGroup)
	    break;

	clearNew(newGroup);
	if (status == IGNORE)
	    add_ret = ignoreGroup(newGroup);
	else if (oldGroup)
	    add_ret = addToNewsrcAfterGroup(newGroup, oldGroup, status);
	else if (newsgroup)
	    add_ret = addToNewsrcAfterGroup(newGroup, newsgroup, status);
	else if (first)
	    add_ret = addToNewsrcBeginning(newGroup, status);
	else
	    add_ret = addToNewsrcEnd(newGroup, status);

	if (add_ret == GOOD_GROUP) {
	    long new_position = left;
	    moveCursor(FORWARD, AddString, &new_position);
	    right -= (new_position - left);
	    removeLine(AddString, &left);
	    TextRemoveLine(AddText, left);

	    if (oldGroupSize < (len = (strlen(newGroup) + 1))) {
		oldGroupSize = len;
		oldGroup = XtRealloc(oldGroup, oldGroupSize);
	    }
	    (void) strcpy(oldGroup, newGroup);
	}
	else
	    break;
    }

    if (setCursorCurrent(AddString, &left)) {
	TextSetInsertionPoint(AddText, left);
    }
    else {
	exitAddMode();
    }

    XtFree(oldGroup);
    XtFree(newGroup);

    TextEnableRedisplay(AddText);
}


/*
 * unsubscribe to the remaining groups and exit add mode
 */
/*ARGSUSED*/
void addQuitFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ADD_MODE) {
	return;
    }

    TextDisableRedisplay(AddText);

    TextSelectAll(AddText);
    addFunction(False, 0, UNSUBSCRIBE);

    TextEnableRedisplay(AddText);
}

/*
 * ignore the remaining groups and exit add mode
 */
/*ARGSUSED*/
void addIgnoreRestFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ADD_MODE) {
	return;
    }

    if (app_resources.fullNewsrc) {
	addQuitFunction(widget, event, string, count);
	return;
    }

    TextDisableRedisplay(AddText);

    TextSelectAll(AddText);
    addFunction(False, 0, IGNORE);

    TextEnableRedisplay(AddText);
}

/*
 * Find selected group(s) and add them to the .newsrc in the first position.
 * Move the cursor to the next group.
 * Update the AddGroupsString, going into newsgroup mode if it
 * is NULL.  Update the text window, update the insertion point.
 *
 */
/*ARGSUSED*/
void addFirstFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    addFunction(True, 0, SUBSCRIBE);
}

/*
 * add the currently selected group(s) to the end of the .newsrc file
 * and subscribe to them.
 */
/*ARGSUSED*/
void addLastFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    addFunction(False, 0, SUBSCRIBE);
}

/* entering the name of a newsgroup to add after */

static Widget AddBox = (Widget) 0;

/*
 * get the newsgroup to add a new newsgroup after in the .newsrc file
 */
static void addHandler _ARGUMENTS((Widget, XtPointer, XtPointer));

/*ARGSUSED*/
static void addHandler(widget, client_data, call_data)
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
	addFunction(False, GetDialogValue(AddBox), SUBSCRIBE);

    PopDownDialog(AddBox);
    AddBox = 0;
    xrnUnbusyCursor();
    inCommand = 0;
    return;
}

/*
 * subscribe to a new newsgroup, adding after a particular group in the
 * .newsrc file
 */
/*ARGSUSED*/
void addAfterFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
      {ABORT_STRING, addHandler, (XtPointer) XRN_CB_ABORT},
      {ADD_STRING,   addHandler, (XtPointer) XRN_CB_CONTINUE},
    };

    if (CurrentMode != ADD_MODE) {
	return;
    }
    if (AddBox == (Widget) 0) {
      AddBox = CreateDialog(TopLevel, BEHIND_WHAT_GROUP_MSG,
				  DIALOG_TEXT, args, XtNumber(args));
    }
    PopUpDialog(AddBox);
    return;
}

/*
 * add a group to the end of the .newsrc file as unsubscribed
 */
/*ARGSUSED*/
void addUnsubFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    addFunction(False, 0, UNSUBSCRIBE);
}

/*
 * ignore group(s)
 */
/*ARGSUSED*/
void addIgnoreFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (app_resources.fullNewsrc) {
	addUnsubFunction(widget, event, string, count);
	return;
    }

    addFunction(False, 0, IGNORE);
}

void switchToAddMode(groups)
    String groups;
{
    PreviousMode = CurrentMode;
    CurrentMode = ADD_MODE;

    swapMode();

    setTopInfoLine(SEL_GROUPS_ADDSUB_MSG);
    redrawAddTextWidget(groups, 0);

    FREE(AddString);
    AddString = XtNewString(groups);
}

void displayAddWidgets()
{
    if (! AddFrame) {
	AddFrame = XtCreateManagedWidget("addFrame", panedWidgetClass,
					 TopLevel, 0, 0);

	XawPanedSetRefigureMode(AddFrame, False);

	AddText = TextCreate("list", True, AddFrame);
	TextSetLineSelections(AddText);
	TextDisableWordWrap(AddText);

	AddInfoLine = InfoLineCreate("info", 0, AddFrame);
	TopInfoLine = AddInfoLine;

	AddButtonBox = ButtonBoxCreate("buttons", AddFrame);
	doButtons(app_resources.addButtonList, AddButtonBox,
		  AddButtonList, &AddButtonListCount, TOP);
	XtManageChild(AddButtonBox);

	if (app_resources.fullNewsrc) {
	    setButtonSensitive(AddButtonBox, "addIgnoreRest", False);
	    setButtonSensitive(AddButtonBox, "addIgnore", False);
	}

	XawPanedSetRefigureMode(AddFrame, True);

	XtSetKeyboardFocus(AddFrame, AddText);
    }
    else {
	TopInfoLine = AddInfoLine;
	XtManageChild(AddFrame);
    }
}

void hideAddWidgets()
{
    XtUnmanageChild(AddFrame);
}
