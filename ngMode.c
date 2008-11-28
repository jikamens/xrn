#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Paned.h>

#include "ngMode.h"
#include "butdefs.h"
#include "xrn.h"
#include "xmisc.h"
#include "butexpl.h"
#include "mesg_strings.h"
#include "buttons.h"
#include "modes.h"
#include "resources.h"
#include "error_hnds.h"
#include "news.h"
#include "internals.h"
#include "Text.h"
#include "InfoLine.h"
#include "ButtonBox.h"
#include "mesg.h"
#include "cursor.h"
#include "dialogs.h"
#include "newsrcfile.h"
#include "compose.h"
#include "artMode.h"
#include "allMode.h"

static String NewsGroupsString;
static Widget NewsgroupFrame, NewsgroupText, NewsgroupInfoLine;
static Widget NewsgroupButtonBox;

static char *GotoNewsgroupString = 0;
static long First, Last;

/*
  The group we're currently positioned on in the newsgroup index.
  */
char *CurrentIndexGroup = 0;
char *LastGroup = 0;
int NewsgroupDisplayMode = 0;	/* 0 for unread groups, 1 for all sub */

#define NG_ENTRY_JUMP 0
#define NG_ENTRY_GOTO 1

static int NewsgroupEntryMode = NG_ENTRY_GOTO;

BUTTON(ngExit,exit);
BUTTON(ngQuit,quit);
BUTTON(ngRead,read group);
BUTTON(ngNext,next);
BUTTON(ngPrev,prev);
BUTTON(ngCatchUp,catch up);
BUTTON(ngSubscribe,subscribe);
BUTTON(ngUnsub,unsubscribe);
BUTTON(ngGoto,goto newsgroup);
BUTTON(ngListOld,sub groups);
BUTTON(ngAllGroups,all groups);
BUTTON(ngRescan,rescan);
BUTTON(ngGetList,get list);
BUTTON(ngPrevGroup,prev group);
BUTTON(ngSelect,select groups);
BUTTON(ngMove,move);
BUTTON(ngCheckPoint,checkpoint);
BUTTON(ngPost,post);
BUTTON(ngPostAndMail,post and mail);
BUTTON(ngMail,mail);
BUTTON(ngGripe,gripe);
BUTTON(ngScroll,scroll forward);
BUTTON(ngScrollBack,scroll backward);

XtActionsRec NgActions[] = {
    {"ngExit",		ngExitAction},
    {"ngQuit",		ngQuitAction},
    {"ngRead",		ngReadAction},
    {"ngNext",		ngNextAction},
    {"ngPrev",		ngPrevAction},
    {"ngCatchUp",	ngCatchUpAction},
    {"ngSubscribe",	ngSubscribeAction},
    {"ngUnsub",		ngUnsubAction},
    {"ngGoto",		ngGotoAction},
    {"ngListOld",	ngListOldAction},
    {"ngAllGroups",	ngAllGroupsAction},
    {"ngRescan",	ngRescanAction},
    {"ngGetList",	ngGetListAction},
    {"ngPrevGroup",	ngPrevGroupAction},
    {"ngSelect",	ngSelectAction},
    {"ngMove",		ngMoveAction},
    {"ngCheckPoint",	ngCheckPointAction},
    {"ngPost",		ngPostAction},
    {"ngPostAndMail",	ngPostAndMailAction},
    {"ngMail",		ngMailAction},
    {"ngGripe",		ngGripeAction},
    {"ngScroll",	ngScrollAction},
    {"ngScrollBack",	ngScrollBackAction},
};    

int NgActionsCount = XtNumber(NgActions);

