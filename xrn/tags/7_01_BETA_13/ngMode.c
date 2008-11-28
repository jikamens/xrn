#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

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
#include "mesg.h"
#include "cursor.h"
#include "dialogs.h"
#include "newsrcfile.h"
#include "compose.h"
#include "artMode.h"
#include "allMode.h"

static String NewsGroupsString;

static char *GotoNewsgroupString = 0;
/*
  The group we're currently positioned on in the newsgroup index.
  */
char CurrentIndexGroup[GROUP_NAME_SIZE];
char LastGroup[GROUP_NAME_SIZE];
int NewsgroupDisplayMode = 0;	/* 0 for unread groups, 1 for all sub */
int NewsgroupEntryMode = NG_ENTRY_GOTO;

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
BUTTON(ngPrevGroup,prev group);
BUTTON(ngSelect,select groups);
BUTTON(ngMove,move);
BUTTON(ngCheckPoint,checkpoint);
BUTTON(ngPost,post);
BUTTON(ngGripe,gripe);
BUTTON(ngScroll,scroll forward);
BUTTON(ngScrollBack,scroll backward);

XtActionsRec NgActions[] = {
    {"ngQuit",		ngQuitAction},
    {"ngRead",		ngReadAction},
    {"ngNext",		ngNextAction},
    {"ngPrev",		ngPrevAction},
    {"ngCatchUp",	ngCatchUpAction},
    {"ngSubscribe",	ngSubscribeAction},
    {"ngUnsub",		ngUnsubAction},
    {"ngGoto",		ngGotoAction},
    {"ngRescan",	ngRescanAction},
    {"ngAllGroups",	ngAllGroupsAction},
    {"ngToggleGroups",	ngListOldAction},
    {"ngPrevGroup",	ngPrevGroupAction},
    {"ngSelect",	ngSelectAction},
    {"ngMove",		ngMoveAction},
    {"ngExit",		ngExitAction},
    {"ngGripe",		ngGripeAction},
    {"ngPost",		ngPostAction},
    {"ngCheckPoint",	ngCheckPointAction},
    {"ngScroll",	ngScrollAction},
    {"ngScrollBack",	ngScrollBackAction},
};    

int NgActionsCount = XtNumber(NgActions);

ButtonList NgButtonList[] = {
    {ngQuitArgs, XtNumber(ngQuitArgs),
    NGQUIT_EXSTR},
    {ngReadArgs, XtNumber(ngReadArgs),
    NGREAD_EXSTR},
    {ngNextArgs, XtNumber(ngNextArgs),
    NGNEXT_EXSTR},
    {ngPrevArgs, XtNumber(ngPrevArgs),
    NGPREV_EXSTR},
    {ngCatchUpArgs, XtNumber(ngCatchUpArgs),
    NGCATCHUP_EXSTR},
    {ngSubscribeArgs, XtNumber(ngSubscribeArgs),
    NGSUBSCRIBE_EXSTR},
    {ngUnsubArgs, XtNumber(ngUnsubArgs),
    NGUNSUB_EXSTR},
    {ngGotoArgs, XtNumber(ngGotoArgs),
    NGGOTO_EXSTR},
    {ngAllGroupsArgs, XtNumber(ngAllGroupsArgs),
    NGALLGROUPS_EXSTR},
    {ngListOldArgs, XtNumber(ngListOldArgs),
    NGLISTOLD_EXSTR},
    {ngRescanArgs, XtNumber(ngRescanArgs),
    NGRESCAN_EXSTR},
    {ngPrevGroupArgs, XtNumber(ngPrevGroupArgs),
    NGPREVGROUP_EXSTR},
    {ngSelectArgs, XtNumber(ngSelectArgs),
    NGSELECT_EXSTR},
    {ngMoveArgs, XtNumber(ngMoveArgs),
    NGMOVE_EXSTR},
    {ngExitArgs, XtNumber(ngExitArgs),
    NGEXIT_EXSTR},
    {ngCheckPointArgs, XtNumber(ngCheckPointArgs),
    NGCHECKPOINT_EXSTR},
    {ngGripeArgs, XtNumber(ngGripeArgs),
    NGGRIPE_EXSTR},
    {ngPostArgs, XtNumber(ngPostArgs),
    NGPOST_EXSTR},
    {ngScrollArgs, XtNumber(ngScrollArgs),
    NGSCROLL_EXSTR},
    {ngScrollBackArgs, XtNumber(ngScrollBackArgs),
    NGSCROLLBACK_EXSTR},
};

