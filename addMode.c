#include <X11/Intrinsic.h>
#include "buttons.h"
#include "butdefs.h"
#include "addMode.h"
#include "ngMode.h"
#include "butexpl.h"
#include "news.h"
#include "Text.h"
#include "cursor.h"
#include "internals.h"
#include "modes.h"
#include "dialogs.h"
#include "mesg_strings.h"

static String AddString;

BUTTON(addQuit,quit);
BUTTON(addFirst,add first);
BUTTON(addLast,add last);
BUTTON(addAfter,add after group);
BUTTON(addUnsub,add unsubscribed);

XtActionsRec AddActions[] = {
    {"addQuit",		addQuitAction},
    {"addFirst",	addFirstAction},
    {"addLast",		addLastAction},
    {"addAfter",	addAfterAction},
    {"addUnsub",	addUnsubAction},
};

int AddActionsCount = XtNumber(AddActions);

ButtonList AddButtonList[] = {
    {addQuitArgs, XtNumber(addQuitArgs),
    ADDQUIT_EXSTR},
    {addFirstArgs, XtNumber(addFirstArgs),
    ADDFIRST_EXSTR},
    {addLastArgs, XtNumber(addLastArgs),
    ADDLAST_EXSTR},
    {addAfterArgs, XtNumber(addAfterArgs),
    ADDAFTER_EXSTR},
    {addUnsubArgs, XtNumber(addUnsubArgs),
    ADDUNSUB_EXSTR},
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
    char newGroup[GROUP_NAME_SIZE];

    TextDisableRedisplay(Text);

    if (!AddString || strcmp(AddString, string)) {
	FREE(AddString);
	AddString = XtNewString(string);
	TextSetString(Text, AddString);
    }

    (void) setCursorCurrent(string, &insertPoint);

    TextSetInsertionPoint(Text, insertPoint);

    adjustMinMaxLines(AddString);

    TextEnableRedisplay(Text);

    for (left = 0; string[left]; left = right + 1) {
	for (right = left; string[right] != '\n'; right++) /* empty */;
	nbytes = right - left;
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

    return;
}


static void addFunction(first, newsgroup, status)
    Boolean first;
    String newsgroup;
    int status;
{
    String newGroup;
    char oldGroup[GROUP_NAME_SIZE] = "";
    long AddPosition;

    if (CurrentMode != ADD_MODE) {
	return;
    }

    TextDisableRedisplay(Text);

    if (newsgroupIterator(AddString, True, True, &AddPosition)) {
	while ((newGroup = newsgroupIterator(AddString, False, True,
					     &AddPosition))) {
	    if (*oldGroup)
		(void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
	    else if (newsgroup)
		(void) addToNewsrcAfterGroup(newGroup, newsgroup, status);
	    else if (first)
		(void) addToNewsrcBeginning(newGroup, status);
	    else
		(void) addToNewsrcEnd(newGroup, status);
	    (void) strcpy(oldGroup, newGroup);
	}
	if (setCursorCurrent(AddString, &AddPosition)) {
	    TextSetInsertionPoint(Text, AddPosition);
	    adjustMinMaxLines(AddString);
	}
	else {
	    exitAddMode();
	}
    }

    TextEnableRedisplay(Text);
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

    TextDisableRedisplay(Text);

    TextSelectAll(Text);
    addFunction(False, 0, UNSUBSCRIBE);

    TextEnableRedisplay(Text);
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
    busyCursor();

    if ((int) client_data != XRN_CB_ABORT)
	addFunction(False, GetDialogValue(AddBox), SUBSCRIBE);

    PopDownDialog(AddBox);
    AddBox = 0;
    unbusyCursor();
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