ButtonList NewsgroupButtonList[] = {
    {"ngQuit",		ngQuitCallbacks,	NGQUIT_EXSTR},
    {"ngRead",		ngReadCallbacks,	NGREAD_EXSTR},
    {"ngNext",		ngNextCallbacks,	NGNEXT_EXSTR},
    {"ngPrev",		ngPrevCallbacks,	NGPREV_EXSTR},
    {"ngScroll",	ngScrollCallbacks,	NGSCROLL_EXSTR},
    {"ngScrollBack",	ngScrollBackCallbacks,	NGSCROLLBACK_EXSTR},
    {"ngCatchUp",	ngCatchUpCallbacks,	NGCATCHUP_EXSTR},
    {"ngSubscribe",	ngSubscribeCallbacks,	NGSUBSCRIBE_EXSTR},
    {"ngUnsub",		ngUnsubCallbacks,	NGUNSUB_EXSTR},
    {"ngGoto",		ngGotoCallbacks,	NGGOTO_EXSTR},
    {"ngAllGroups",	ngAllGroupsCallbacks,	NGALLGROUPS_EXSTR},
    {"ngRescan",	ngRescanCallbacks,	NGRESCAN_EXSTR},
    {"ngGetList",	ngGetListCallbacks,	NGGETLIST_EXSTR},
    {"ngPrevGroup",	ngPrevGroupCallbacks,	NGPREVGROUP_EXSTR},
    {"ngListOld",	ngListOldCallbacks,	NGLISTOLD_EXSTR},
    {"ngSelect",	ngSelectCallbacks,	NGSELECT_EXSTR},
    {"ngMove",		ngMoveCallbacks,	NGMOVE_EXSTR},
    {"ngExit",		ngExitCallbacks,	NGEXIT_EXSTR},
    {"ngCheckPoint",	ngCheckPointCallbacks,	NGCHECKPOINT_EXSTR},
    {"ngGripe",		ngGripeCallbacks,	NGGRIPE_EXSTR},
    {"ngPost",		ngPostCallbacks,	NGPOST_EXSTR},
    {"ngPostAndMail",	ngPostAndMailCallbacks,	NGPOST_AND_MAIL_EXSTR},
    {"ngMail",		ngMailCallbacks,	MAIL_EXSTR},
};

int NewsgroupButtonListCount = XtNumber(NewsgroupButtonList);

/*
  Set the current insertion point in the newsgroup widget, saving the
  current display position first (since setting the insertion point
  causes the window to scroll so that the cursor is visible).
  */
static void setNewsgroupPosition(position)
    long position;
{
    TextSetInsertionPoint(NewsgroupText, position);
}


/*
 * called when the user wants to quit xrn
 *
 *  full update the newsrc file
 *  exit
 */
/*ARGSUSED*/
void ngQuitFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (pendingCompositionP()) {
	mesgPane(XRN_SERIOUS, 0, PENDING_COMPOSITION_MSG);
	XBell(XtDisplay(TopLevel), 0);
	return;
    }
    confirmBox(ARE_YOU_SURE_MSG, NEWSGROUP_MODE, NG_QUIT, ehCleanExitXRN);
}

static String newsgroupNewsgroupIterator(start, out_left)
    Boolean start;
    long *out_left;
{
    return anyIterator(NewsgroupText, NewsGroupsString, True, start, False,
		       out_left);
}

static void resetSelection()
/*
 * Reset First and Last to zero, so the user doesn't accidentally
 * move groups
 */
{
    First = 0;
    Last = 0;

    setButtonSensitive(NewsgroupButtonBox, "ngMove", False);

    return;
}


/*
 * called when the user wants to read a new newsgroup
 *
 * get the selected group, set the internal pointers, and go to article mode
 *
 */
/*ARGSUSED*/
void ngReadFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    String name;
    int status;
      
    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }

    if (newsgroupNewsgroupIterator(True, 0) &&
	(name = newsgroupNewsgroupIterator(False, 0))) {
	CurrentIndexGroup = XtRealloc(CurrentIndexGroup, strlen(name) + 1);
	(void) strcpy(CurrentIndexGroup, name);
	status = enterNewsgroup(name, ENTER_SETUP |
				(NewsgroupDisplayMode ? 0 : ENTER_UNREAD) |
				((NewsgroupEntryMode == NG_ENTRY_JUMP) ?
				 (ENTER_UNSUBBED | ENTER_SUBSCRIBE) : 0));
	NewsgroupEntryMode = NG_ENTRY_GOTO;
	if (NewsgroupDisplayMode && (status == ENTER_UNREAD)) {
	    mesgPane(XRN_INFO, 0, DISPLAYING_LAST_UNREAD_MSG, name);
	    status = GOOD_GROUP;
	}
	if (status == GOOD_GROUP) {
	    LastGroup = XtRealloc(LastGroup, strlen(name) + 1);
	    (void) strcpy(LastGroup, name);
	    switchToArticleMode();
	}
	else {
	    if (status == XRN_NOUNREAD)
		mesgPane(XRN_INFO, 0, PROBABLY_KILLED_MSG, name);
	    else if (status == BAD_GROUP)
		mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, name);
	    else if (status == XRN_NOMORE)
		mesgPane(XRN_SERIOUS, 0, NewsgroupDisplayMode ? NO_ARTICLES_MSG :
			 PROBABLY_EXPIRED_MSG, name);
	    else
		mesgPane(XRN_SERIOUS, 0, UNKNOWN_FUNC_RESPONSE_MSG, status,
			 "enterNewsgroup", "ngReadFunction");
	    updateNewsgroupMode(True, False);
	    return;
	}
    }
}


/*
 * called when the user does not want to read a newsgroup
 *
 * if selected group, set internal group
 * call updateNewsgroupMode
 */