int NgButtonListCount = XtNumber(NgButtonList);

/*
  Set the current insertion point in the newsgroup widget, saving the
  current display position first (since setting the insertion point
  causes the window to scroll so that the cursor is visible).
  */
static void setNewsgroupPosition(position, adjust_top)
    long position;
    Boolean adjust_top;
{
    long top = TextGetTopPosition(Text);

    TextDisableRedisplay(Text);

    TextSetInsertionPoint(Text, position);
    if (adjust_top)
	adjustMinMaxLines(NewsGroupsString);
    else
	TextSetTopPosition(Text, top);

    TextEnableRedisplay(Text);
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
    confirmBox(ARE_YOU_SURE_MSG, NEWSGROUP_MODE, NG_QUIT, ehCleanExitXRN);
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

    resetSelection();

    if (newsgroupIterator(NewsGroupsString, True, False, 0) &&
	(name = newsgroupIterator(NewsGroupsString, False, False, 0))) {
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
		mesgPane(XRN_SERIOUS, 0, UNKNOWN_ENTER_NG_RESPONSE_MSG, status,
			 "ngReadFunction");
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
    resetSelection();

    (void) newsgroupIterator(NewsGroupsString, True, False, &left);
    (void) moveUpWrap(NewsGroupsString, &left);
    /*
      Optimization -- XawTextSetInsertionPoint always redisplays the
      whole Text widget.  However, the next-line action procedure
      doesn't.  Therefore, we use that action procedure here (and
      below in ngPrevFunction), because in most cases, it'll put the
      insertion point in the right place, and a redraw won't be
      necessary.
      */
    TextMoveLine(Text, FORWARD);
    currentGroup(CurrentMode, NewsGroupsString, CurrentIndexGroup, left);
    left = getNearbyNewsgroup(NewsGroupsString, CurrentIndexGroup);
    setNewsgroupPosition(left, True);
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
    resetSelection();

    (void) newsgroupIterator(NewsGroupsString, True, False, &left);
    (void) moveCursor(BACK, NewsGroupsString, &left);
    TextMoveLine(Text, BACK);
    currentGroup(CurrentMode, NewsGroupsString, CurrentIndexGroup, left);
    left = getNearbyNewsgroup(NewsGroupsString, CurrentIndexGroup);
    setNewsgroupPosition(left, True);
}


/*
 * used when the user has elected to catch
 * up newsgroups in newsgroup mode
 */
static void catchUpNG()
{
    String name;
    int ret;

    if (newsgroupIterator(NewsGroupsString, True, False, 0)) {
	while ((name = newsgroupIterator(NewsGroupsString, False, False, 0))) {
	    (void) strcpy(CurrentIndexGroup, name);
	    if ((ret = enterNewsgroup(name, ENTER_UNSUBBED))
		== GOOD_GROUP) {
		catchUp();
	    }
	    else if (ret == BAD_GROUP) {
		 mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, name);
	    }
	    else {
		 mesgPane(XRN_SERIOUS, 0, UNKNOWN_ENTER_NG_RESPONSE_MSG, ret,
			  "catchUpNG");
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

    if (newsgroupIterator(NewsGroupsString, True, False, 0)) {
	while ((name = newsgroupIterator(NewsGroupsString, False, False, 0))) {
	    (void) strcpy(CurrentIndexGroup, name);
	    if ((ret = enterNewsgroup(name, ENTER_UNSUBBED))
		== GOOD_GROUP) {
		unsubscribe();
	    }
	    else if (ret == BAD_GROUP) {
		 mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, name);
	    }
	    else {
		mesgPane(XRN_SERIOUS, 0, UNKNOWN_ENTER_NG_RESPONSE_MSG, ret,
			 "unsubscribeNG");
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
    char *group, name[GROUP_NAME_SIZE];
    long left, insertion;
    int ret;

    if (inCommand) {
	return;
    }
    inCommand = 1;
    busyCursor();

    TextDisableRedisplay(Text);

    switch ((int) client_data) {
    case XRNsub_LASTGROUP:
	if (LastGroup[0] != '\0') {
	    if ((ret = enterNewsgroup(LastGroup, ENTER_UNSUBBED))
		== BAD_GROUP) {
		mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, LastGroup);
		goto done;
	    }
	    else if (ret == GOOD_GROUP) {
		subscribe();
		(void) strcpy(CurrentIndexGroup, LastGroup);
		updateNewsgroupMode(True, False);
	    }
	    else {
		mesgPane(XRN_SERIOUS, 0, UNKNOWN_ENTER_NG_RESPONSE_MSG, ret,
			 "subscribeHandler");
	    }
	}
	break;

    case XRNsub_FIRST:
	if (addToNewsrcBeginning(group = GetDialogValue(SubscribeBox),
				 status) == GOOD_GROUP) {
	    (void) strcpy(CurrentIndexGroup, group);
	    updateNewsgroupMode(True, False);
	}
	break;

    case XRNsub_LAST:
	if (addToNewsrcEnd(group = GetDialogValue(SubscribeBox),
			   status) == GOOD_GROUP) {
	    (void) strcpy(CurrentIndexGroup, group);
	    updateNewsgroupMode(True, False);
	}
	break;

    case XRNsub_CURRENT:
	insertion = TextGetInsertionPoint(Text);

	if (! NewsGroupsString[insertion]) {
	    if (addToNewsrcEnd(group = GetDialogValue(SubscribeBox),
			       status) == GOOD_GROUP) {
		(void) strcpy(CurrentIndexGroup, group);
		updateNewsgroupMode(True, False);
	    }
	} else {
	    /* don't need to check for the null group here, it would have */
	    /* been already handled above */
	    (void) newsgroupIterator(NewsGroupsString, True, False, &left);
	    if (left == 0) {
	        if (addToNewsrcBeginning(group = GetDialogValue(SubscribeBox),
					 status) == GOOD_GROUP) {
		    (void) strcpy(CurrentIndexGroup, group);
		}
	    } else {
		(void) moveCursor(BACK, NewsGroupsString, &left);
		currentGroup(CurrentMode, NewsGroupsString, name, left);
	        if (addToNewsrcAfterGroup(group = GetDialogValue(SubscribeBox),
					  name, status) == GOOD_GROUP) {
		    (void) strcpy(CurrentIndexGroup, group);
		}
	    }
	    updateNewsgroupMode(True, False);
    	}
	break;
    }

  done:
    TextEnableRedisplay(Text);
    PopDownDialog(SubscribeBox);
    SubscribeBox = 0;
    unbusyCursor();
    inCommand = 0;
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
    resetSelection();
    
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
    busyCursor();
    TextUnsetSelection(Text);
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
		  (void) strcpy(LastGroup, name);
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
		  mesgPane(XRN_SERIOUS, 0, UNKNOWN_ENTER_NG_RESPONSE_MSG, ret,
			   "gotoHandler");
	     }
	}
	XtFree(GotoNewsgroupString);
	GotoNewsgroupString = GetDialogValue(GotoNewsgroupBox);
	GotoNewsgroupString = XtNewString(GotoNewsgroupString);
    }
    PopDownDialog(GotoNewsgroupBox);
    GotoNewsgroupBox = 0;
    unbusyCursor();
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
    resetSelection();
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
    redrawNewsgroupTextWidget(0, False, True);
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
    resetSelection();
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
    resetSelection();
    rescanServer();
    determineMode();
    
    return;
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
    resetSelection();
    if (LastGroup[0] != '\0') {
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
	    mesgPane(XRN_SERIOUS, 0, UNKNOWN_ENTER_NG_RESPONSE_MSG, ret,
		     "ngPrevGroupFunction");
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
    if (TextGetSelectedOrCurrentLines(Text, &First, &Last))
	TextUnsetSelection(Text);
    
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
    char newGroup[GROUP_NAME_SIZE];
    char oldGroup[GROUP_NAME_SIZE];
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
    (void) newsgroupIterator(NewsGroupsString, True, False, &left);
    if ((left >= First) && (left <= Last+1)) {
	mesgPane(XRN_SERIOUS, 0, NG_NOT_MOVED_MSG);
	resetSelection();
	return;
    }
    cursorSpot = left;
    if (left > First) {
	direction = 1;
    }
    currentGroup(CurrentMode, newString, newGroup, stringPoint);
    if (!moveCursor(BACK, NewsGroupsString, &left)) {
	(void) addToNewsrcBeginning(newGroup, status);
	(void) strcpy(oldGroup, newGroup);
	(void) moveCursor(FORWARD, newString, &stringPoint);
    } else {
	currentGroup(CurrentMode, NewsGroupsString, oldGroup, left);
	(void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
	(void) strcpy(oldGroup, newGroup);
	(void) moveCursor(FORWARD, newString, &stringPoint);
    }
    while (newString[stringPoint] != '\0') {
	currentGroup(CurrentMode, newString, newGroup, stringPoint);
	(void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
	(void) strcpy(oldGroup, newGroup);
	if (!moveCursor(FORWARD, newString, &stringPoint)) {
	    break;
	}
    }
    (void) strcpy(CurrentIndexGroup, newGroup);
    updateNewsgroupMode(True, False);
    resetSelection();
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
    post(0);
    
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
    TextScrollPage(Text, FORWARD);
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
    TextScrollPage(Text, BACK);
    return;
}

/*
  Rebuild the newsgroup text window.

  If newsgroup is non-null, update only that line in the existing
  newsgroup list, if it's there, either changing what it says or
  deleting it completely if it's not in the new list.

  If it's not there, or if newsgroup is null, replace the whole list.
  */
void redrawNewsgroupTextWidget(newsgroup, skip_last, adjust_list)
    String newsgroup;
    Boolean skip_last, adjust_list;
{
    long GroupPosition, NewPosition;
    String new;
    char old_name[GROUP_NAME_SIZE], new_name[GROUP_NAME_SIZE];

    if (CurrentMode != NEWSGROUP_MODE) {
	return;
    }

    TextDisableRedisplay(Text);

    new = unreadGroups(NewsgroupDisplayMode);

    if (newsgroup && NewsGroupsString) {
	(void) strcpy(old_name, newsgroup);
	GroupPosition = getNearbyNewsgroup(NewsGroupsString, old_name);
	if (! strcmp(newsgroup, old_name)) {
	    (void) strcpy(new_name, newsgroup);
	    NewPosition = getNearbyNewsgroup(new, new_name);
	    if (! strcmp(old_name, new_name)) {
		String str = new + NewPosition;
		int len = (index(str, '\n') - str) + 1;
		long left = GroupPosition;
		long right = (index(NewsGroupsString +
					       GroupPosition, '\n') -
					 NewsGroupsString) + 1;
		if (strncmp(str, NewsGroupsString + GroupPosition, len))
		    TextReplace(Text, str, len, left, right);
		strncpy(NewsGroupsString + GroupPosition, str, len);
	    }
	    else {
		TextRemoveLine(Text, GroupPosition);
		removeLine(NewsGroupsString, &GroupPosition);
	    }
	    FREE(new);
	}
    }

    if (new && (!NewsGroupsString || strcmp(NewsGroupsString, new))) {
	FREE(NewsGroupsString);
	NewsGroupsString = new;
	TextSetString(Text, NewsGroupsString);
    }

    if (utStrlen(NewsGroupsString) == 0) {
	*CurrentIndexGroup = '\0';
	setTopInfoLine(NO_MORE_UNREAD_ART_MSG);
	if (XtIsRealized(TopLevel))
	    xmSetIconAndName(ReadIcon);
    } else {
	if (XtIsRealized(TopLevel)) {
	    if (unreadNews())
		xmSetIconAndName(UnreadIcon);
	    else
		xmSetIconAndName(ReadIcon);
	}
    }

    GroupPosition = getNearbyNewsgroup(NewsGroupsString, CurrentIndexGroup);
    if (skip_last &&
	*CurrentIndexGroup && STREQ(CurrentIndexGroup, LastGroup)) {
	(void) moveUpWrap(NewsGroupsString, &GroupPosition);
	currentGroup(CurrentMode, NewsGroupsString, CurrentIndexGroup,
		     GroupPosition);
    }

    setNewsgroupPosition(GroupPosition, adjust_list);

    TextEnableRedisplay(Text);
}


/*
 * update the info line and update the newsgroup text window
 */
void updateNewsgroupMode(prefetch, skip_last)
    Boolean prefetch, skip_last;
{
    if (CurrentMode != NEWSGROUP_MODE)
	return;

    if (PreviousMode != NEWSGROUP_MODE) {
	setTopInfoLine(OPEARATION_APPLY_CURSOR_MSG);
    }
    redrawNewsgroupTextWidget(0, skip_last, True);
    if (prefetch && *CurrentIndexGroup)
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

    resetSelection();

    /* switch buttons */
    swapMode();

    FREE(NewsGroupsString);

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

    if (! TextGetSelectedOrCurrentLines(Text, &left, &right))
	return;

    currentGroup(CurrentMode, NewsGroupsString, CurrentIndexGroup, left);

    if (*CurrentIndexGroup)
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
    busyCursor();
    infoNow(AUTOMATIC_RESCAN_MSG);
    ngRescanFunction(NULL, NULL, NULL, NULL);
    infoNow("");
    unbusyCursor();
    addTimeOut();

    return;
}