/*ARGSUSED*/
void ngNextFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    long left;

    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }

    (void) newsgroupNewsgroupIterator(True, &left);
    (void) moveUpWrap(NewsGroupsString, &left);
    /*
      Optimization -- XawTextSetInsertionPoint always redisplays the
      whole Text widget.  However, the next-line action procedure
      doesn't.  Therefore, we use that action procedure here (and
      below in ngPrevFunction), because in most cases, it'll put the
      insertion point in the right place, and a redraw won't be
      necessary.
      */
    TextMoveLine(NewsgroupText, FORWARD);
    currentGroup(CurrentMode, NewsGroupsString, &CurrentIndexGroup, left);
    left = getNearbyNewsgroup(NewsGroupsString, &CurrentIndexGroup);
    setNewsgroupPosition(left);
}


/*
 * called when the user wants to move the cursor up in
 * the newsgroup window
 *
 * if selected group, set internal group
 * call updateNewsgroupMode
 */
/*ARGSUSED*/
void ngPrevFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    long left;

    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }

    (void) newsgroupNewsgroupIterator(True, &left);
    (void) moveCursor(BACK, NewsGroupsString, &left);
    TextMoveLine(NewsgroupText, BACK);
    currentGroup(CurrentMode, NewsGroupsString, &CurrentIndexGroup, left);
    left = getNearbyNewsgroup(NewsGroupsString, &CurrentIndexGroup);
    setNewsgroupPosition(left);
}


/*
 * used when the user has elected to catch
 * up newsgroups in newsgroup mode
 */
static void catchUpNG()
{
    String name;
    int ret;

    if (newsgroupNewsgroupIterator(True, 0)) {
	while ((name = newsgroupNewsgroupIterator(False, 0))) {
	    CurrentIndexGroup = XtRealloc(CurrentIndexGroup, strlen(name) + 1);
	    (void) strcpy(CurrentIndexGroup, name);
	    if ((ret = enterNewsgroup(name, ENTER_UNSUBBED))
		== GOOD_GROUP) {
		catchUp();
	    }
	    else if (ret == BAD_GROUP) {
		 mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, name);
	    }
	    else {
		 mesgPane(XRN_SERIOUS, 0, UNKNOWN_FUNC_RESPONSE_MSG, ret,
			  "enterNewsgroup", "catchUpNG");
	    }
	}
	updateNewsgroupMode(True, False);
    }
}


/*
 * Unsubscribe user from selected group(s)
 */
static void unsubscribeNG()
{
    String name;
    int ret;

    if (newsgroupNewsgroupIterator(True, 0)) {
	while ((name = newsgroupNewsgroupIterator(False, 0))) {
	    CurrentIndexGroup = XtRealloc(CurrentIndexGroup, strlen(name) + 1);
	    (void) strcpy(CurrentIndexGroup, name);
	    if ((ret = enterNewsgroup(name, ENTER_UNSUBBED))
		== GOOD_GROUP) {
		unsubscribe();
	    }
	    else if (ret == BAD_GROUP) {
		 mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, name);
	    }
	    else {
		mesgPane(XRN_SERIOUS, 0, UNKNOWN_FUNC_RESPONSE_MSG, ret,
			 "enterNewsgroup", "unsubscribeNG");
	    }
	}
	updateNewsgroupMode(True, False);
    }
}


/*
 * called to catch up on all unread articles in this newsgroup
 * use a confirmation box if the user has requested it
 * if selected group, set internal group
 */
/*ARGSUSED*/
void ngCatchUpFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    confirmBox(OK_CATCHUP_MSG, NEWSGROUP_MODE, NG_CATCHUP, catchUpNG);
}

/*
 * called to unsubscribe to a newsgroup
 *
 * if selected group, set internal group
 * do internals
 * call updateNewsgroupMode
 */
/*ARGSUSED*/
void ngUnsubFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    confirmBox(OK_TO_UNSUB_MSG, NEWSGROUP_MODE, NG_UNSUBSCRIBE, unsubscribeNG);
}

#define XRNsub_ABORT 0
#define XRNsub_LASTGROUP 1
#define XRNsub_FIRST 2
#define XRNsub_LAST 3
#define XRNsub_CURRENT 4

static Widget SubscribeBox = (Widget) 0;

static void subscribeHandler _ARGUMENTS((Widget, XtPointer, XtPointer));

/*ARGSUSED*/
static void subscribeHandler(widget, client_data, call_data)
    Widget widget;
    XtPointer client_data;
    XtPointer call_data;
{
    int status = SUBSCRIBE;
    char *group, *name = 0;
    long left, insertion;
    int ret;

    if (inCommand) {
	return;
    }
    inCommand = 1;
    xrnBusyCursor();

    TextDisableRedisplay(NewsgroupText);

    switch ((int) client_data) {
    case XRNsub_LASTGROUP:
	if (LastGroup && *LastGroup) {
	    if ((ret = enterNewsgroup(LastGroup, ENTER_UNSUBBED))
		== BAD_GROUP) {
		mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, LastGroup);
		goto done;
	    }
	    else if (ret == GOOD_GROUP) {
		subscribe();
		CurrentIndexGroup = XtRealloc(CurrentIndexGroup,
					      strlen(LastGroup) + 1);
		(void) strcpy(CurrentIndexGroup, LastGroup);
		updateNewsgroupMode(True, False);
	    }
	    else {
		mesgPane(XRN_SERIOUS, 0, UNKNOWN_FUNC_RESPONSE_MSG, ret,
			 "enterNewsgroup", "subscribeHandler");
	    }
	}
	break;

    case XRNsub_FIRST:
	if (addToNewsrcBeginning(group = GetDialogValue(SubscribeBox),
				 status) == GOOD_GROUP) {
	    CurrentIndexGroup = XtRealloc(CurrentIndexGroup, strlen(group) + 1);
	    (void) strcpy(CurrentIndexGroup, group);
	    updateNewsgroupMode(True, False);
	}
	break;

    case XRNsub_LAST:
	if (addToNewsrcEnd(group = GetDialogValue(SubscribeBox),
			   status) == GOOD_GROUP) {
	    CurrentIndexGroup = XtRealloc(CurrentIndexGroup, strlen(group) + 1);
	    (void) strcpy(CurrentIndexGroup, group);
	    updateNewsgroupMode(True, False);
	}
	break;

    case XRNsub_CURRENT:
	insertion = TextGetInsertionPoint(NewsgroupText);

	if (! NewsGroupsString[insertion]) {
	    if (addToNewsrcEnd(group = GetDialogValue(SubscribeBox),
			       status) == GOOD_GROUP) {
		CurrentIndexGroup = XtRealloc(CurrentIndexGroup,
					      strlen(group) + 1);
		(void) strcpy(CurrentIndexGroup, group);
		updateNewsgroupMode(True, False);
	    }
	} else {
	    /* don't need to check for the null group here, it would have */
	    /* been already handled above */
	    (void) newsgroupNewsgroupIterator(True, &left);
	    if (left == 0) {
	        if (addToNewsrcBeginning(group = GetDialogValue(SubscribeBox),
					 status) == GOOD_GROUP) {
		    CurrentIndexGroup = XtRealloc(CurrentIndexGroup,
						  strlen(group) + 1);
		    (void) strcpy(CurrentIndexGroup, group);
		}
	    } else {
		(void) moveCursor(BACK, NewsGroupsString, &left);
		currentGroup(CurrentMode, NewsGroupsString, &name, left);
	        if (addToNewsrcAfterGroup(group = GetDialogValue(SubscribeBox),
					  name, status) == GOOD_GROUP) {
		    CurrentIndexGroup = XtRealloc(CurrentIndexGroup,
						  strlen(group) + 1);
		    (void) strcpy(CurrentIndexGroup, group);
		}
	    }
	    updateNewsgroupMode(True, False);
    	}
	break;
    }

  done:
    TextEnableRedisplay(NewsgroupText);
    PopDownDialog(SubscribeBox);
    SubscribeBox = 0;
    xrnUnbusyCursor();
    inCommand = 0;
    XtFree(name);
    return;
}


/*
 * Subscribe to a group currently unsubscribed to
 */
/*ARGSUSED*/
void ngSubscribeFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
      {ABORT_STRING,            subscribeHandler, (XtPointer) XRNsub_ABORT},
      {LAST_GROUP_STRING,       subscribeHandler, (XtPointer) XRNsub_LASTGROUP},
      {FIRST_STRING,            subscribeHandler, (XtPointer) XRNsub_FIRST},
      {LAST_STRING,             subscribeHandler, (XtPointer) XRNsub_LAST},
      {CURSOR_POS_STRING,  subscribeHandler, (XtPointer) XRNsub_CURRENT},
    };

    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }
    
    if (SubscribeBox == (Widget) 0) {
      SubscribeBox = CreateDialog(TopLevel, GROUP_SUB_TO_MSG,
				    DIALOG_TEXT, args, XtNumber(args));
    }
    PopUpDialog(SubscribeBox);

    return;
}

#define XRNgoto_ABORT 0
#define XRNgoto_GOTO 1

static Widget GotoNewsgroupBox = (Widget) 0;

static void gotoHandler _ARGUMENTS((Widget, XtPointer, XtPointer));

/*ARGSUSED*/
static void gotoHandler(widget, client_data, call_data)
    Widget widget;
    XtPointer client_data;
    XtPointer call_data;
{
    char *name;
    int ret;

    if (inCommand) {
	return;
    }
    inCommand = 1;
    xrnBusyCursor();
    TextUnsetSelection(NewsgroupText);
    if ((int) client_data == XRNgoto_GOTO) {
	name = GetDialogValue(GotoNewsgroupBox);
	if (name[0] == '\0') {
	    mesgPane(XRN_INFO, 0, NO_NG_SPECIFIED_MSG);
	}
	else {
	     ret = enterNewsgroup(name, ENTER_SETUP | ENTER_UNSUBBED |
				  ENTER_SUBSCRIBE | ENTER_REGEXP);
	     if (ret == XRN_NOUNREAD) {
		  /*
		   * Use CurrentGroup->name instead of just name
		   * because the name specified might be a regular
		   * expression which matched a group name.
		   */
		  mesgPane(XRN_INFO, 0, DISPLAYING_LAST_UNREAD_MSG,
			   CurrentGroup->name);
		  ret = GOOD_GROUP;
	     }
	     if (ret == GOOD_GROUP) {
		  name = CurrentGroup->name;
		  LastGroup = XtRealloc(LastGroup, strlen(name) + 1);
		  (void) strcpy(LastGroup, name);
		  CurrentIndexGroup = XtRealloc(CurrentIndexGroup,
						strlen(name) + 1);
		  (void) strcpy(CurrentIndexGroup, name);
		  switchToArticleMode();
	     }
	     else if (ret == BAD_GROUP) {
		  mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_MSG, name);
	     }
	     else if (ret == XRN_NOMORE) {
		  mesgPane(XRN_SERIOUS, 0, NO_ARTICLES_MSG, name);
	     }
	     else {
		  mesgPane(XRN_SERIOUS, 0, UNKNOWN_FUNC_RESPONSE_MSG, ret,
			   "enterNewsgroup", "gotoHandler");
	     }
	}
	XtFree(GotoNewsgroupString);
	GotoNewsgroupString = GetDialogValue(GotoNewsgroupBox);
	GotoNewsgroupString = XtNewString(GotoNewsgroupString);
    }
    PopDownDialog(GotoNewsgroupBox);
    GotoNewsgroupBox = 0;
    xrnUnbusyCursor();
    inCommand = 0;
    return;
}

/*
 * Jump to a newsgroup not displayed in newsgroup mode (either because
 * it's not subscribed to, or because all the articles have been read)
 *
 */
/*ARGSUSED*/
void ngGotoFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
      {ABORT_STRING,    gotoHandler, (XtPointer) XRNgoto_ABORT},
      {GOTO_NG_STRING , gotoHandler, (XtPointer) XRNgoto_GOTO},
    };

    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }

    if (GotoNewsgroupBox == (Widget) 0) {
      GotoNewsgroupBox = CreateDialog(TopLevel, GROUP_TO_GO_MSG,
				    GotoNewsgroupString == NULL ?
				    DIALOG_TEXT : GotoNewsgroupString,
				    args, XtNumber(args));
    }
    PopUpDialog(GotoNewsgroupBox);
    return;
}

/*ARGSUSED*/
void ngListOldFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }
    NewsgroupDisplayMode = (NewsgroupDisplayMode == 0) ? 1 : 0;
    redrawNewsgroupTextWidget(0, False);
    return;
}

/*
 * Enter "all" mode.  Display all available groups to allow user to
 * subscribe/unsubscribe to them.
 */
/*ARGSUSED*/
void ngAllGroupsFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    
    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }
    cancelPrefetch();
    switchToAllMode();
}

/*
 * query the server to see if there are any new articles and groups
 */
/*ARGSUSED*/
void ngRescanFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }
    rescanServer(False);
    determineMode();
    
    return;
}

/*
 * query the server to see if there are any new articles and groups, by
 * fetching the active list even if cacheActive is true.
 */

static void getListNG()
{
  rescanServer(True);
  determineMode();
}

void ngGetListFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
  confirmBox(OK_GETLIST_MSG, NEWSGROUP_MODE, NG_GETLIST, getListNG);
}

/*
 * put the user in the previous newsgroup accessed
 */
/*ARGSUSED*/
void ngPrevGroupFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    int ret;

    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }
    if (LastGroup && *LastGroup) {
	ret = enterNewsgroup(LastGroup, ENTER_SETUP | ENTER_UNSUBBED |
			     ENTER_SUBSCRIBE);
	if (ret == XRN_NOUNREAD) {
	    mesgPane(XRN_INFO, 0, DISPLAYING_LAST_UNREAD_MSG, LastGroup);
	    ret = GOOD_GROUP;
	}
	if (ret == GOOD_GROUP) {
	    switchToArticleMode();
	}
	else if (ret == BAD_GROUP) {
	    mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, LastGroup);
	}
	else if (ret == XRN_NOMORE) {
	    mesgPane(XRN_SERIOUS, 0, NO_ARTICLES_MSG, LastGroup);
	}
	else {
	    mesgPane(XRN_SERIOUS, 0, UNKNOWN_FUNC_RESPONSE_MSG, ret,
		     "enterNewsgroup", "ngPrevGroupFunction");
	}
    }
    else {
	mesgPane(XRN_INFO, 0, NO_PREV_NG_MSG);
    }

    return;
}

/* 
 * save the user's selection of groups to be moved with the move
 * command
 */
/*ARGSUSED*/
void ngSelectFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }
    if (TextGetSelectedOrCurrentLines(NewsgroupText, &First, &Last))
	TextUnsetSelection(NewsgroupText);

    setButtonSensitive(NewsgroupButtonBox, "ngMove", First != Last);

    return;
}

/*
 * Move the previously selected groups to the position before the
 * current selection
 */
/*ARGSUSED*/
void ngMoveFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *newGroup = 0;
    char *oldGroup = 0;
    int status = SUBSCRIBE;
    char *newString;
    long left;
    long stringPoint;
    long cursorSpot;
    int direction = 0;

    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }
    if (First == Last) {
	mesgPane(XRN_INFO, 0, NO_GROUPS_SELECTED_MSG);
	return;	
    }
    buildString(&newString, First, Last, NewsGroupsString);
    stringPoint = 0;
    (void) newsgroupNewsgroupIterator(True, &left);
    if ((left >= First) && (left <= Last+1)) {
	mesgPane(XRN_SERIOUS, 0, NG_NOT_MOVED_MSG);
	XtFree(newString);
	return;
    }
    cursorSpot = left;
    if (left > First) {
	direction = 1;
    }
    currentGroup(CurrentMode, newString, &newGroup, stringPoint);
    if (!moveCursor(BACK, NewsGroupsString, &left)) {
	(void) addToNewsrcBeginning(newGroup, status);
	oldGroup = XtRealloc(oldGroup, strlen(newGroup) + 1);
	(void) strcpy(oldGroup, newGroup);
	(void) moveCursor(FORWARD, newString, &stringPoint);
    } else {
	currentGroup(CurrentMode, NewsGroupsString, &oldGroup, left);
	(void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
	oldGroup = XtRealloc(oldGroup, strlen(newGroup) + 1);
	(void) strcpy(oldGroup, newGroup);
	(void) moveCursor(FORWARD, newString, &stringPoint);
    }
    while (newString[stringPoint] != '\0') {
	currentGroup(CurrentMode, newString, &newGroup, stringPoint);
	(void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
	oldGroup = XtRealloc(oldGroup, strlen(newGroup) + 1);
	(void) strcpy(oldGroup, newGroup);
	if (!moveCursor(FORWARD, newString, &stringPoint)) {
	    break;
	}
    }
    XtFree(newString);
    XtFree(oldGroup);
    XtFree(newGroup);
    CurrentIndexGroup = XtRealloc(CurrentIndexGroup, strlen(newGroup) + 1);
    (void) strcpy(CurrentIndexGroup, newGroup);
    updateNewsgroupMode(True, False);
}

/*
 * Quit xrn, leaving the newsrc in the state it was in at
 * the last invokation of rescan.
 */
/*ARGSUSED*/
void ngExitFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (pendingCompositionP()) {
	mesgPane(XRN_SERIOUS, 0, PENDING_COMPOSITION_MSG);
	XBell(XtDisplay(TopLevel), 0);
	return;
    }
    confirmBox(ARE_YOU_SURE_MSG, NEWSGROUP_MODE, NG_EXIT, ehNoUpdateExitXRN);
}

/*
 * update the .newsrc file
 */
/*ARGSUSED*/
void ngCheckPointFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }

    while (!updatenewsrc())
      ehErrorRetryXRN(ERROR_CANT_UPDATE_NEWSRC_MSG, True);

    return;
}

/*
 * Allow user to gripe
 */
/*ARGSUSED*/
void ngGripeFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }
    gripe();
    return;
}

/*
 * allow user to post an article
 */
/*ARGSUSED*/
void ngPostFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }
    post(False);
    
    return;
}

/*
 * allow user to post an article and mail it
 */
/*ARGSUSED*/
void ngPostAndMailFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }
    post_and_mail(False);
    
    return;
}

/*
 * allow user to mail a message
 */
/*ARGSUSED*/
void ngMailFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }
    mail();
    
    return;
}

/*
 * called when the user wants to scroll the newsgroup list
 */
/*ARGSUSED*/
void ngScrollFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }
    TextScrollPage(NewsgroupText, FORWARD);
    currentGroup(CurrentMode, NewsGroupsString, &CurrentIndexGroup,
		 TextGetInsertionPoint(NewsgroupText));
    return;
}

/*
 * called when the user wants to scroll the newsgroup list
 */
/*ARGSUSED*/
void ngScrollBackFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }
    TextScrollPage(NewsgroupText, BACK);
    currentGroup(CurrentMode, NewsGroupsString, &CurrentIndexGroup,
		 TextGetInsertionPoint(NewsgroupText));
    return;
}

/*
  Rebuild the newsgroup text window.

  If newsgroup is non-null, update only that line in the existing
  newsgroup list, if it's there, either changing what it says or
  deleting it completely if it's not in the new list.

  If it's not there, or if newsgroup is null, replace the whole list.
  */
void redrawNewsgroupTextWidget(newsgroup, skip_last)
    String newsgroup;
    Boolean skip_last;
{
    long GroupPosition, NewPosition;
    String new;
    char *old_name = 0, *new_name = 0;

    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }

    TextDisableRedisplay(NewsgroupText);

    new = unreadGroups(TextGetColumns(NewsgroupText), NewsgroupDisplayMode);

    if (newsgroup && NewsGroupsString) {
	old_name = XtNewString(newsgroup);
	GroupPosition = getNearbyNewsgroup(NewsGroupsString, &old_name);
	if (! strcmp(newsgroup, old_name)) {
	    new_name = XtNewString(newsgroup);
	    NewPosition = getNearbyNewsgroup(new, &new_name);
	    if (! strcmp(old_name, new_name)) {
		String str = new + NewPosition;
		int len = (index(str, '\n') - str) + 1;
		long left = GroupPosition;
		long right = (index(NewsGroupsString +
					       GroupPosition, '\n') -
					 NewsGroupsString) + 1;
		if (strncmp(str, NewsGroupsString + GroupPosition, len))
		    TextReplace(NewsgroupText, str, len, left, right);
		strncpy(NewsGroupsString + GroupPosition, str, len);
	    }
	    else {
		resetSelection();
		TextRemoveLine(NewsgroupText, GroupPosition);
		removeLine(NewsGroupsString, &GroupPosition);
	    }
	    FREE(new);
	}
    }

    if (new) {
	if (!NewsGroupsString || strcmp(NewsGroupsString, new)) {
	    resetSelection();
	    FREE(NewsGroupsString);
	    NewsGroupsString = new;
	    TextSetString(NewsgroupText, NewsGroupsString);
	}
	else {
	    FREE(new);
	}
    }

    if (utStrlen(NewsGroupsString) == 0) {
	CurrentIndexGroup = XtRealloc(CurrentIndexGroup, 1);
	*CurrentIndexGroup = '\0';
	setTopInfoLine(NO_MORE_UNREAD_ART_MSG);
	if (XtIsRealized(TopLevel))
	    xmSetIconAndName(ReadIcon);
    } else {
	setTopInfoLine(OPEARATION_APPLY_CURSOR_MSG);
	if (XtIsRealized(TopLevel)) {
	    if (unreadNews())
		xmSetIconAndName(UnreadIcon);
	    else
		xmSetIconAndName(ReadIcon);
	}
    }

    GroupPosition = getNearbyNewsgroup(NewsGroupsString, &CurrentIndexGroup);
    if (skip_last && *CurrentIndexGroup &&
	LastGroup && STREQ(CurrentIndexGroup, LastGroup)) {
	(void) moveUpWrap(NewsGroupsString, &GroupPosition);
	currentGroup(CurrentMode, NewsGroupsString, &CurrentIndexGroup,
		     GroupPosition);
    }

    setNewsgroupPosition(GroupPosition);

    XtFree(old_name);
    XtFree(new_name);
    TextEnableRedisplay(NewsgroupText);
}


/*
 * update the info line and update the newsgroup text window
 */
void updateNewsgroupMode(prefetch, skip_last)
    Boolean prefetch, skip_last;
{
    if (CurrentMode != NEWSGROUP_MODE)
	return;

    redrawNewsgroupTextWidget(0, skip_last);
    if (prefetch && CurrentIndexGroup && *CurrentIndexGroup)
	prefetchGroup(CurrentIndexGroup);

    return;
}


/*
 * install the newsgroup mode buttons (and the delete the previous mode buttons)
 * and then go to newsgroup mode
 */
void switchToNewsgroupMode(skip_last)
    Boolean skip_last;
{
    PreviousMode = CurrentMode;
    CurrentMode = NEWSGROUP_MODE;

    /* switch buttons */
    swapMode();

    FREE(NewsGroupsString);

    setButtonSensitive(NewsgroupButtonBox, "ngPrevGroup", LastGroup && *LastGroup);

    /* update the newsgroup mode windows */
    updateNewsgroupMode(True, skip_last);
    
    return;
}

/*
  Do a prefetch in newsgroup mode, or do nothing in any other mode.
  First figures out the group to prefetch by using getSelection to get
  either the group on the current line or the first group in the
  current region.  If it finds a group, puts it into CurrentIndexGroup
  and starts a prefetch for it.
  */

void doPrefetch(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    long left, right;

    if (CurrentMode != NEWSGROUP_MODE)
	return;

    if (! TextGetSelectedOrCurrentLines(NewsgroupText, &left, &right)) {
	cancelPrefetch();
	return;
    }

    currentGroup(CurrentMode, NewsGroupsString, &CurrentIndexGroup, left);

    if (CurrentIndexGroup && *CurrentIndexGroup)
	prefetchGroup(CurrentIndexGroup);

    return;
}

/* handle autorescan timeouts */
static XtIntervalId TimeOut = 0;

static void autoRescan _ARGUMENTS((XtPointer, XtIntervalId *));

void addTimeOut()
{
    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }

    if (app_resources.rescanTime <= 0) {
	return;
    }

    /* do not allow recursive timeouts */
    if (TimeOut) {
	return;
    }
    /* handle race conditions??? */
    TimeOut = 1;

    TimeOut = XtAppAddTimeOut(TopContext,
			      app_resources.rescanTime * 1000, autoRescan, 0);
    return;
}


void removeTimeOut()
{
    XtIntervalId temp;

    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }

    /* handle race conditions??? */
    temp = TimeOut;
    TimeOut = 0;

    /* do not allow recursive timeouts */
    if (temp) {
	XtRemoveTimeOut(temp);
    }
    return;
}

/*ARGSUSED*/
static void autoRescan(data, id)
    XtPointer data;
    XtIntervalId *id;
{
    if (CurrentMode != NEWSGROUP_MODE) {
	TimeOut = 0;
	return;
    }
    if (TimeOut != *id) {
	TimeOut = 0;
	return;
    }
    TimeOut = 0;
    xrnBusyCursor();
    infoNow(AUTOMATIC_RESCAN_MSG);
    ngRescanFunction(NULL, NULL, NULL, NULL);
    infoNow("");
    xrnUnbusyCursor();
    addTimeOut();

    return;
}

void displayNewsgroupWidgets()
{
    if (! NewsgroupFrame) {
	NewsgroupFrame = XtCreateManagedWidget("ngFrame",
					       panedWidgetClass,
					       TopLevel, 0, 0);

	XawPanedSetRefigureMode(NewsgroupFrame, False);

	NewsgroupText = TextCreate("newsgroups", True, NewsgroupFrame);
	TextSetLineSelections(NewsgroupText);
	TextDisableWordWrap(NewsgroupText);

	NewsgroupInfoLine = InfoLineCreate("info", 0, NewsgroupFrame);
	TopInfoLine = NewsgroupInfoLine;

	NewsgroupButtonBox = ButtonBoxCreate("buttons", NewsgroupFrame);
	doButtons(app_resources.ngButtonList, NewsgroupButtonBox,
		  NewsgroupButtonList, &NewsgroupButtonListCount, TOP);
	XtManageChild(NewsgroupButtonBox);

	setButtonSensitive(NewsgroupButtonBox, "ngPost", PostingAllowed);
	setButtonSensitive(NewsgroupButtonBox, "ngPostAndMail",
			   PostingAllowed);

	XawPanedSetRefigureMode(NewsgroupFrame, True);

	XtSetKeyboardFocus(NewsgroupFrame, NewsgroupText);
    }
    else {
	TopInfoLine = NewsgroupInfoLine;
	XtManageChild(NewsgroupFrame);
    }
}

void hideNewsgroupWidgets()
{
    XtUnmanageChild(NewsgroupFrame);
}

String getNewsgroupString()
{
    if (NewsGroupsString)
	return XtNewString(NewsGroupsString);
    else
	return(0);
}

void ngDoTheRightThing(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (count && *count == 1 && strcmp(string[0], "jump") == 0) {
	NewsgroupEntryMode = NG_ENTRY_JUMP;
    }
    ngReadFunction(widget, event, string, count);
}
