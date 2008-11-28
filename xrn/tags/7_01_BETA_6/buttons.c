
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: buttons.c,v 1.35 1994-12-05 18:42:42 jik Exp $";
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
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#ifndef MOTIF
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Dialog.h>
#define XawStringSourceDestroy XtDestroyWidget
#else
#include <string.h>
#include <Xm/PanedW.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/MessageB.h>
#include <Xm/List.h>
#endif

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

#ifndef O_RDONLY
#define O_RDONLY 0
#endif

static off_t ArticleTextFilesize;		/* size of the file */

#ifdef VMS
#define getArticle getArticleFile
#include <descrip.h>
#endif

static Widget *AddButtons = NIL(Widget);
static Widget *NgButtons = NIL(Widget);
static Widget *AllButtons = NIL(Widget);
static Widget *ArtButtons = NIL(Widget);
static Widget *ArtSpecButtons = NIL(Widget);

static char *AddGroupsString = NIL(char);  /* new newsgroups list ...           */
static char *NewsGroupsString = NIL(char); /* newsgroups list that is displayed */
static char *AllGroupsString = NIL(char); /* list of all groups so the user */
				          /* can subscribe/unsubscribe to them */
static char *SubjectString = NIL(char); /* list of article numbers and subjects */

static XawTextPosition GroupPosition = (XawTextPosition) 0; /* cursor position */
				/* newsgroup window */
static XawTextPosition ArtPosition = (XawTextPosition) 0;	/* cursor position in */
				/* article subject window */
static XawTextPosition NewsgroupTop = (XawTextPosition) 0; /* top position in */
				/* newsgroup window */
static XawTextPosition ArticleTop = (XawTextPosition) 0; /* top position in */
				/* article subject window */

static char LastGroup[GROUP_NAME_SIZE];	/* last newsgroup accessed; used to */
				/* determine whether or not to move the */
				/* cursor in the newsgroup window */
static char *LastArticle;	/* the article currently displayed */
				/* in the article window */
static art_num CurrentArticle;	/* the number of the article currently */
				/* displayed, used for marking an article */
				/* as saved */

static art_num PrevArticle;	/* the number of the article displayed */
				/* before the current one */

static int ArtStatus = 0;	/* keeps track of what kind of article to */
				/* to search for: next, previous, or next */
				/* unread */
static int AllStatus = 1;	/* keeps track of which order to put the */
				/* groups in in all groups mode */
static char *LastRegexp;	/* last regular expression searched for */
static int LastSearch;		/* the direction of the last regular */
				/* expression search */

static XawTextPosition First;	/* keeps the beginning of the selected text */
				/* for the command move groups */
static XawTextPosition Last;	/* keeps the end of the selected text for */
				/* the command move groups */

static int Action;		/* action to take when a confirmation box */
				/* is clicked in */

static int NewsgroupDisplayMode = 0;	/* 0 for unread groups, 1 for all sub */
#define XRN_JUMP 0
#define XRN_GOTO 1
static int NewsgroupEntryMode = XRN_GOTO;

static char *SaveString = 0;	/* last input to save box */
static char *GotoNewsgroupString = 0;
static int ArtEntry = 1;

/* article mode "modes" ... determines what to do: jump out of article */
/* mode, change the subject string, or do nothing */
#define art_DONE 0
#define art_CHANGE 1
#define art_NOCHANGE 2

/* keeps track of which type of article to search for: Next, Unread, or */
/* previous */
#define art_NEXT 0
#define art_UNREAD 1
#define art_PREV 2
#define art_NEXTWRAP 3


static int Mode = NO_MODE;            /* current mode                       */
static int PreviousMode = NO_MODE;    /* previous mode, what buttons to */
				       /* remove */

#define XRN_NO 0
#define XRN_YES 1

/* the user is in a command - eat type ahead */
int inCommand = 0;

#ifndef MOTIF
static XawTextSelectType lineSelectArray[] = {XawselectLine, XawselectNull};

static Arg lineSelArgs[] = {
    {XtNselectTypes, (XtArgVal) lineSelectArray},
};    

static XawTextSelectType allSelectArray[] =
    {XawselectPosition, XawselectChar, XawselectWord, XawselectLine,
     XawselectParagraph, XawselectAll, XawselectNull};

static Widget AllSource = 0;
static Widget SubjectSource = 0;
static Widget ArtSource = 0;
static Widget DummySource = 0;
#else
extern char *TextMotifString;            /* Moved to xawmotif.c */
extern char *ArticleTextMotifString;     /* Moved to xawmotif.c */
#endif

static struct _translations {
    Widget widget;
    XtTranslations tables[MAX_MODE];
    char *unparsed[MAX_MODE];
} Translations[6];

/* handle autorescan timeouts */
static XtIntervalId TimeOut = 0;

/*
  XawFmt8Bit only available starting in X11R6.
  */
#ifndef XawFmt8Bit
#define XawFmt8Bit FMT8BIT
#endif

static void autoRescan _ARGUMENTS((XtPointer, XtIntervalId *));

void addTimeOut()
{
    if (Mode != NEWSGROUP_MODE) {
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

    TimeOut = XtAppAddTimeOut(XtWidgetToApplicationContext(TopLevel),
			      app_resources.rescanTime * 1000, autoRescan, 0);
    return;
}


void removeTimeOut()
{
    XtIntervalId temp;

    if (Mode != NEWSGROUP_MODE) {
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


BUTTON(addQuit,quit);
BUTTON(addFirst,add first);
BUTTON(addLast,add last);
BUTTON(addAfter,add after group);
BUTTON(addUnsub,add unsubscribed);

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

BUTTON(artQuit,quit);
BUTTON(artNext,next);
BUTTON(artNextUnread,next unread);
BUTTON(artPrev,prev);
BUTTON(artLast,last);
BUTTON(artNextGroup,next newsgroup);
BUTTON(artGotoArticle,goto article);
BUTTON(artCatchUp,catch up);
BUTTON(artFedUp,fed up);
BUTTON(artMarkRead,mark read);
BUTTON(artMarkUnread,mark unread);
BUTTON(artUnsub,unsubscribe);
BUTTON(artScroll,scroll forward);
BUTTON(artScrollBack,scroll backward);
BUTTON(artScrollLine,scroll line forward);
BUTTON(artScrollBackLine,scroll line backward);
BUTTON(artScrollEnd,scroll to end);
BUTTON(artScrollBeginning,scroll to beginning);
BUTTON(artScrollIndex, scroll index);
BUTTON(artScrollIndexBack, scroll index back);
BUTTON(artSubNext,subject next);
BUTTON(artSubPrev,subject prev);
BUTTON(artKillSession,session kill);
BUTTON(artKillLocal,local kill);
BUTTON(artKillGlobal,global kill);
BUTTON(artKillAuthor,author kill);
BUTTON(artSubSearch,subject search);
BUTTON(artContinue,continue);
BUTTON(artPost,post);
BUTTON(artExit,exit);
BUTTON(artCheckPoint,checkpoint);
BUTTON(artGripe,gripe);
BUTTON(artListOld,list old);

BUTTON(artSave,save);
BUTTON(artReply,reply);
BUTTON(artForward,forward);
BUTTON(artFollowup,followup);
BUTTON(artFollowupAndReply,followup and reply);
BUTTON(artCancel,cancel);
BUTTON(artRot13,rot-13);
#ifdef XLATE
BUTTON(artXlate,translate text);
#endif /* XLATE */
BUTTON(artHeader,toggle header);
BUTTON(artPrint,print);

void doTheRightThing _ARGUMENTS((Widget, XEvent *,String *,Cardinal *));
static XtActionsRec TopActions[] = {
    {"doTheRightThing",	doTheRightThing},
};

static XtActionsRec AddActions[] = {
    {"addQuit",		addQuitAction},
    {"addFirst",	addFirstAction},
    {"addLast",		addLastAction},
    {"addAfter",	addAfterAction},
    {"addUnsub",	addUnsubAction},
};

static XtActionsRec NgActions[] = {
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


static XtActionsRec ArtActions[] = {
    {"artQuit",		artQuitAction},
    {"artNextUnread",	artNextUnreadAction},
    {"artScroll",	artScrollAction},
    {"artScrollBack",	artScrollBackAction},
    {"artScrollLine",	artScrollLineAction},
    {"artScrollBackLine",	artScrollBackLineAction},
    {"artScrollEnd",	artScrollEndAction},
    {"artScrollBeginning",	artScrollBeginningAction},
    {"artScrollIndex",	artScrollIndexAction},
    {"artScrollIndexBack",	artScrollIndexBackAction},
    {"artNext",		artNextAction},
    {"artPrev",		artPrevAction},
    {"artLast",		artLastAction},
    {"artNextGroup",	artNextGroupAction},
    {"artCatchUp",	artCatchUpAction},
    {"artFedUp",	artFedUpAction},
    {"artGotoArticle",	artGotoArticleAction},
    {"artMarkRead",	artMarkReadAction},
    {"artMarkUnread",	artMarkUnreadAction},
    {"artUnsub",	artUnsubAction},
    {"artSubNext",	artSubNextAction},
    {"artSubPrev",	artSubPrevAction},
    {"artKillSession",	artKillSessionAction},
    {"artKillLocal",	artKillLocalAction},
    {"artKillGlobal",	artKillGlobalAction},
    {"artKillAuthor",	artKillAuthorAction},
    {"artSubSearch",	artSubSearchAction},
    {"artContinue",	artContinueAction},
    {"artPost",		artPostAction},
    {"artExit",		artExitAction},
    {"artCheckPoint",	artCheckPointAction},
    {"artSave",		artSaveAction},
    {"artReply",	artReplyAction},
    {"artForward",	artForwardAction},
    {"artFollowup",	artFollowupAction},
    {"artFollowupAndReply", artFollowupAndReplyAction},
    {"artCancel",	artCancelAction},
    {"artRot13",	artRot13Action},
#ifdef XLATE
    {"artXlate",	artXlateAction},
#endif /* XLATE */
    {"artHeader",	artHeaderAction},
    {"artPrint",	artPrintAction},
    {"artGripe",	artGripeAction},
    {"artListOld",	artListOldAction},
};

static XtActionsRec AllActions[] = {
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

typedef struct buttonList {
    Arg *buttonArgs;
    unsigned int size;
    char *message;
} ButtonList;

static ButtonList AddButtonList[] = {
#ifdef lint
    {NULL, NULL}
#else
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
#endif /* lint */
};

static int AddButtonListCount = XtNumber(AddButtonList);

static ButtonList NgButtonList[] = {
#ifdef lint
    {NULL, NULL}
#else
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
#endif /* lint */
};

static int NgButtonListCount = XtNumber(NgButtonList);

static ButtonList AllButtonList[] = {
#ifdef lint
    {NULL, NULL}
#else
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
#endif /* lint */
};

static int AllButtonListCount = XtNumber(AllButtonList);

static ButtonList ArtButtonList[] = {
#ifdef lint
    {NULL, NULL}
#else
    {artQuitArgs, XtNumber(artQuitArgs),
    ARTQUIT_EXSTR},
    {artNextUnreadArgs, XtNumber(artNextUnreadArgs),
    ARTNEXTUNREAD_EXSTR},
    {artNextArgs, XtNumber(artNextArgs),
    ARTNEXT_EXSTR},
    {artScrollArgs, XtNumber(artScrollArgs),
    ARTSCROLL_EXSTR},
    {artScrollBackArgs, XtNumber(artScrollBackArgs),
    ARTSCROLLBACK_EXSTR},
    {artScrollLineArgs, XtNumber(artScrollLineArgs),
    ARTSCROLLLINE_EXSTR},
    {artScrollBackLineArgs, XtNumber(artScrollBackLineArgs),
    ARTSCROLLBACKLINE_EXSTR},
    {artScrollEndArgs, XtNumber(artScrollEndArgs),
    ARTSCROLLEND_EXSTR},
    {artScrollBeginningArgs, XtNumber(artScrollBeginningArgs),
    ARTSCROLLBEGINNING_EXSTR},
    {artScrollIndexArgs, XtNumber(artScrollIndexArgs),
    ARTSCROLLINDEX_EXSTR},
    {artScrollIndexBackArgs, XtNumber(artScrollIndexBackArgs),
    ARTSCROLLINDEXBACK_EXSTR},
    {artPrevArgs, XtNumber(artPrevArgs),
    ARTPREV_EXSTR},
    {artLastArgs, XtNumber(artLastArgs),
    ARTLAST_EXSTR},
    {artNextGroupArgs, XtNumber(artNextGroupArgs),
    ARTNEXTGROUP_EXSTR},
    {artCatchUpArgs, XtNumber(artCatchUpArgs),
    ARTCATCHUP_EXSTR},
    {artFedUpArgs, XtNumber(artFedUpArgs),
    ARTFEEDUP_EXSTR},
    {artGotoArticleArgs, XtNumber(artGotoArticleArgs),
    ARTGOTOARTICLE_EXSTR},
    {artMarkReadArgs, XtNumber(artMarkReadArgs),
    ARTMARKREAD_EXSTR},
    {artMarkUnreadArgs, XtNumber(artMarkUnreadArgs),
    ARTMARKUNREAD_EXSTR},
    {artUnsubArgs, XtNumber(artUnsubArgs),
    ARTUNSUB_EXSTR},
    {artSubNextArgs, XtNumber(artSubNextArgs),
    ARTSUBNEXT_EXSTR},
    {artSubPrevArgs, XtNumber(artSubPrevArgs),
    ARTSUBPREV_EXSTR},
    {artKillSessionArgs, XtNumber(artKillSessionArgs),
    ARTKILLSESSION_EXSTR},
    {artKillLocalArgs, XtNumber(artKillLocalArgs),
    ARTKILLLOCAL_EXSTR},
    {artKillGlobalArgs, XtNumber(artKillGlobalArgs),
    ARTKILLGLOBAL_EXSTR},
    {artKillAuthorArgs, XtNumber(artKillAuthorArgs),
    ARTKILLAUTHOR_EXSTR},
    {artSubSearchArgs, XtNumber(artSubSearchArgs),
    ARTSUBSEARCH_EXSTR},
    {artContinueArgs, XtNumber(artContinueArgs),
    ARTCONTINUE_EXSTR},
    {artPostArgs, XtNumber(artPostArgs),
    ARTPOST_EXSTR},
    {artExitArgs, XtNumber(artExitArgs),
    ARTEXIT_EXSTR},
    {artCheckPointArgs, XtNumber(artCheckPointArgs),
    ARTCHECKPOINT_EXSTR},
    {artGripeArgs, XtNumber(artGripeArgs),
    ARTGRIPE_EXSTR},
    {artListOldArgs, XtNumber(artListOldArgs),
    ARTLISTOLD_EXSTR},
#endif /* lint */
};

static int ArtButtonListCount = XtNumber(ArtButtonList);

static ButtonList ArtSpecButtonList[] = {
#ifdef lint
    {NULL, NULL}
#else
    {artSaveArgs, XtNumber(artSaveArgs),
    ARTSAVE_EXSTR},
    {artReplyArgs, XtNumber(artReplyArgs),
    ARTREPLY_EXSTR},
    {artForwardArgs, XtNumber(artForwardArgs),
    ARTFORWARD_EXSTR},
    {artFollowupArgs, XtNumber(artFollowupArgs),
    ARTFOLLOWUP_EXSTR},
    {artFollowupAndReplyArgs, XtNumber(artFollowupAndReplyArgs),
    ARTFOLLOWUPANDREPLY_EXSTR},
    {artCancelArgs, XtNumber(artCancelArgs),
    ARTCANCEL_EXSTR},
    {artRot13Args, XtNumber(artRot13Args),
    ARTROT13_EXSTR},
#ifdef XLATE
    {artXlateArgs, XtNumber(artXlateArgs),
    ARTXLATE_EXSTR},
#endif /*XLATE */
    {artHeaderArgs, XtNumber(artHeaderArgs),
    ARTHEADER_EXSTR},
    {artPrintArgs, XtNumber(artPrintArgs),
    ARTPRINT_EXSTR},
#endif /* lint */
};

static int ArtSpecButtonListCount = XtNumber(ArtSpecButtonList);

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
    Arg infoLineArg[1];
#ifndef MOTIF

    if (event->type == LeaveNotify) {
	XtSetArg(infoLineArg[0], XtNlabel, TopNonButtonInfo);
    } else if (event->type == EnterNotify) {
	XtSetArg(infoLineArg[0], XtNlabel, client_data);
    }
    XtSetValues(TopInfoLine, infoLineArg, XtNumber(infoLineArg));
#else
    XmString xs;

    if (event->type == LeaveNotify) {
      xs = XmStringCreate(TopNonButtonInfo, XmSTRING_DEFAULT_CHARSET);
      XtSetArg(infoLineArg[0], XmNlabelString, xs);
    } else if (event->type == EnterNotify) {
      xs = XmStringCreate(client_data, XmSTRING_DEFAULT_CHARSET);
      XtSetArg(infoLineArg[0], XmNlabelString, xs);
    }
    XtSetValues(TopInfoLine, infoLineArg, 1);
    XmStringFree(xs);
#endif
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

#ifndef MOTIF
    if (event->type == LeaveNotify) {
	XtSetArg(infoLineArg[0], XtNlabel, BottomNonButtonInfo);
    } else if (event->type == EnterNotify) {
	XtSetArg(infoLineArg[0], XtNlabel, client_data);
    }
    XtSetValues(BottomInfoLine, infoLineArg, XtNumber(infoLineArg));
#else
    XmString xs;

    if (event->type == LeaveNotify) {
      xs = XmStringCreate(BottomNonButtonInfo, XmSTRING_DEFAULT_CHARSET);
      XtSetArg(infoLineArg[0], XmNlabelString, xs);
    } else if (event->type == EnterNotify) {
      xs = XmStringCreate(client_data, XmSTRING_DEFAULT_CHARSET);
      XtSetArg(infoLineArg[0], XmNlabelString, xs);
    }
    XtSetValues(BottomInfoLine, infoLineArg, 1);
    XmStringFree(xs);
#endif
    return;
}

static void resetSelection()
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

static void setTopInfoLine _ARGUMENTS((char *));

static void setTopInfoLine(message)  
    char *message;
{
    Arg infoLineArg[1];

#ifndef MOTIF
    XtSetArg(infoLineArg[0], XtNlabel, message);
    (void) strcpy(TopNonButtonInfo, (char *) infoLineArg[0].value);
    XtSetValues(TopInfoLine, infoLineArg, XtNumber(infoLineArg));
#else
    XmString xs;

    (void) strcpy(TopNonButtonInfo, message);
    xs = XmStringCreate(message, XmSTRING_DEFAULT_CHARSET);
    XtSetArg(infoLineArg[0], XmNlabelString, xs);
    XtSetValues(TopInfoLine, infoLineArg, 1);
    XmStringFree(xs);
#endif
    return;
}

#undef setTopInfoLine

#ifdef SWITCH_TOP_AND_BOTTOM
#define setBottomInfoLine setTopInfoLine
#endif

static void setBottomInfoLine _ARGUMENTS((char *));

static void setBottomInfoLine(message)  
    char *message;
{
    Arg infoLineArg[1];

#ifndef MOTIF
    XtSetArg(infoLineArg[0], XtNlabel, message);
    (void) strcpy(BottomNonButtonInfo, (char *) infoLineArg[0].value);
    XtSetValues(BottomInfoLine, infoLineArg, XtNumber(infoLineArg));
#else
    XmString xs;

    (void) strcpy(BottomNonButtonInfo, message);
    xs = XmStringCreate(message, XmSTRING_DEFAULT_CHARSET);
    XtSetArg(infoLineArg[0], XmNlabelString, xs);
    XtSetValues(BottomInfoLine, infoLineArg, 1);
    XmStringFree(xs);
#endif
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
#ifndef MOTIF
		    buttons[i] = XtCreateWidget((char *) buttonList[j].buttonArgs[0].value,
						  commandWidgetClass,
						  box,
						  buttonList[j].buttonArgs,
						  buttonList[j].size);
#else
		    if (app_resources.useGadgets) {
		      buttons[i] = XmCreatePushButtonGadget(box,
						      (char *) buttonList[j].buttonArgs[0].value,
						      buttonList[j].buttonArgs,
						      buttonList[j].size);
		    } else {
		      buttons[i] = XmCreatePushButton(box,
						      (char *) buttonList[j].buttonArgs[0].value,
						      buttonList[j].buttonArgs,
						      buttonList[j].size);
		    }
		  if (!app_resources.useGadgets)
#endif
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
#ifndef MOTIF	    
	    buttons[i] = XtCreateWidget((char *) buttonList[i].buttonArgs[0].value,
					  commandWidgetClass,
					  box,
					  buttonList[i].buttonArgs,
					  buttonList[i].size);
#else
	    if (app_resources.useGadgets) {
	      buttons[i] = XmCreatePushButtonGadget(box,
					      (char *) buttonList[i].buttonArgs[0].value,
					      buttonList[i].buttonArgs,
					      buttonList[i].size);
	    } else {
	      buttons[i] = XmCreatePushButton(box,
					      (char *) buttonList[i].buttonArgs[0].value,
					      buttonList[i].buttonArgs,
					      buttonList[i].size);
	    }
	  if (!app_resources.useGadgets)
#endif
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
    XtAppContext app = XtWidgetToApplicationContext(TopLevel);

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

    XtAppAddActions(app, TopActions, XtNumber(TopActions));
    
    AddButtons = ARRAYALLOC(Widget, XtNumber(AddButtonList));
    XtAppAddActions(app, AddActions, XtNumber(AddActions));

    doButtons(app_resources.addButtonList, TopButtonBox, AddButtons, AddButtonList, &AddButtonListCount, TOP);

    NgButtons = ARRAYALLOC(Widget, XtNumber(NgButtonList));
    XtAppAddActions(app, NgActions, XtNumber(NgActions));
    
    doButtons(app_resources.ngButtonList, TopButtonBox, NgButtons, NgButtonList, &NgButtonListCount, TOP);

    AllButtons = ARRAYALLOC(Widget, XtNumber(AllButtonList));
    XtAppAddActions(app, AllActions, XtNumber(AllActions));
    
    doButtons(app_resources.allButtonList, BottomButtonBox, AllButtons, AllButtonList, &AllButtonListCount, BOTTOM);
    
    ArtButtons = ARRAYALLOC(Widget, XtNumber(ArtButtonList));
    XtAppAddActions(app, ArtActions, XtNumber(ArtActions));
    
    doButtons(app_resources.artButtonList, TopButtonBox, ArtButtons, ArtButtonList, &ArtButtonListCount, TOP);
    
    ArtSpecButtons = ARRAYALLOC(Widget, XtNumber(ArtSpecButtonList));
    
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

static void swapMode()
/*
 * change the buttons displayed in the TopButtonBox (switch modes)
 */
{
    if (PreviousMode == Mode) {
	return;
    }

    XawTextDisableRedisplay(ArticleText);
#ifndef MOTIF
    XawPanedSetRefigureMode(Frame, False);
#else
    {
      Arg args[1];

      XtSetArg(args[0], XmNrefigureMode, False);
      XtSetValues(Frame, args, 1);
    }
#endif
    
    /*
     * NONE -> ADD
     *    manage add in top box
     *    manage art in bottom box
     *    desensitize bottom box
     *    install add actions in top box
     */
    if ((PreviousMode == NO_MODE) && (Mode == ADD_MODE)) {
	XtManageChildren(AddButtons, AddButtonListCount);
	XtManageChildren(ArtSpecButtons, ArtSpecButtonListCount);
	XtSetSensitive(BottomButtonBox, False);
	setTranslations(ADD_MODE);
    /*    
     * NONE -> NG
     *    manage ng in top box
     *    manage art in bottom box
     *    desensitize bottom box
     *    install ng actions in top box
     */
    } else if ((PreviousMode == NO_MODE) && (Mode == NEWSGROUP_MODE)) {
	XtManageChildren(NgButtons, NgButtonListCount);
	XtManageChildren(ArtSpecButtons, ArtSpecButtonListCount);
	XtSetSensitive(BottomButtonBox, False);
	setTranslations(NEWSGROUP_MODE);
    /*
     * ADD -> NG
     *    unmanage add in top box
     *    manage ng in top box
     *    install ng actions in top box
     */
    } else if ((PreviousMode == ADD_MODE) && (Mode == NEWSGROUP_MODE)) {
	XtUnmanageChildren(AddButtons, AddButtonListCount);
	XtManageChildren(NgButtons, NgButtonListCount);
	setTranslations(NEWSGROUP_MODE);
    /*
     * NG -> ART
     *    unmanage ng in top box
     *    manage art in top box
     *    sensitize bottom box
     *    install art actions in top box
     *    install art actions in bottom box
     */
    } else if ((PreviousMode == NEWSGROUP_MODE) && (Mode == ARTICLE_MODE)) {
	XtUnmanageChildren(NgButtons, NgButtonListCount);
	XtManageChildren(ArtButtons, ArtButtonListCount);
	XtSetSensitive(BottomButtonBox, True);
	setTranslations(ARTICLE_MODE);
    /*
     * NG -> ADD
     *    unmanage ng in top box
     *    manage add in top box
     *    install add actions in top box
     */
    } else if ((PreviousMode == NEWSGROUP_MODE) && (Mode == ADD_MODE)) {
	XtUnmanageChildren(NgButtons, NgButtonListCount);
	XtManageChildren(AddButtons, AddButtonListCount);
	setTranslations(ADD_MODE);
    /*
     * NG -> ALL
     *    desensitize top box
     *    unmanage ng in bottom box
     *    manage all in bottom box
     *    sensitize bottom box
     *    install all actions in bottom box
     */
    } else if ((PreviousMode == NEWSGROUP_MODE) && (Mode == ALL_MODE)) {
	XtSetSensitive(TopButtonBox, False);
	XtUnmanageChildren(ArtSpecButtons, ArtSpecButtonListCount);
	XtManageChildren(AllButtons, AllButtonListCount);
	XtSetSensitive(BottomButtonBox, True);
	setTranslations(ALL_MODE);
    /*     
     * ART -> NG
     *    desensitize bottom box
     *    unmanage art in top box
     *    manage ng in top box
     *    install ng actions in top box
     */
    } else if ((PreviousMode == ARTICLE_MODE) && (Mode == NEWSGROUP_MODE)) {
	XtSetSensitive(BottomButtonBox, False);
	XtUnmanageChildren(ArtButtons, ArtButtonListCount);
	XtManageChildren(NgButtons, NgButtonListCount);
	setTranslations(NEWSGROUP_MODE);
    /*
     * ALL -> NG
     *    sensitize top box
     *    unmanage all in bottom box
     *    manage art in bottom box
     *    desensitize bottom box
     */
    } else if ((PreviousMode == ALL_MODE) && (Mode == NEWSGROUP_MODE)) {
	XtSetSensitive(TopButtonBox, True);
	XtUnmanageChildren(AllButtons, AllButtonListCount);
	XtManageChildren(ArtSpecButtons, ArtSpecButtonListCount);
	XtSetSensitive(BottomButtonBox, False);
	setTranslations(NEWSGROUP_MODE);
    /*
     * ART -> ALL (going back to previous ALL_MODE)
     *    unmanage art in bottom box
     *    unmanage art in top box
     *    manage all in bottom box
     *    manage ng in top box
     *    desensitize top box
     *    install all actions in bottom box
     */
    } else if ((PreviousMode == ARTICLE_MODE) && (Mode == ALL_MODE)) {
	XtUnmanageChildren(ArtSpecButtons, ArtSpecButtonListCount);
	XtUnmanageChildren(ArtButtons, ArtButtonListCount);
	XtManageChildren(AllButtons, AllButtonListCount);
	XtManageChildren(NgButtons, NgButtonListCount);
	XtSetSensitive(TopButtonBox, False);
	setTranslations(ALL_MODE);
    /*	
     * ALL -> ART
     *    unmanage ng in top box
     *    sensitize top box
     *    manage art in top box
     *    unmanage all in bottom box
     *    manage art in bottom box
     *    install art actions in bottom box
     */
    } else if ((PreviousMode == ALL_MODE) && (Mode == ARTICLE_MODE)) {
	XtSetSensitive(TopButtonBox, True);
	XtUnmanageChildren(NgButtons, NgButtonListCount);
	XtManageChildren(ArtButtons, ArtButtonListCount);
	XtUnmanageChildren(AllButtons, AllButtonListCount);
	XtManageChildren(ArtSpecButtons, ArtSpecButtonListCount);
	setTranslations(ARTICLE_MODE);
    } else {
      (void) sprintf(error_buffer, ERROR_UNSUP_TRANS_MSG ,
			       PreviousMode, Mode);
	ehErrorExitXRN(error_buffer);
    }
#ifndef MOTIF
    XawPanedSetRefigureMode(Frame, True);
#else
    {
      Arg args[2];
      Dimension height;

/* Lock size of button boxes to their desired height */
      height = 0;
      if (XtIsManaged(AddButtons[0])) {
	height = DesiredBoxHeight(TopButtonBox, AddButtons,
				  AddButtonListCount);
      } else if (XtIsManaged(NgButtons[0])) {
	height = DesiredBoxHeight(TopButtonBox, NgButtons,
				  NgButtonListCount);
      } else if (XtIsManaged(ArtButtons[0])) {
	height = DesiredBoxHeight(TopButtonBox, ArtButtons,
				  ArtButtonListCount);
      }
      if (height) {
	XtSetArg(args[0], XmNpaneMinimum, height);
	XtSetArg(args[1], XmNpaneMaximum, height);
	XtSetValues(TopButtonBox, args, 2);
      }

      height = 0;
      if (XtIsManaged(AllButtons[0])) {
	height = DesiredBoxHeight(BottomButtonBox, AllButtons,
				  AllButtonListCount);
      } else if (XtIsManaged(ArtSpecButtons[0])) {
	height = DesiredBoxHeight(BottomButtonBox, ArtSpecButtons,
				  ArtSpecButtonListCount);
      }
      if (height) {
	XtSetArg(args[0], XmNpaneMinimum, height);
	XtSetArg(args[1], XmNpaneMaximum, height);
	XtSetValues(BottomButtonBox, args, 2);
      }

      XtSetArg(args[0], XmNrefigureMode, True);
      XtSetValues(Frame, args, 1);
#ifndef MOTIF_BUG	/* Motif paned widget bug */
      XtUnmanageChild(BottomButtonBox);
      XtManageChild(BottomButtonBox);
#endif
    }
#endif
    XawTextEnableRedisplay(ArticleText);
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

#ifndef MOTIF

static int lastPage _ARGUMENTS((Widget));

static int lastPage(w)
    Widget w;
{
    XawTextPosition top;

    top = XawTextTopPosition(w);

    if (top >= ArticleTextFilesize) {
	return 1;
    }
    return 0;
}

#else

/**********************************************************************
In Motif, scrolling the text when their is no more text to scroll
doesn't result in a detectable state like the Xaw text widget does.
Therefore, this is a different routine that should be used before
trying to scroll, and indicates whether scrolling is possible.  We do
this by examining the vertical scrollbar state of the scrolled window
which is the parent of the text widget.
**********************************************************************/

static Boolean onLastPage _ARGUMENTS((Widget));

static Boolean onLastPage(w)
    Widget w;
{
#ifdef OLD_VERSION
    Widget vs;
    int max, value, size, ignore;
    Arg args[2];

    XtSetArg(args[0], XmNverticalScrollBar, &vs);
    XtGetValues(XtParent(w), args, 1);
    XtSetArg(args[0], XmNmaximum, &max);
    XtGetValues(vs, args, 1);
    XmScrollBarGetValues(vs, &value, &size, &ignore, &ignore);
    return (value+size >= max);
#else
    Position ignore;
    return (XmTextPosToXY(w, XmTextGetLastPosition(w), &ignore, &ignore));
#endif
}
#endif


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
    switch (Mode) {
	case ALL_MODE:
	allScrollFunction(NULL, event, NULL, NULL);
	break;

	case NEWSGROUP_MODE:
	if (count && *count == 1 && strcmp(string[0], "jump") == 0) {
	    NewsgroupEntryMode = XRN_JUMP;
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
/* For Motif, check if scrolling is possible before doing it */
#ifndef MOTIF
	    artScrollFunction(0, event, 0, 0);
	    if (lastPage(ArticleText))
#else
	    if (onLastPage(ArticleText))
#endif
	    {
		if (app_resources.subjectRead == False) {
		    artNextUnreadFunction(NULL, NULL, NULL, NULL);
		} else {
		    artSubNextFunction(NULL, NULL, NULL, NULL);
		}
#ifdef MOTIF
	    } else {
	      artScrollFunction(NULL, event, NULL, NULL);
#endif
	    }
	}
	break;
    }
    unbusyCursor();
    inCommand = 0;
    return;
}

/*
 *  Redraw the text between left and right in the subject window;
 *  Adjust the window so the cursor is between min and max lines.
 */
static void updateSubjectWidget _ARGUMENTS((XawTextPosition, XawTextPosition));
    
static void updateSubjectWidget(left, right)
    XawTextPosition left, right;
{
    XawTextPosition currentPos;
#ifndef MOTIF
    Arg arg[1];
    Arg sargs[5];
#endif
    int numLines, count;

#ifndef MOTIF
    if (SubjectSource != 0)
#else
    if (TextMotifString != 0)
#endif
    {
	currentPos = ArticleTop = XawTextTopPosition(Text);
	XawTextInvalidate(Text, left - 1, right + 1);
	if ((app_resources.minLines >= 0) && (app_resources.maxLines >= 0)) {
	    if (currentPos <= ArtPosition) {
		for (numLines = 1; currentPos < ArtPosition; numLines++) {
		    if (!moveCursor(FORWARD, SubjectString, &currentPos)) {
			break;
		    }
		}
	    } else {
		numLines = -1;
		currentPos = ArtPosition;
	    }
	    if (numLines > app_resources.maxLines
			|| numLines < app_resources.minLines) {
		for (count = 1; count < app_resources.defaultLines; count++) {
		    if (!moveCursor(BACK, SubjectString, &currentPos)) {
			break;
		    }
		}
#ifndef MOTIF
		XtSetArg(arg[0], XtNdisplayPosition, currentPos);
		XtSetValues(Text, arg, XtNumber(arg));
#else
		XmListSetPos(Text,
			     XawTextToMotifIndex(TextMotifString, currentPos));
#endif
	    }
#ifdef notdef
	      else if (numLines < app_resources.minLines) {
		for (count = 1; count < app_resources.maxLines; count++) {
		    if (!moveCursor(BACK, SubjectString, &currentPos)) {
			break;
		    }
		}
#ifndef MOTIF
		XtSetArg(arg[0], XtNdisplayPosition, currentPos);
		XtSetValues(Text, arg, XtNumber(arg));
#else
		XmListSetPos(Text,
			     XawTextToMotifIndex(TextMotifString, currentPos));
#endif
	    }
#endif /* notdef */
	}
    } else {
#ifndef MOTIF
	ArticleTop = (XawTextPosition) 0;
	XtSetArg(sargs[0], XtNstring, SubjectString);
	XtSetArg(sargs[1], XtNlength, utStrlen(SubjectString) + 1);
	XtSetArg(sargs[2], XtNeditType, XawtextRead);
	XtSetArg(sargs[3], XtNuseStringInPlace, True);
	XtSetArg(sargs[4], XtNtype, XawAsciiString);
	SubjectSource = XtCreateWidget("subjectTextSource",
				       asciiSrcObjectClass,
				       Text, sargs, XtNumber(sargs));
	XawTextSetSource(Text, SubjectSource, (XawTextPosition) ArticleTop);
#else
	XawTextSetMotifString(Text, SubjectString);
#endif
    }
    XawTextSetInsertionPoint(Text, ArtPosition);
    return;
}


/*
 * Get the nearest article to the cursor.  If there is no article on the
 * current line, search forward or backwards for a valid article, depending
 * on the value of status.  Return the filename and question of the
 * article obtained.
 */
static int getNearbyArticle _ARGUMENTS((int, char **, char **, long *));

static int getNearbyArticle(status, filename, question, artNum)
    int status;
    char **filename, **question;
    long *artNum;
{
    XawTextPosition beginning;
    char *mesg_name = "getNearbyArticle";

    if (status == art_PREV) {
	if (SubjectString[ArtPosition] == '\0') {
	    if (ArtPosition == 0) {
		/* no articles remain, jump out of article mode */
		return art_DONE;
	    }
	    if (!moveCursor(BACK, SubjectString, &ArtPosition)) {
		return art_DONE;
	    }
	}
	*artNum = atol(&SubjectString[ArtPosition+2]);
	gotoArticle(*artNum);
	while (getArticle(filename, question) != XRN_OKAY) {
	    mesgPane(XRN_SERIOUS, mesg_name, ART_NOT_AVAIL_MSG, *artNum);
#ifndef MOTIF
	    removeLine(SubjectString, Text, &SubjectSource, ArtPosition,
		       &ArticleTop);
#else
	    removeLine(SubjectString, Text, (Widget *)&TextMotifString, ArtPosition,
		       &ArticleTop);
#endif
	    if (!moveCursor(BACK, SubjectString, &ArtPosition)) {
		return art_DONE;
	    }
	    *artNum = atol(&SubjectString[ArtPosition+2]);
	    gotoArticle(*artNum);
	}
	return art_CHANGE;
    }

    if (status == art_NEXT) {
	if (SubjectString[ArtPosition] == '\0') {
	    return art_DONE;
	}
	*artNum = atol(&SubjectString[ArtPosition+2]);
	gotoArticle(*artNum);
	while (getArticle(filename, question) != XRN_OKAY) {
	    mesgPane(XRN_SERIOUS, mesg_name, ART_NOT_AVAIL_MSG, *artNum);
#ifndef MOTIF
	    removeLine(SubjectString, Text, &SubjectSource, ArtPosition,
		       &ArticleTop);
#else
	    removeLine(SubjectString, Text, (Widget *)&TextMotifString, ArtPosition,
		       &ArticleTop);
#endif
	    if (SubjectString[ArtPosition] == '\0') {
		return art_DONE;
	    }
	    *artNum = atol(&SubjectString[ArtPosition+2]);
	    gotoArticle(*artNum); 
	}
	return art_CHANGE;
    }
    if (status == art_NEXTWRAP) {
	if (SubjectString[ArtPosition] == '\0') {
	    if (ArtPosition == 0) {
		return art_DONE;
	    }
	    ArtPosition = 0;
	}
	*artNum = atol(&SubjectString[ArtPosition+2]);
	gotoArticle(*artNum);
	while (getArticle(filename, question) != XRN_OKAY) {
	    mesgPane(XRN_SERIOUS, mesg_name, ART_NOT_AVAIL_MSG, *artNum);
#ifndef MOTIF
	    removeLine(SubjectString, Text, &SubjectSource, ArtPosition,
		       &ArticleTop);
#else
	    removeLine(SubjectString, Text, (Widget *)&TextMotifString, ArtPosition,
		       &ArticleTop);
#endif
	    if (SubjectString[ArtPosition] == '\0') {
		if (ArtPosition == 0) {
		    return art_DONE;
		}
		ArtPosition = 0;
	    }
	    *artNum = atol(&SubjectString[ArtPosition+2]);
	    gotoArticle(*artNum); 
	}
	return art_CHANGE;
    }
    if (status == art_UNREAD) {
	if (SubjectString[ArtPosition] == '\0') {
	    if (ArtPosition == 0) {
		return art_DONE;
	    }
	    ArtPosition = 0;
	}
	beginning = ArtPosition;
	if (SubjectString[ArtPosition] != ' ') {
	    (void) moveUpWrap(SubjectString, &ArtPosition);
	    while ((SubjectString[ArtPosition] != ' ') &&
		   (ArtPosition != beginning)) {
		if (!moveUpWrap(SubjectString, &ArtPosition)) {
		    return art_DONE;
		}
	    }
	    if (ArtPosition == beginning) {
		return art_DONE;
	    }
	}
	/* we are at an unread article */
	*artNum = atol(&SubjectString[ArtPosition+2]);
	gotoArticle(*artNum);
	while (getArticle(filename, question) != XRN_OKAY) {
	    mesgPane(XRN_SERIOUS, mesg_name, ART_NOT_AVAIL_MSG, *artNum);
#ifndef MOTIF
	    removeLine(SubjectString, Text, &SubjectSource, ArtPosition,
		       &ArticleTop);
#else
	    removeLine(SubjectString, Text, (Widget *)&TextMotifString, ArtPosition,
		       &ArticleTop);
#endif
	    if (SubjectString[ArtPosition] == '\0') {
		if (ArtPosition == 0) {
		    return art_DONE;
		}
		ArtPosition = 0;
	    }
	    while ((SubjectString[ArtPosition] != ' ') &&
		   (ArtPosition != beginning)) {
		if (!moveUpWrap(SubjectString, &ArtPosition)) {
		    return art_DONE;
		}
	    }
	    if (ArtPosition == beginning) {
		return art_DONE;
	    }
	    *artNum = atol(&SubjectString[ArtPosition+2]);
	    gotoArticle(*artNum); 
	}
	return art_CHANGE;
    }
    return art_CHANGE;
}

#define CHANGE 0		/* subject window has changed */
#define NOCHANGE 1		/* subject window has not changed */
#define DONE 2			/* no new article was found */
				/* EXIT is already defined, it implies */
				/* there are no articles left at all */

/*
 *
 */
static int isPrevSubject _ARGUMENTS((char *, char **, char **, long *));

static int isPrevSubject(subject, filename, question, artNum)
    char *subject;
    char **filename, **question;
    long *artNum;
{
    char *newsubject;
    char *newLine;
    char *newSubjectString;
    char *newString;
    char *oldString;
    XawTextPosition save;
    int count = 0;

    oldString = NIL(char);
    save = ArtPosition;
    startSearch();
    abortClear();
    
    for (;;) {
	count++;

	if (count == app_resources.cancelCount) {
	    cancelCreate();
	}

	if (abortP()) {
	    failedSearch();
	    ArtPosition = save;
	    cancelDestroy();
	    return ABORT;
	}
	if (SubjectString[ArtPosition] == '\0') {
	    cancelDestroy();
	    return EXIT;
	}
	if (ArtPosition != 0) {
	    (void) moveCursor(BACK, SubjectString, &ArtPosition);
	    *artNum = atol(&SubjectString[ArtPosition+2]);
	    newsubject = getSubject(*artNum);
	    if (utSubjectCompare(newsubject, subject) == 0) {
		gotoArticle(*artNum);
		if (getArticle(filename, question) != XRN_OKAY) {
		    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
#ifndef MOTIF
		    removeLine(SubjectString, Text, &SubjectSource, ArtPosition,
			       &ArticleTop);
#else
		    removeLine(SubjectString, Text, (Widget *)&TextMotifString, ArtPosition,
			       &ArticleTop);
#endif
		    continue;
		}
		if (SubjectString[ArtPosition] == 'u') {
		    markArticleAsUnread(*artNum);
		}
		cancelDestroy();
		return NOCHANGE;
	    }
	    continue;
	} else {
	    if ((newLine = getPrevSubject()) == NIL(char)) {
		failedSearch();
		ArtPosition = save;
		cancelDestroy();
		return DONE;
	    }
	    newLine[0] = '+';
	    *artNum = atol(&newLine[2]);
	    newsubject = getSubject(*artNum);
	    if (oldString != NIL(char)) {
		newString = ARRAYALLOC(char, (utStrlen(oldString) + utStrlen(newLine) + 2));
		(void) strcpy(newString, newLine);
		(void) strcat(newString, "\n");
		(void) strcat(newString, oldString);
		FREE(oldString);
	    } else {
		newString = ARRAYALLOC(char, (utStrlen(newLine) + 2));
		(void) strcpy(newString, newLine);
		(void) strcat(newString, "\n");
	    }
	    if (utSubjectCompare(newsubject, subject) == 0) {
		/* found a match, go with it */

		newSubjectString = ARRAYALLOC(char, (utStrlen(newString) + utStrlen(SubjectString) + 1));
		(void) strcpy(newSubjectString, newString);
		(void) strcat(newSubjectString, SubjectString);
		FREE(SubjectString);
		SubjectString = newSubjectString;

		gotoArticle(*artNum);
		if (getArticle(filename, question) != XRN_OKAY) {
		    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
		}
		if (SubjectString[ArtPosition] == 'u') {
		    markArticleAsUnread(*artNum);
		}
		cancelDestroy();
		return CHANGE;
	    }
	    oldString = newString;
	    continue;
	}
    }
}

/*
 *
 */
static int isNextSubject _ARGUMENTS((char *, char **, char **, long *));

static int isNextSubject(subject, filename, question, artNum)
    char *subject;
    char **filename, **question;
    long *artNum;
{
    char *newsubject;
    XawTextPosition save = ArtPosition;
    int count = 0;

    abortClear();
    
    for (;;) {
	count++;

	if (count == app_resources.cancelCount) {
	    cancelCreate();
	}

	if (count >= app_resources.cancelCount && count % 10 == 0 && abortP()) {
	    failedSearch();
	    ArtPosition = save;
	    cancelDestroy();
	    return ABORT;
	}
	if (SubjectString[ArtPosition] == '\0') {
	    cancelDestroy();
	    if (ArtPosition == 0) {
		return EXIT;
	    }
	    ArtPosition = save;
	    return DONE;
	}
	*artNum = atol(&SubjectString[ArtPosition+2]);
	newsubject = getSubject(*artNum);
	if (utSubjectCompare(newsubject, subject) == 0) {
	    gotoArticle(*artNum);
	    if (getArticle(filename, question) != XRN_OKAY) {
		mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
#ifndef MOTIF
		removeLine(SubjectString, Text, &SubjectSource, ArtPosition,
			   &ArticleTop);
#else
		removeLine(SubjectString, Text, (Widget *)&TextMotifString, ArtPosition,
			   &ArticleTop);
#endif
		continue;
	    }
	    cancelDestroy();
	    return NOCHANGE;
	} else {
	    if (!moveCursor(FORWARD, SubjectString, &ArtPosition)) {
		cancelDestroy();
		return EXIT;
	    }
	}
    }
}

/*
 *
 */
static int getPrevious _ARGUMENTS((XawTextPosition *));

static int getPrevious(artNum)
    XawTextPosition *artNum;
{
    char *newLine;
    char *newString;
#ifndef MOTIF
    Arg sargs[5];
#endif

    if ((newLine = getPrevSubject()) != NIL(char)) {
	newString = ARRAYALLOC(char, (utStrlen(SubjectString) + utStrlen(newLine) + 2));
	(void) strcpy(newString, newLine);
	(void) strcat(newString, "\n");
	(void) strcat(newString, SubjectString);
	FREE(SubjectString);
	SubjectString = newString;
	ArtPosition = ArticleTop = 0;
#ifndef MOTIF
	if (SubjectSource != 0) {
	    XawStringSourceDestroy(SubjectSource);
	}

	XtSetArg(sargs[0], XtNstring, SubjectString);
	XtSetArg(sargs[1], XtNlength, utStrlen(SubjectString) + 1);
	XtSetArg(sargs[2], XtNeditType, XawtextRead);
	XtSetArg(sargs[3], XtNuseStringInPlace, True);
	XtSetArg(sargs[4], XtNtype, XawAsciiString);
	SubjectSource = XtCreateWidget("subjectTextSource",
				       asciiSrcObjectClass,
				       Text, sargs, XtNumber(sargs));
	XawTextSetSource(Text, SubjectSource, ArticleTop);
#else
	XawTextSetMotifString(Text, SubjectString);
#endif
	XawTextSetInsertionPoint(Text, ArtPosition);
	*artNum = atol(&SubjectString[ArtPosition+2]);
	return TRUE;
    }
    
    return FALSE;
}
    
/*
 */
static void selectedArticle _ARGUMENTS((int));

static void selectedArticle(status)
    int status;
{
    XawTextPosition left, right;
    
    XawTextGetSelectionPos(Text, &left, &right);
    XawTextUnsetSelection(Text);
    
    /* nothing was selected if left == right, so get article on NEXT or */
    /* PREVIOUS line */

    if (left == right) {
	ArtPosition = XawTextGetInsertionPoint(Text);

	if (ArtEntry == 1) {
	    return;
	}

	/* get article on NEXT line */

	if ((status == art_NEXT) || (status == art_UNREAD)) {
	    (void) moveCursor(FORWARD, SubjectString, &ArtPosition);
	} else if (status == art_PREV) {
	    (void) moveCursor(BACK, SubjectString, &ArtPosition);
	}
	return;
    }

    /* something was selected */

    /* make sure selection includes only whole groups */
    moveBeginning(SubjectString, &left);
    ArtPosition = left;
    return;
}


/*
 * Adjust the top position in the newsgroup window such
 * that the cursor stays between min and max lines.
 */
static void adjustNewsgroupWidget()
{
    int numLines, count;
    XawTextPosition currentPos;

    moveBeginning(NewsGroupsString, &GroupPosition);
    currentPos = NewsgroupTop;
#ifndef MOTIF
    if ((app_resources.minLines >= 0) && (app_resources.maxLines >= 0))
#else
/* Make sure that the number of list items is greater than the min and max
   constraints before adjusting the list.  Otherwise, we get really weird
   scrolling effects.  This is actually a potential bug with Xaw, as well. */
      {
	Arg args[1];
	int v;

	XtSetArg(args[0], XmNvisibleItemCount, &v);
	XtGetValues(Text, args, 1);
	count = v;
      }
    if (app_resources.minLines >= 0 && app_resources.minLines < count &&
	app_resources.maxLines >= 0 && app_resources.maxLines < count)
#endif
    {
	if (currentPos <= GroupPosition) {
	    for (numLines = 1; currentPos < GroupPosition; numLines++) {
		if (!moveCursor(FORWARD, NewsGroupsString, &currentPos)) {
		    break;
		}
	    }
	} else {
	    numLines = -1;
	    currentPos = GroupPosition;
	}
	if (numLines > app_resources.maxLines
		|| numLines < app_resources.minLines) {
	    for (count = 1; count < app_resources.defaultLines; count++) {
		if (!moveCursor(BACK, NewsGroupsString, &currentPos)) {
		    break;
		}
	    }
	    NewsgroupTop = currentPos;
	}
    }
    return;
}

/*
 * Redraw the all groups window, assuming it has changed
 */
static void redrawAllWidget _ARGUMENTS((XawTextPosition));

static void redrawAllWidget(position)
    XawTextPosition position;
{
#ifndef MOTIF
    Arg sargs[5];
#endif

    if (Mode != ALL_MODE) {
	return;
    }

#ifndef MOTIF
    /* free source */
    if (AllSource != 0) {
	XawStringSourceDestroy(AllSource);
	AllSource = 0;
    }
#else
    ArticleTextMotifString = 0;
#endif
    
    /* free string */
    if (AllGroupsString != NIL(char)) {
	FREE(AllGroupsString);
	AllGroupsString = NIL(char);
    }
    AllGroupsString = getStatusString(AllStatus);

	
#ifndef MOTIF
    XtSetArg(sargs[0], XtNstring, AllGroupsString);
    XtSetArg(sargs[1], XtNlength, utStrlen(AllGroupsString) + 1);
    XtSetArg(sargs[2], XtNeditType, XawtextRead);
    XtSetArg(sargs[3], XtNuseStringInPlace, True);
    XtSetArg(sargs[4], XtNtype, XawAsciiString);
    AllSource = XtCreateWidget("allTextSource",
				   asciiSrcObjectClass,
				   ArticleText, sargs, XtNumber(sargs));
    XawTextSetSource(ArticleText, AllSource, position);
    XtSetValues(ArticleText, lineSelArgs, XtNumber(lineSelArgs));
#else
    XawTextSetMotifString(ArticleText, AllGroupsString);
#endif
    
    return;
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

/*
 * Rebuild the newsgroup text window.
 * Find out what groups have articles to read and build up the string.
 * Create a string source and display it.
 */
static void redrawNewsgroupTextWidget()
{
#ifndef MOTIF
    static Widget NgSource = 0;
    Arg sargs[5];
#endif
    char name[GROUP_NAME_SIZE];

    if (Mode != NEWSGROUP_MODE) {
	return;
    }

    if (NewsGroupsString != NIL(char)) {
	FREE(NewsGroupsString);
    }
    
    NewsGroupsString = unreadGroups(NewsgroupDisplayMode);

    /* update the info line */
    if (utStrlen(NewsGroupsString) == 0) {
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
#ifndef MOTIF
    if (NgSource != 0) {
	XawStringSourceDestroy(NgSource);
    }
#endif
    /* used to be NewsGroupsString[GroupPosition] - bug reported by Bob E. */
    if (NewsGroupsString[0] == '\0') {
	GroupPosition = 0;
    }
    currentGroup(Mode, NewsGroupsString, name, GroupPosition);
    if (STREQ(name, LastGroup)) {
	(void) moveUpWrap(NewsGroupsString, &GroupPosition);
    }
    
    adjustNewsgroupWidget();
    
#ifndef MOTIF
    XtSetArg(sargs[0], XtNstring, NewsGroupsString);
    XtSetArg(sargs[1], XtNlength, utStrlen(NewsGroupsString) + 1);
    XtSetArg(sargs[2], XtNeditType, XawtextRead);
    XtSetArg(sargs[3], XtNuseStringInPlace, True);
    XtSetArg(sargs[4], XtNtype, XawAsciiString);
    NgSource = XtCreateWidget("allTextSource",
				   asciiSrcObjectClass,
				   Text, sargs, XtNumber(sargs));
    XawTextSetSource(Text, NgSource, (XawTextPosition) NewsgroupTop);
#else
    if (NewsGroupsString[0] == '\0' && !XtIsRealized(TopLevel)) {
	XawTextSetMotifString(Text, " ");
    }
    XawTextSetMotifString(Text, NewsGroupsString);
#endif
    
    return;
}

/*
 * update the info line and update the newsgroup text window
 */
static void updateNewsgroupMode()
{
    if (PreviousMode != NEWSGROUP_MODE) {
	setTopInfoLine(OPEARATION_APPLY_CURSOR_MSG);
    }
    redrawNewsgroupTextWidget();
    XawTextSetInsertionPoint(Text, GroupPosition);
    return;
}


/*
 * install the newsgroup mode buttons (and the delete the previous mode buttons)
 * and then go to newsgroup mode
 */
static void switchToNewsgroupMode()
{
    PreviousMode = Mode;
    Mode = NEWSGROUP_MODE;
    LastRegexp = NIL(char);

    resetSelection();

    /* switch buttons */
    swapMode();
    
    /* update the newsgroup mode windows */
    updateNewsgroupMode();
    
    return;
}

/*
 * install the article mode buttons (and delete the previous mode buttons),
 * build the subject line screen, and call ARTICLE_MODE function 'next unread'
 */
static int switchToArticleMode()
{
    int oldMode;
    char *NewSubjectString;
    
    NewSubjectString = getSubjects(UNREAD);

    if (! (NewSubjectString && *NewSubjectString)) {
	bogusNewsgroup();
	/*
	 * the sources and strings have been destroyed at this point
	 * have to recreate them - the redraw routines check the mode
	 * so we can call all of them and only the one that is for the
	 * current mode will do something
	 */
	redrawAllWidget((XawTextPosition) 0);
	redrawNewsgroupTextWidget();
	if (NewSubjectString != NIL(char)) {
	    FREE(NewSubjectString);
	}
	return BAD_GROUP;
    }

#ifndef MOTIF
    /* change the text window */
    /* XawTextSetLastPos(Text, (XawTextPosition) 0); */

    if (SubjectSource != 0) {
	XawStringSourceDestroy(SubjectSource);
	SubjectSource = 0;
    }
#else
    TextMotifString = 0;
#endif

    if (SubjectString != NIL(char)) {
	FREE(SubjectString);
    }
    SubjectString = NewSubjectString;
    
    /* get rid of previous groups save file string */
    if (SaveString && app_resources.resetSave) {
	if (SaveString != app_resources.saveString) {
		XtFree(SaveString);
	}
	SaveString = XtNewString(app_resources.saveString);
    }	

    oldMode = PreviousMode;
    
    PreviousMode = Mode;
    Mode = ARTICLE_MODE;

    /* switch buttons */
    swapMode();

    /*
     * "What's the purpose of this?"  you're saying.  "It looks wrong."
     * Well, try taking out any mention of oldMode in this function,
     * then recompile and do the following:
     * 1. Enter article mode
     * 2. Clock "Next Newsgroup".
     * 3. Click "Quit".
     * You will be returned to all mode instead of article mode.
     * Therefore, we've got to keep track in this function of whether
     * we're switching from article mode to article mode, and if so,
     * set PreviousMode truly in order for swapMode to work, but once
     * that's done, we can put it back to what it was before).
     */
    if (PreviousMode == ARTICLE_MODE) {
	PreviousMode = oldMode;
    }
    setTopInfoLine(OPEARATION_APPLY_CURSOR_MSG);

    /* get and display the article */
    ArtPosition = 0;
    updateSubjectWidget(0, 0);
    XawTextSetInsertionPoint(Text, 0);	/* source isn't around... */
#ifndef MOTIF
    XawTextSetSelectionArray(ArticleText, allSelectArray);
#endif /* MOTIF */
    ArtEntry = 1;
    artNextUnreadFunction(NULL, NULL, NULL, NULL);

    return GOOD_GROUP;
}

/*
 *
 */
static void updateAllWidget _ARGUMENTS((XawTextPosition, XawTextPosition));

static void updateAllWidget(left, right)
    XawTextPosition left, right;
{
    XawTextPosition current;
#ifndef MOTIF
    Arg sargs[5];
#endif

#ifndef MOTIF
    if (AllSource != 0)
#else
    if (ArticleTextMotifString != 0)
#endif
    {
	XawTextInvalidate(ArticleText, left - 1, right + 1);
	current = right+1;
	(void) setCursorCurrent(AllGroupsString, &current);
	XawTextSetInsertionPoint(ArticleText, current);
    } else {
#ifndef MOTIF
	XtSetArg(sargs[0], XtNstring, AllGroupsString);
	XtSetArg(sargs[1], XtNlength, utStrlen(AllGroupsString) + 1);
	XtSetArg(sargs[2], XtNeditType, XawtextRead);
	XtSetArg(sargs[3], XtNuseStringInPlace, True);
	XtSetArg(sargs[4], XtNtype, XawAsciiString);
	AllSource = XtCreateWidget("allTextSource",
				   asciiSrcObjectClass,
				   ArticleText, sargs, XtNumber(sargs));
	XawTextSetSource(ArticleText, AllSource, 0);
#else
	XawTextSetMotifString(ArticleText, AllGroupsString);
#endif
    }
    
    return;
}

/*
 * If the article to be displayed has changed, update the article
 * window and redraw the mode line
 */
/*
 * In order to avoid bug(?) in AsciiSrc (It will only notice if the *value*
 * of XtNstring (i.e. the address) has changed when XtNtype = XawAsciiFile),
 * we will do it "the Motif way" and read in the file ourselves...
 */

#ifndef MOTIF
static char *ArticleString = (char *) 0;
#endif

static void redrawArticleWidget _ARGUMENTS((char *, char *));

static void redrawArticleWidget(filename, question)
    char *filename, *question;
{
#ifndef MOTIF
    Arg args[5];
#endif
    int fildes;
    struct stat buf;
    char *data;
#if !defined(MOTIF) && defined(HILITE_SUBJECT)
    XawTextPosition pos;
    XawTextBlock block;
#endif

    if (filename != LastArticle) {
	LastArticle = filename;
	if ((fildes = open(filename, O_RDONLY)) != -1) {
	    fstat(fildes, &buf);
	    data = XtMalloc((size_t) buf.st_size+1);
	    read(fildes, data, (unsigned) buf.st_size);
	    data[buf.st_size] = '\0';
	    close(fildes);
#ifndef MOTIF
	    XtSetArg(args[0], XtNtype, XawAsciiString);
	    XtSetArg(args[1], XtNstring, data);
	    XtSetArg(args[2], XtNlength, buf.st_size+1);
	    XtSetArg(args[3], XtNuseStringInPlace, True);
	    XtSetArg(args[4], XtNeditType, XawtextRead);
#ifdef TEXT_WIDGET_WORKS_CORRECTLY
	    if (ArtSource == 0) {
		ArtSource = XtCreateWidget("artTextSource",
					   asciiSrcObjectClass,
					   ArticleText, args, XtNumber(args));
		XawTextSetSource(ArticleText, ArtSource, (XawTextPosition) 0);
		XawTextSetSelectionArray(ArticleText, allSelectArray);
		ArticleString = data;
	    } else {
		if (ArticleString) {
		    XtFree(ArticleString);
		    ArticleString = 0;
		}
		ArticleString = data;
		XtSetValues(ArtSource, args, XtNumber(args));
	    }
#else
	    /* destroy the old text file window */
	    if (ArtSource != 0) {
		if (ArticleString) {
		    XtFree(ArticleString);
		    ArticleString = 0;
		}
		XtDestroyWidget(ArtSource);
	    }

	    ArtSource = XtCreateWidget("artTextSource",
				       asciiSrcObjectClass,
				       ArticleText, args, XtNumber(args));
	    XawTextSetSource(ArticleText, ArtSource, (XawTextPosition) 0);
	    XawTextSetSelectionArray(ArticleText, allSelectArray);

#endif

#if defined(HILITE_SUBJECT)
	    /* EXPERIMENTAL: highlight the `Subject' field */
	    if (app_resources.highlightSubjects) {
		block.firstPos = 0;
		block.ptr = "Subject: ";
		block.length = utStrlen(block.ptr);
		block.format = XawFmt8Bit;
		pos = XawTextSearch(ArticleText, XawsdRight, &block);
		if (pos != 0) {
		    /* Reset all fields in block because I don't which */
		    /* fields XawTextSearch tampers with.	       */
		    block.firstPos = 0;
		    block.ptr = "\nSubject: ";
		    block.length = utStrlen(block.ptr);
		    block.format = XawFmt8Bit;
		    pos = XawTextSearch(ArticleText, XawsdRight, &block);
		    if (pos >= 0) {
			pos++;
		    }
		}
		if (pos >= 0) {
		    XawTextPosition pos2;
		    pos2 = XawTextSourceScan(ArtSource, pos, XawstEOL,
					     XawsdRight, 1, 0);
		    if (pos2 > pos) {
			XawTextSetSelection(ArticleText, pos, pos2);
		    }
		}
	    }
#endif
#else
	    ChooseText(True);
	    XmTextSetString(ArticleText, data);
	    XtFree(data);
#endif
	    ArticleTextFilesize = buf.st_size;
	}

	setBottomInfoLine(question);
#ifdef XRN_PREFETCH
	/* force the screen to update before prefetching */
	xthHandlePendingExposeEvents();
	
	prefetchNextArticle();
	
#endif /* XRN_PREFETCH */
    }
    return;
}

static void switchToAllMode();

/*
 * release the storage associated with article mode, unlink the article files,
 * and go to newsgroup mode
 */
static void exitArticleMode()
{
#if !defined(MOTIF) && defined(TEXT_WIDGET_WORKS_CORRECTLY)
    char *data;
#endif

    LastArticle = NIL(char);
    PrevArticle = CurrentArticle = 0;
    
#ifndef MOTIF
    /* release storage and unlink files */
    if (SubjectSource != 0) {
	XawStringSourceDestroy(SubjectSource);
	SubjectSource = 0;
    }

    /* dummy source - a placeholder until the new sources are installed */
    if (DummySource == 0) {
	static Arg sargs[] = {
	    {XtNstring, (XtArgVal) ""},
	    {XtNlength, (XtArgVal) 2},
	    {XtNeditType, (XtArgVal) XawtextRead},
	    {XtNuseStringInPlace, (XtArgVal) True},
	    {XtNtype, (XtArgVal) XawAsciiString},
	};
	DummySource = XtCreateWidget("dummyTextSource",
				   asciiSrcObjectClass,
				   ArticleText, sargs, XtNumber(sargs));
    }
    XawTextSetSource(Text, DummySource, (XawTextPosition) 0);
#ifdef TEXT_WIDGET_WORKS_CORRECTLY
    {
	Arg args[5];
	if (ArticleString) {
	    XtFree(ArticleString);
	    ArticleString = 0;
	}
	
	/* it turns out that this null string is NEVER freed --
	 * orphaning 1 byte each time we quit a group.  Not enough
	 * to worry about...
	 */
	ArticleString = data = XtNewString("");
	XtSetArg(args[0], XtNtype, XawAsciiString);
	XtSetArg(args[1], XtNstring, data);
	XtSetArg(args[2], XtNlength, 1);
	XtSetArg(args[3], XtNuseStringInPlace, True);
	XtSetArg(args[4], XtNeditType, XawtextRead);
	XtSetValues(ArtSource, args, XtNumber(args));
    }
#else
    /* clear the article window */
    if (ArtSource != 0) {
	if (ArticleString) {
	    XtFree(ArticleString);
	    ArticleString = 0;
	}
	XtDestroyWidget(ArtSource);
	ArtSource = 0;
    }

    XawTextSetSource(ArticleText, DummySource, (XawTextPosition) 0);
#endif /* TEXT_WIDGET_WORKS_CORRECTLY */
#else /* MOTIF */
    XawTextSetMotifString(Text, NULL);
    XawTextSetMotifString(ArticleText, NULL);
#endif
    setBottomInfoLine("");
    if (SubjectString != NIL(char)) {
	FREE(SubjectString);
	SubjectString = NIL(char);
    }
    
    releaseNewsgroupResources(CurrentGroup);
    if (app_resources.updateNewsrc == TRUE) {
	while (!updatenewsrc())
          ehErrorRetryXRN(ERROR_CANT_UPDATE_NEWSRC_MSG, True);
    }

    if (PreviousMode == NEWSGROUP_MODE) {
	switchToNewsgroupMode();
    } else {
	switchToAllMode();
    }
    
    return;
}

/*
 * Exit article mode if app_resources.stayInArticleMode is false;
 * otherwise, try to go to the next group.
 */
static void maybeExitArticleMode()
{
    if (app_resources.stayInArticleMode)
	artNextGroupFunction(0, 0, 0, 0);
    else
	exitArticleMode();
}

#ifndef MOTIF
/* source for the add mode text window */
static Widget AddSource = 0;
#endif

/*
 * release storage associated with add mode and go to newsgroup mode
 */
static void exitAddMode()
{
#ifndef MOTIF
    if (AddSource != 0) {
	XawStringSourceDestroy(AddSource);
	AddSource = 0;
    }
#else
    TextMotifString = 0;
#endif

    if (AddGroupsString != NIL(char)) {
	FREE(AddGroupsString);
	AddGroupsString = NIL(char);
    }

    switchToNewsgroupMode();
    
    return;
}

/*
 * update the add mode text window to correspond to the new set of groups
 */
static void redrawAddTextWidget _ARGUMENTS((XawTextPosition));

static void redrawAddTextWidget(insertPoint)
    XawTextPosition insertPoint;
{
#ifndef MOTIF
    Arg sargs[5];
#endif
    int unread = 0;
    int left, right, nbytes;
    char newGroup[GROUP_NAME_SIZE];

#ifndef MOTIF
    if (AddSource != 0) {
	XawStringSourceDestroy(AddSource);
    }
#endif
    (void) setCursorCurrent(AddGroupsString, &insertPoint);

#ifndef MOTIF
    XtSetArg(sargs[0], XtNstring, AddGroupsString);
    XtSetArg(sargs[1], XtNlength, utStrlen(AddGroupsString) + 1);
    XtSetArg(sargs[2], XtNeditType, XawtextRead);
    XtSetArg(sargs[3], XtNuseStringInPlace, True);
    XtSetArg(sargs[4], XtNtype, XawAsciiString);
    AddSource = XtCreateWidget("allTextSource",
				   asciiSrcObjectClass,
				   Text, sargs, XtNumber(sargs));
    
    XawTextSetSource(Text, AddSource, (XawTextPosition) insertPoint);
#else
    XawTextSetMotifString(Text, AddGroupsString);
#endif
    XawTextSetInsertionPoint(Text, insertPoint);

    left = 0;

    while (AddGroupsString[left] != '\0') {
	for (right = left; AddGroupsString[right] != '\n'; right++);
	nbytes = right - left;
	(void) strncpy(newGroup, &AddGroupsString[left], nbytes);
	newGroup[nbytes] = '\0';
	if (watchingGroup(newGroup)) {
	    unread++;
	    break;
	}
	left = right + 1;
    }
    if (! unread)
	unread = unreadNews();
    if (unread)
	xmSetIconAndName(UnreadIcon);
    else
	xmSetIconAndName(ReadIcon);

    return;
}

/*
 * Display new article, mark as read.
 */
static void foundArticle _ARGUMENTS((char *, char *, long));

static void foundArticle(file, ques, artNum)
    char *file, *ques;
    long artNum;
{
    PrevArticle = CurrentArticle;
    if (SubjectString[ArtPosition] == 'u') {
	markArticleAsUnread(artNum);
	CurrentArticle = artNum;
    } else {
	CurrentArticle = markStringRead(SubjectString, ArtPosition);
    }
    updateSubjectWidget(ArtPosition, ArtPosition);
    XawTextSetInsertionPoint(Text, ArtPosition);

    redrawArticleWidget(file, ques);
    
    return;
}

/*
 * used when the user has elected to catch
 * up newsgroups in newsgroup mode
 */
void catchUpNG()
{
    char name[GROUP_NAME_SIZE];
    XawTextPosition left, right;
    int ret;

    if (getSelection(Text, NewsGroupsString, &left, &right)) {
	GroupPosition = left;
	while (left <= right) {
	    currentGroup(Mode, NewsGroupsString, name, left);
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
	    if (!moveCursor(FORWARD, NewsGroupsString, &left)) {
		break;
	    }
	}
	updateNewsgroupMode();
	return;
    }
    (void) moveCursor(BACK, NewsGroupsString, &left);
    GroupPosition = left;
    XawTextSetInsertionPoint(Text, GroupPosition);
    
    return;
}

/*
 * Unsubscribe user from selected group(s)
 */
void unsubscribeNG()
{
    char name[GROUP_NAME_SIZE];
    XawTextPosition left, right;
    int ret;

    if (getSelection(Text, NewsGroupsString, &left, &right)) {
	GroupPosition = left;
	while (left <= right) {
	    currentGroup(Mode, NewsGroupsString, name, left);
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
	    if (!moveCursor(FORWARD, NewsGroupsString, &left)) {
		break;
	    }
	}
	updateNewsgroupMode();
	return;
    }
    (void) moveCursor(BACK, NewsGroupsString, &left);
    GroupPosition = left;
    XawTextSetInsertionPoint(Text, GroupPosition);
    
    return;
}

/*
 * Catch up group, and exit article mode
 */
void catchUpART()
{
    catchUp();
    exitArticleMode();
    return;   
}


/*
 * Mark articles not marked as unread between 0 and ArtPosition as read.
 * Get the next unread article and display it, quit
 * if there are no more unread articles.
 */
void catchUpPartART()
{
    char *filename, *question;
    long artNum;
    XawTextPosition left = 0;

    while (left < ArtPosition) {
	if (SubjectString[left] != 'u') {
	    SubjectString[left] = '+';
	    markArticleAsRead(atol(&SubjectString[left+2]));
	}
	(void) moveCursor(FORWARD, SubjectString, &left);
    }
    (void) moveCursor(BACK, SubjectString, &left);
    updateSubjectWidget((XawTextPosition) 0, ArtPosition);
    if (getNearbyArticle(art_UNREAD, &filename, &question, &artNum) == 
	art_DONE) {
	/* XXX this was commented out in 6.17, why??? */
	exitArticleMode();
	return;
    }
    foundArticle(filename, question, artNum);
    
    return;
}

void fedUpART()
{
    catchUp();
    artNextGroupFunction(NULL, NULL, NULL, NULL);
}


/*
 * Unsubscribe user from the current group;
 * exit article mode
 */
void unsubscribeART()
{
    unsubscribe();
    maybeExitArticleMode();
    
    return;
}

static Widget ExitConfirmBox = (Widget) 0;
static Widget CatchUpConfirmBox = (Widget) 0;
static Widget PartCatchUpConfirmBox = (Widget) 0;
static Widget UnSubConfirmBox = (Widget) 0;
static Widget FedUpConfirmBox = (Widget) 0;

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
    switch(Action) {
	case NG_EXIT:
	if ((int) client_data == XRN_YES) {
	    ehNoUpdateExitXRN();
	}
	PopDownDialog(ExitConfirmBox);
	ExitConfirmBox = 0;
	break;

	case NG_QUIT:
	if ((int) client_data == XRN_YES) {
	    ehCleanExitXRN();
	}
	PopDownDialog(ExitConfirmBox);
	ExitConfirmBox = 0;
	break;

	case NG_CATCHUP:
	if ((int) client_data == XRN_YES) {
	    catchUpNG();
	}
	PopDownDialog(CatchUpConfirmBox);
	CatchUpConfirmBox = 0;
    	break;
	    
	case NG_UNSUBSCRIBE:
	if ((int) client_data == XRN_YES) {
	    unsubscribeNG();
	}
	PopDownDialog(UnSubConfirmBox);
	UnSubConfirmBox = 0;
	break;
	    
	case ART_CATCHUP:
	if ((int) client_data == XRN_YES) {
	    catchUpART();
	}	    
	PopDownDialog(CatchUpConfirmBox);
	CatchUpConfirmBox = 0;
	break;
	    
	case ART_PART_CATCHUP:
	if ((int) client_data == XRN_YES) {
	    catchUpPartART();
	}	    
	PopDownDialog(PartCatchUpConfirmBox);
	PartCatchUpConfirmBox = 0;
	break;
	    
	case ART_UNSUBSCRIBE:
	if ((int) client_data == XRN_YES) {
	    unsubscribeART();
	}	    
	PopDownDialog(UnSubConfirmBox);
	UnSubConfirmBox = 0;
	break;
	    
	case ART_FEDUP:
	if ((int) client_data == XRN_YES) {
	    fedUpART();
	}	    
	PopDownDialog(FedUpConfirmBox);
	FedUpConfirmBox = 0;
	break;
    }
    unbusyCursor();
    inCommand = 0;
    return;
}

/*
 * called when the user wants to quit xrn
 *
 *  full update the newsrc file
 *  exit
 */
/*ARGSUSED*/
static void ngQuitFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
        {NO_STRING,               generalHandler, (XtPointer) XRN_NO},
      {YES_STRING,              generalHandler, (XtPointer) XRN_YES},
    };


    if (Mode != NEWSGROUP_MODE) {
	return;
    }
    if (app_resources.confirmMode & NG_QUIT) {
	Action = NG_QUIT;
	if (ExitConfirmBox == (Widget) 0) {
          ExitConfirmBox = CreateDialog(TopLevel, ARE_YOU_SHURE_MSG,
					   DIALOG_NOTEXT, args, XtNumber(args));
	}
	PopUpDialog(ExitConfirmBox);
	return;
    }
    ehCleanExitXRN();
}

/*
 * called when the user wants to read a new newsgroup
 *
 * get the selected group, set the internal pointers, and go to article mode
 *
 */
/*ARGSUSED*/
static void ngReadFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char name[GROUP_NAME_SIZE];
    XawTextPosition left, right;
    int status;
      
    if (Mode != NEWSGROUP_MODE) {
	return;
    }

    resetSelection();
    if (getSelection(Text, NewsGroupsString, &left, &right)) {
	currentGroup(Mode, NewsGroupsString, name, left);
	XawTextUnsetSelection(Text);
	status = enterNewsgroup(name, ENTER_SETUP |
				(NewsgroupDisplayMode ? 0 : ENTER_UNREAD) |
				((NewsgroupEntryMode == XRN_JUMP) ?
				 (ENTER_UNSUBBED | ENTER_JUMPING) : 0));
	NewsgroupEntryMode = XRN_GOTO;
	if (NewsgroupDisplayMode && (status == ENTER_UNREAD)) {
	    mesgPane(XRN_INFO, 0, DISPLAYING_LAST_UNREAD_MSG, name);
	    status = GOOD_GROUP;
	}
	if (status == GOOD_GROUP) {
	    (void) strcpy(LastGroup, name);
	    GroupPosition = left;
	    switchToArticleMode();
	}
	else if (status == XRN_NOUNREAD) {
	     mesgPane(XRN_INFO, 0, PROBABLY_KILLED_MSG, name);
	     updateNewsgroupMode();
	     return;
	}
	else if (status == BAD_GROUP) {
	     mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, name);
	     return;
	}
	else if (status == XRN_NOMORE) {
	    mesgPane(XRN_SERIOUS, 0, NewsgroupDisplayMode ? NO_ARTICLES_MSG :
		     PROBABLY_EXPIRED_MSG, name);
	    updateNewsgroupMode();
	    return;
	}
	else {
	    mesgPane(XRN_SERIOUS, 0, UNKNOWN_ENTER_NG_RESPONSE_MSG, status,
		     "ngReadFunction");
	    return;
	}
    } else {
	(void) moveCursor(BACK, NewsGroupsString, &left);
	GroupPosition = left;
	XawTextSetInsertionPoint(Text, GroupPosition);
    }
    
    return;
}

/*
 * called when the user does not want to read a newsgroup
 *
 * if selected group, set internal group
 * call updateNewsgroupMode
 */
/*ARGSUSED*/
static void ngNextFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    XawTextPosition left, right;
#ifndef MOTIF
    Arg arg[1];
#endif

    if (Mode != NEWSGROUP_MODE) {
	return;
    }
    resetSelection();

    (void) getSelection(Text, NewsGroupsString, &left, &right);
    (void) moveUpWrap(NewsGroupsString, &left);
    GroupPosition = left;
    adjustNewsgroupWidget();
#ifndef MOTIF
    XtSetArg(arg[0], XtNdisplayPosition, NewsgroupTop);
    XtSetValues(Text, arg, XtNumber(arg));
#else
    XmListSetPos(Text,
		 XawTextToMotifIndex(TextMotifString, NewsgroupTop));
#endif
    XawTextSetInsertionPoint(Text, GroupPosition);

    return;
}

/*
 * called when the user wants to move the cursor up in
 * the newsgroup window
 *
 * if selected group, set internal group
 * call updateNewsgroupMode
 */
/*ARGSUSED*/
static void ngPrevFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    XawTextPosition left, right;
#ifndef MOTIF
    Arg arg[1];
#endif

    if (Mode != NEWSGROUP_MODE) {
	return;
    }
    resetSelection();
    
    (void) getSelection(Text, NewsGroupsString, &left, &right);
    (void) moveCursor(BACK, NewsGroupsString, &left);
    GroupPosition = left;
    adjustNewsgroupWidget();
#ifndef MOTIF
    XtSetArg(arg[0], XtNdisplayPosition, NewsgroupTop);
    XtSetValues(Text, arg, XtNumber(arg));
#else
    XmListSetPos(Text,
		 XawTextToMotifIndex(TextMotifString, NewsgroupTop));
#endif
    XawTextSetInsertionPoint(Text, GroupPosition);

    return;
}


/*
 * called to catch up on all unread articles in this newsgroup
 * use a confirmation box if the user has requested it
 * if selected group, set internal group
 */
/*ARGSUSED*/
static void ngCatchUpFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
        {NO_STRING,               generalHandler, (XtPointer) XRN_NO},
      {YES_STRING,              generalHandler, (XtPointer) XRN_YES},
    };

    if (Mode != NEWSGROUP_MODE) {
	return;
    }
    resetSelection();
    
    if (app_resources.confirmMode & NG_CATCHUP) {
	Action = NG_CATCHUP;
	if (CatchUpConfirmBox == (Widget) 0) {
          CatchUpConfirmBox = CreateDialog(TopLevel, OK_CATCHUP_MSG,
					     DIALOG_NOTEXT, args, XtNumber(args));
	}
	PopUpDialog(CatchUpConfirmBox);

	return;
    }
    catchUpNG();

    return;
}

/*
 * called to unsubscribe to a newsgroup
 *
 * if selected group, set internal group
 * do internals
 * call updateNewsgroupMode
 */
/*ARGSUSED*/
static void ngUnsubFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
        {NO_STRING,               generalHandler, (XtPointer) XRN_NO},
      {YES_STRING,              generalHandler, (XtPointer) XRN_YES},
    };    

    if (Mode != NEWSGROUP_MODE) {
	return;
    }
    resetSelection();
    
    if (app_resources.confirmMode & NG_UNSUBSCRIBE) {
	Action = NG_UNSUBSCRIBE;
	if (UnSubConfirmBox == (Widget) 0) {
          UnSubConfirmBox = CreateDialog(TopLevel, OK_TO_SUB_MSG,
					   DIALOG_NOTEXT, args, XtNumber(args));
	}
	PopUpDialog(UnSubConfirmBox);
	return;
    }
    unsubscribeNG();

    return;
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
    char name[GROUP_NAME_SIZE];
    XawTextPosition left, right;
    int ret;

    if (inCommand) {
	return;
    }
    inCommand = 1;
    busyCursor();
    switch ((int) client_data) {
	case XRNsub_LASTGROUP:
	if (LastGroup[0] != '\0') {
	    if ((ret = enterNewsgroup(LastGroup, ENTER_UNSUBBED))
		== BAD_GROUP) {
		mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, name);
		PopDownDialog(SubscribeBox);
		SubscribeBox = 0;
		unbusyCursor();
		inCommand = 0;
		return;
	    }
	    else if (ret == GOOD_GROUP) {
		subscribe();
		updateNewsgroupMode();
	    }
	    else {
		mesgPane(XRN_SERIOUS, 0, UNKNOWN_ENTER_NG_RESPONSE_MSG, ret,
			 "subscribeHandler");
	    }
	}
	break;

	case XRNsub_FIRST:
	if (addToNewsrcBeginning(GetDialogValue(SubscribeBox),
				 status) == GOOD_GROUP) {
	    GroupPosition = 0;
	    updateNewsgroupMode();
	}
	break;

	case XRNsub_LAST:
	if (addToNewsrcEnd(GetDialogValue(SubscribeBox),
			   status) == GOOD_GROUP) {
	    updateNewsgroupMode();
	    endInsertionPoint(NewsGroupsString, &GroupPosition);
	    XawTextSetInsertionPoint(Text, GroupPosition);
	}
	break;

	case XRNsub_CURRENT:
	if (NewsGroupsString[XawTextGetInsertionPoint(Text)] == '\0') {
	    if (addToNewsrcEnd(GetDialogValue(SubscribeBox),
			       status) == GOOD_GROUP) {
		updateNewsgroupMode();
		endInsertionPoint(NewsGroupsString, &GroupPosition);
		XawTextSetInsertionPoint(Text, GroupPosition);
	    }
	} else {

	    /* don't need to check for the null group here, it would have */
	    /* been already handled above */
	    (void) getSelection(Text, NewsGroupsString, &left, &right);
	    GroupPosition = left;
	    if (GroupPosition == 0) {
	        if (addToNewsrcBeginning(GetDialogValue(
		    SubscribeBox),status) == GOOD_GROUP) {
		    updateNewsgroupMode();
		}
	    } else {
		(void) moveCursor(BACK, NewsGroupsString, &GroupPosition);
		currentGroup(Mode, NewsGroupsString, name, GroupPosition);
	        if (addToNewsrcAfterGroup(GetDialogValue(
		    SubscribeBox), name, status) == GOOD_GROUP) {
		    (void) moveUpWrap(NewsGroupsString, &GroupPosition);
		    updateNewsgroupMode();
		}
	    }
    	}
	break;
    }
    
    PopDownDialog(SubscribeBox);
    SubscribeBox = 0;
    XawTextUnsetSelection(Text);
    unbusyCursor();
    inCommand = 0;
    return;
}

/*
 * Subscribe to a group currently unsubscribed to
 */
/*ARGSUSED*/
static void ngSubscribeFunction(widget, event, string, count)
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

    if (Mode != NEWSGROUP_MODE) {
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
    XawTextUnsetSelection(Text);
    if ((int) client_data == XRNgoto_GOTO) {
	name = GetDialogValue(GotoNewsgroupBox);
	if (name[0] == '\0') {
	    mesgPane(XRN_INFO, 0, NO_NG_SPECIFIED_MSG);
	}
	else {
	     ret = enterNewsgroup(name, ENTER_SETUP | ENTER_UNSUBBED |
				  ENTER_JUMPING | ENTER_REGEXP);
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
	GotoNewsgroupString = XtNewString(GetDialogValue(GotoNewsgroupBox));
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
static void ngGotoFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
      {ABORT_STRING,    gotoHandler, (XtPointer) XRNgoto_ABORT},
      {GOTO_NG_STRING , gotoHandler, (XtPointer) XRNgoto_GOTO},
    };

    if (Mode != NEWSGROUP_MODE) {
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

static void switchToAllMode()
{
#ifndef MOTIF
    Arg sargs[5];
#endif

    PreviousMode = Mode;
    Mode = ALL_MODE;

    /* switch buttons */
    swapMode();
    
    setBottomInfoLine(VIEW_ALLNG_SUB_MSG);
    /* create the screen */
    AllGroupsString = getStatusString(AllStatus);

#ifndef MOTIF
    XtSetArg(sargs[0], XtNstring, AllGroupsString);
    XtSetArg(sargs[1], XtNlength, utStrlen(AllGroupsString) + 1);
    XtSetArg(sargs[2], XtNeditType, XawtextRead);
    XtSetArg(sargs[3], XtNuseStringInPlace, True);
    XtSetArg(sargs[4], XtNtype, XawAsciiString);
    AllSource = XtCreateWidget("allTextSource",
				   asciiSrcObjectClass,
				   ArticleText, sargs, XtNumber(sargs));
    XawTextSetSource(ArticleText, AllSource, (XawTextPosition) 0);
    XtSetValues(ArticleText, lineSelArgs, XtNumber(lineSelArgs));
#else
    XawTextSetMotifString(ArticleText, AllGroupsString);
#endif
    
    return;
}

/*ARGSUSED*/
static void ngListOldFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != NEWSGROUP_MODE) {
	return;
    }
    NewsgroupDisplayMode = (NewsgroupDisplayMode == 0) ? 1 : 0;
    redrawNewsgroupTextWidget();
    return;
}

/*
 * Enter "all" mode.  Display all available groups to allow user to
 * subscribe/unsubscribe to them.
 */
/*ARGSUSED*/
static void ngAllGroupsFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    
    if (Mode != NEWSGROUP_MODE) {
	return;
    }
    resetSelection();
    switchToAllMode();
}

/*
 * query the server to see if there are any new articles and groups
 */
/*ARGSUSED*/
static void ngRescanFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != NEWSGROUP_MODE) {
	return;
    }
    resetSelection();
    rescanServer();
    determineMode();
    
    return;
}

/*ARGSUSED*/
static void autoRescan(data, id)
    XtPointer data;
    XtIntervalId *id;
{
    if (Mode != NEWSGROUP_MODE) {
	TimeOut = 0;
	return;
    }
    if (TimeOut != *id) {
      /* fprintf(stderr, BAD_TIMEOUT_MSG, *id, TimeOut); */
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

/*
 * put the user in the previous newsgroup accessed
 */
/*ARGSUSED*/
static void ngPrevGroupFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    int ret;

    if (Mode != NEWSGROUP_MODE) {
	return;
    }
    resetSelection();
    if (LastGroup[0] != '\0') {
	ret = enterNewsgroup(LastGroup, ENTER_SETUP | ENTER_UNSUBBED |
			     ENTER_JUMPING);
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
static void ngSelectFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != NEWSGROUP_MODE) {
	return;
    }
    (void) getSelection(Text, NewsGroupsString, &First, &Last);
    
    return;
}

/*
 * Move the previously selected groups to the position before the
 * current selection
 */
/*ARGSUSED*/
static void ngMoveFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char newGroup[GROUP_NAME_SIZE];
    char oldGroup[GROUP_NAME_SIZE];
    int status = SUBSCRIBE;
    char *newString;
    XawTextPosition left, right;
    XawTextPosition stringPoint;
    XawTextPosition cursorSpot;
    int direction = 0;
    int numGroups = 0;

    if (Mode != NEWSGROUP_MODE) {
	return;
    }
    if (First == Last) {
	mesgPane(XRN_INFO, 0, NO_GROUPS_SELECTED_MSG);
	return;	
    }
    buildString(&newString, First, Last, NewsGroupsString);
    stringPoint = 0;
    (void) getSelection(Text, NewsGroupsString, &left, &right);
    if ((left >= First) && (left <= Last+1)) {
	mesgPane(XRN_SERIOUS, 0, NG_NOT_MOVED_MSG);
	resetSelection();
	return;
    }
    GroupPosition = cursorSpot= left;
    if (left > First) {
	direction = 1;
    }
    currentGroup(Mode, newString, newGroup, stringPoint);
    if (!moveCursor(BACK, NewsGroupsString, &left)) {
	(void) addToNewsrcBeginning(newGroup, status);
	(void) strcpy(oldGroup, newGroup);
	(void) moveCursor(FORWARD, newString, &stringPoint);
	numGroups++;
    } else {
	currentGroup(Mode, NewsGroupsString, oldGroup, left);
	(void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
	(void) strcpy(oldGroup, newGroup);
	(void) moveCursor(FORWARD, newString, &stringPoint);
	numGroups++;
    }
    while (newString[stringPoint] != '\0') {
	numGroups++;
	currentGroup(Mode, newString, newGroup, stringPoint);
	(void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
	(void) strcpy(oldGroup, newGroup);
	if (!moveCursor(FORWARD, newString, &stringPoint)) {
	    break;
	}
    }
    redrawNewsgroupTextWidget();
    if (direction) {
	GroupPosition = cursorSpot;
	while (numGroups > 0) {
	    (void) moveCursor(BACK, NewsGroupsString, &GroupPosition);
	    numGroups--;
	}
	adjustNewsgroupWidget();
    }
    XawTextSetInsertionPoint(Text, GroupPosition);
    resetSelection();
    
    return;
}

/*
 * Quit xrn, leaving the newsrc in the state it was in at
 * the last invokation of rescan.
 */
/*ARGSUSED*/
static void ngExitFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
        {NO_STRING,               generalHandler, (XtPointer) XRN_NO},
      {YES_STRING,              generalHandler, (XtPointer) XRN_YES},
    };

    if (Mode != NEWSGROUP_MODE) {
	return;
    }

    if (app_resources.confirmMode & NG_EXIT) {
	Action = NG_EXIT;
	if (ExitConfirmBox == (Widget) 0) {
          ExitConfirmBox = CreateDialog(TopLevel, ARE_YOU_SHURE_MSG,
					   DIALOG_NOTEXT, args, XtNumber(args));
	}
	PopUpDialog(ExitConfirmBox);
	return;
    }

    ehNoUpdateExitXRN();
}

/*
 * update the .newsrc file
 */
/*ARGSUSED*/
static void ngCheckPointFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != NEWSGROUP_MODE) {
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
static void ngGripeFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != NEWSGROUP_MODE) {
	return;
    }
    gripe();
    return;
}

/*
 * allow user to post an article
 */
/*ARGSUSED*/
static void ngPostFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != NEWSGROUP_MODE) {
	return;
    }
    post(0);
    
    return;
}

/*
 * called when the user wants to scroll the newsgroup list
 */
/*ARGSUSED*/
static void ngScrollFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != NEWSGROUP_MODE) {
	return;
    }
    XtCallActionProc(Text, "next-page", event, 0, 0);
    return;
}

/*
 * called when the user wants to scroll the newsgroup list
 */
/*ARGSUSED*/
static void ngScrollBackFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != NEWSGROUP_MODE) {
	return;
    }
    XtCallActionProc(Text, "previous-page", event, 0, 0);
    return;
}


/*
 * called when the user wants to quit the current newsgroup and go to
 * the next one
 */
/*ARGSUSED*/
static void artQuitFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
    exitArticleMode();
    
    return;
}

/*
 * called when the user wants to read the next article
 */
/*ARGSUSED*/
static void artNextFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *filename;		/* name of the article file */
    char *question;		/* question to put in the question box */
    long artNum;

    if (Mode != ARTICLE_MODE) {
	return;
    }
    selectedArticle(ArtStatus);
    if (getNearbyArticle(ArtStatus, &filename, &question,
			 &artNum) == art_DONE) {
	maybeExitArticleMode();
	return;
    }
    /* update the text window */
    foundArticle(filename, question, artNum);

    ArtStatus = art_NEXT;
    ArtEntry = 0;
    
    return;
}


/*
 * called when the user wants to scroll the current article
 */
/*ARGSUSED*/
static void artScrollFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
    XtCallActionProc(ArticleText, "next-page", event, 0, 0);
    return;
}


/*
 * called when the user wants to scroll the current article
 */
/*ARGSUSED*/
static void artScrollBackFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
    XtCallActionProc(ArticleText, "previous-page", event, 0, 0);
    return;
}


/*
 * called when the user wants to scroll the index window
 */
/*ARGSUSED*/
static void artScrollIndexFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
    XtCallActionProc(Text, "next-page", event, 0, 0);
    return;
}


/*
 * called when the user wants to scroll the index window
 */
/*ARGSUSED*/
static void artScrollIndexBackFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
    XtCallActionProc(Text, "previous-page", event, 0, 0);
    return;
}


/*
 * called when the user wants to scroll the current article
 */
/*ARGSUSED*/
static void artScrollLineFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
    XtCallActionProc(ArticleText, "scroll-one-line-up", event, 0, 0);
    return;
}


/*
 * called when the user wants to scroll the current article
 */
/*ARGSUSED*/
static void artScrollBackLineFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
    XtCallActionProc(ArticleText, "scroll-one-line-down", event, 0, 0);
    return;
}


/*
 * called when the user wants to scroll the current article
 */
/*ARGSUSED*/
static void artScrollEndFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
#ifndef MOTIF
    /* Workaround for TextWidget bug: Only scrolls if insertion point moved */
    XawTextSetInsertionPoint(ArticleText, XawTextTopPosition(ArticleText));
    XtCallActionProc(ArticleText, "end-of-file", event, 0, 0);
#else
    {
      char *data;

      data = XmTextGetString(ArticleText);
      XmTextShowPosition(ArticleText, strlen(data)-1);
      XtCallActionProc(ArticleText, "scroll-one-line-up", event, 0, 0);
      XtFree(data);
    }
#endif
    return;
}


/*
 * called when the user wants to scroll the current article
 */
/*ARGSUSED*/
static void artScrollBeginningFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
#ifndef MOTIF
    /* Workaround for TextWidget bug: Only scrolls if insertion point moved */
    XawTextSetInsertionPoint(ArticleText, XawTextTopPosition(ArticleText));
    XtCallActionProc(ArticleText, "beginning-of-file", 0, 0, 0);
#else
    XmTextShowPosition(ArticleText, 0);
#endif
    return;
}

/*
 * called when the user wants to go to the next unread news
 * article in the current newsgroup
 * 
 */
/*ARGSUSED*/
static void artNextUnreadFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
    ArtStatus = art_UNREAD;
    artNextFunction(widget, NULL, NULL, NULL);
    
    return;
}

/*
 * called when the user wants to read the previous article
 */
/*ARGSUSED*/
static void artPrevFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    XawTextPosition left, right;
    long artNum;
    char *filename, *question;

    if (Mode != ARTICLE_MODE) {
	return;
    }
    ArtStatus = art_PREV;
    XawTextGetSelectionPos(Text, &left, &right);
    XawTextUnsetSelection(Text);
    ArtPosition = left;
    if (left == right) {
	ArtPosition = XawTextGetInsertionPoint(Text);
	if (ArtPosition == 0) {
	    if (getPrevious((XawTextPosition *)&artNum)) {
		gotoArticle(artNum);
		if (getArticle(&filename, &question) != XRN_OKAY) {
		    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, artNum);
		}
		foundArticle(filename, question, artNum);
	    }
	    goto done;
	}
	(void) moveCursor(BACK, SubjectString, &ArtPosition);
    } else {
	moveBeginning(SubjectString, &ArtPosition);
    }
    if (getNearbyArticle(ArtStatus, &filename, &question, &artNum) ==
	art_DONE) {
	maybeExitArticleMode();
	goto done;
    }
    foundArticle(filename, question, artNum);

  done:
    ArtStatus = art_NEXT;
    return;
}

/*ARGSUSED*/
static void artNextGroupFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char name[GROUP_NAME_SIZE];
    struct newsgroup *LastGroupStruct = CurrentGroup;
    int ret;
    char *p;
    char *mesg_name = "artNextGroupFunction";

    LastArticle = NIL(char);
    PrevArticle = CurrentArticle = 0;

    if (! NewsGroupsString) {
	NewsGroupsString = unreadGroups(NewsgroupDisplayMode);
    }

    while (1) {
	 /* 
	  * XXX if the newsgroup is fully read, then when the new newsgroup
	  * string is regenerated (upon entry to newsgroup mode), the string
	  * for this group will not be there and thus the Group Position will
	  * be too far forward (by one group)
	  */

	currentGroup(Mode, NewsGroupsString, name, GroupPosition);
        if (STREQ(name, LastGroup)) {
	    /* last group not fully read */
	    if (! moveCursor(FORWARD, NewsGroupsString, &GroupPosition)) {
		 artQuitFunction(widget, NULL, NULL, NULL);
		 return;
	    }
	    continue;
        }

	 if (*name == '\0') {
	      artQuitFunction(widget, NULL, NULL, NULL);
	      return;
	 }

	/*
	 * Efficiency hack.  If NewsgroupDisplayMode is true, then the
	 * odds are that we won't actually want to try to enter most
	 * of the newsgroups we encounter, so we search forward for
	 * the first newsgroup that we think there are unread articles
	 * in.  However, if rescanOnEnter is true, then we don't do
	 * this, because there might be new articles in a group that
	 * we didn't know about when we built NewsGroupsString.
	 */
	if (NewsgroupDisplayMode && (! app_resources.rescanOnEnter)) {
	    XawTextPosition new_pos;

	    if (! (p = strstr(&NewsGroupsString[GroupPosition], UNREAD_MSG))) {
		artQuitFunction(widget, 0, 0, 0);
		return;
	    }
	    new_pos = p - NewsGroupsString;
	    moveBeginning(NewsGroupsString, &new_pos);
	    if (new_pos > GroupPosition) {
		GroupPosition = new_pos;
		continue;
	    }
	}

	ret = enterNewsgroup(name, ENTER_SETUP | ENTER_UNREAD);

	if (ret == BAD_GROUP) {
	    mesgPane(XRN_SERIOUS, mesg_name, NO_SUCH_NG_DELETED_MSG, name);
	    mesgPane(XRN_SERIOUS | XRN_SAME_LINE, mesg_name,
		     SKIPPING_TO_NEXT_NG_MSG);
	    (void) strcpy(LastGroup, name);
	    continue;
	}
	else if (ret == XRN_NOMORE) {
	    if ((p = strstr(&NewsGroupsString[GroupPosition], NEWS_IN_MSG)) &&
		(p < strstr(&NewsGroupsString[GroupPosition], name))) {
		mesgPane(XRN_INFO, mesg_name, PROBABLY_EXPIRED_MSG, name);
		mesgPane(XRN_INFO | XRN_SAME_LINE, mesg_name,
			 SKIPPING_TO_NEXT_NG_MSG);
	    }
	    (void) strcpy(LastGroup, name);
	    continue;
	}
	else if (ret == XRN_NOUNREAD) {
	    if ((p = strstr(&NewsGroupsString[GroupPosition], UNREAD_MSG)) &&
		(p < strstr(&NewsGroupsString[GroupPosition], name))) {
		mesgPane(XRN_INFO, mesg_name, PROBABLY_KILLED_MSG, name);
		mesgPane(XRN_INFO | XRN_SAME_LINE, mesg_name,
			 SKIPPING_TO_NEXT_NG_MSG);
	    }
	    (void) strcpy(LastGroup, name);
	    continue;
	}
	else if (ret != GOOD_GROUP) {
	    mesgPane(XRN_SERIOUS, mesg_name, UNKNOWN_ENTER_NG_RESPONSE_MSG, ret,
		     mesg_name);
	    continue;
	}

	 if (switchToArticleMode() == GOOD_GROUP) {
	      releaseNewsgroupResources(LastGroupStruct);
	      (void) strcpy(LastGroup, name);
	      if (app_resources.updateNewsrc == TRUE) {
		   while (!updatenewsrc())
                     ehErrorRetryXRN(ERROR_CANT_UPDATE_NEWSRC_MSG,
				       True);
	      }
	      return;
	 }
	 /*
	  * Normally, I'd put a call to mesgPane in here to tell the
	  * user that the switchToArticleMode failed, 0, but it isn't
	  * necessary because switchToArticleMode calls bogusNewsgroup
	  * if it fails, and bogusNewsgroup calls mesgPane with an
	  * appropriate message.
	  */
    }
}

/*ARGSUSED*/
static void artFedUpFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
        {NO_STRING,               generalHandler, (XtPointer) XRN_NO},
      {YES_STRING,              generalHandler, (XtPointer) XRN_YES},
    };

    if (app_resources.confirmMode & ART_FEDUP) {
	Action = ART_FEDUP;
	if (FedUpConfirmBox == (Widget) 0) {
          FedUpConfirmBox = CreateDialog(TopLevel, ARE_YOU_SHURE_MSG,
					   DIALOG_NOTEXT, args, XtNumber(args));
	}
	PopUpDialog(FedUpConfirmBox);
	return;
    }

    fedUpART();
    return;
}

/*
 * called when the user wants to mark all articles in the current group as read
 */
/*ARGSUSED*/
static void artCatchUpFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    XawTextPosition left, right;

    static struct DialogArg args[] = {
        {NO_STRING,               generalHandler, (XtPointer) XRN_NO},
      {YES_STRING,              generalHandler, (XtPointer) XRN_YES},
    };

    if (Mode != ARTICLE_MODE) {
	return;
    }
    XawTextGetSelectionPos(Text, &left, &right);
    if (left != right) {
	(void) getSelection(Text, SubjectString, &left, &right);
    }
    ArtPosition = right;
    if (left == right) {
	if (app_resources.confirmMode & ART_CATCHUP) {
	    Action = ART_CATCHUP;
	    if (CatchUpConfirmBox == (Widget) 0) {
              CatchUpConfirmBox = CreateDialog(TopLevel, OK_CATCHUP_MSG,
						 DIALOG_NOTEXT, args, XtNumber(args));
	    }
	    PopUpDialog(CatchUpConfirmBox);

	    return;
	}
	catchUpART();
	return;
    }
    if (moveCursor(FORWARD, SubjectString, &ArtPosition)) {
	if (app_resources.confirmMode & ART_CATCHUP) {
	    Action = ART_PART_CATCHUP;
	    if (PartCatchUpConfirmBox == (Widget) 0) {
		PartCatchUpConfirmBox = CreateDialog(TopLevel,
                                                   OK_CATCHUP_CUR_MSG,
						     DIALOG_NOTEXT, args, XtNumber(args));
	    }
	    PopUpDialog(PartCatchUpConfirmBox);
	} else {
	    catchUpPartART();
	}
    }
    return;
}

/*
 * called when the user wants to unsubscribe to the current group
 */
/*ARGSUSED*/
static void artUnsubFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
        {NO_STRING,               generalHandler, (XtPointer) XRN_NO},
      {YES_STRING,              generalHandler, (XtPointer) XRN_YES},
    };

    if (Mode != ARTICLE_MODE) {
	return;
    }
    if (app_resources.confirmMode & ART_UNSUBSCRIBE) {
	Action = ART_UNSUBSCRIBE;
	if (UnSubConfirmBox == (Widget) 0) {
          UnSubConfirmBox = CreateDialog(TopLevel, OK_TO_UNSUB_MSG,
					   DIALOG_NOTEXT, args, XtNumber(args));
	}
	PopUpDialog(UnSubConfirmBox);
	return;
    }
    unsubscribeART();
    return;
}

/*
 * Get selection region, mark articles, redisplay subject window.
 */
static void markFunction _ARGUMENTS((/* char */ int));

static void markFunction(marker)
    char marker;
{
    XawTextPosition left, right;
    
    (void) getSelection(Text, SubjectString, &left, &right);
    markArticles(SubjectString, left, right, marker);
    updateSubjectWidget(left, right);
    
    return;
}

/*
 * Mark selected article(s) as read
 */
/*ARGSUSED*/
static void artMarkReadFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char marker = '+';
    XawTextPosition save;

    if (Mode != ARTICLE_MODE) {
	return;
    }
    save = ArtPosition;
    markFunction(marker);
    ArtPosition = save;
    return;
}

/*
 * Mark selected article(s) as unread
 */
/*ARGSUSED*/
static void artMarkUnreadFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char marker = 'u';
    XawTextPosition save;

    if (Mode != ARTICLE_MODE) {
	return;
    }
    save = ArtPosition;
    markFunction(marker);
    ArtPosition = save;
    return;
}

/*
 * allow user to post to the newsgroup currently being read
 */
/*ARGSUSED*/
static void artPostFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
    post(1);
    
    return;
}


/*
 *
 */
/*ARGSUSED*/
static void artSubNextFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    XawTextPosition left, right;
    char *filename, *question;
    char *subject;
    long artNum;
    int status;
    
    if (Mode != ARTICLE_MODE) {
	return;
    }
    XawTextGetSelectionPos(Text, &left, &right);
    XawTextUnsetSelection(Text);
    ArtPosition = left;
    if (left == right) {
	ArtPosition = XawTextGetInsertionPoint(Text);
	if (SubjectString[ArtPosition] == '\0') {
	    return;
	}
	artNum = atol(&SubjectString[ArtPosition+2]);
	subject = XtNewString(getSubject(artNum));
	(void) moveCursor(FORWARD, SubjectString, &ArtPosition);
	status = isNextSubject(subject, &filename, &question, &artNum);
	switch (status) {
	  case ABORT:
	    FREE(subject);
          infoNow(ERROR_SUBJ_ABORT_MSG);
	    return;

	  case NOCHANGE:
          (void) sprintf(error_buffer, ERROR_SUBJ_SEARCH_MSG);
	    info(error_buffer);
	    FREE(subject);
	    foundArticle(filename, question, artNum);
	    return;

	  case DONE:
	    FREE(subject);
	    ArtPosition = 0;
          infoNow(ERROR_SUBJ_EXH_MSG);
	    if (getNearbyArticle(art_UNREAD,&filename,&question,&artNum) == art_DONE) {
		maybeExitArticleMode();
		return;
	    }
	    foundArticle(filename, question, artNum);
	    return;
	  case EXIT:
	    FREE(subject);
	    maybeExitArticleMode();
	    return;
	}
    }
    if (getNearbyArticle(art_NEXT, &filename, &question, &artNum) == art_DONE) {
	maybeExitArticleMode();
	return;
    }
    foundArticle(filename, question, artNum);
    
    return;
}

/*
 *
 */
/*ARGSUSED*/
static void artSubPrevFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    XawTextPosition left, right;
    char *subject;
    long artNum;
    char *filename, *question;
    int status;
#ifndef MOTIF
    Arg sargs[5];
#endif

    if (Mode != ARTICLE_MODE) {
	return;
    }
    XawTextGetSelectionPos(Text, &left, &right);
    XawTextUnsetSelection(Text);
    ArtPosition = left;
    if (left == right) {
	/* nothing selected, we should be on a valid article */
	ArtPosition = XawTextGetInsertionPoint(Text);
	if (SubjectString[ArtPosition] == '\0') {
	    return;
	}
	artNum = atol(&SubjectString[ArtPosition+2]);
	subject = XtNewString(getSubject(artNum));
      (void) sprintf(error_buffer, ERROR_SUBJ_SEARCH_MSG, subject);
	status = isPrevSubject(subject, &filename, &question, &artNum);
	FREE(subject);
	switch(status) {
	  case ABORT:
          infoNow(ERROR_SUBJ_ABORT_MSG);
	    return;
	  case NOCHANGE:
	    info(error_buffer);
	    foundArticle(filename, question, artNum);
	    return;
	  case CHANGE:
#ifndef MOTIF
	    if (SubjectSource != 0)
#else
	    if (TextMotifString != 0)
#endif
            {
		ArticleTop = XawTextTopPosition(Text);
#ifndef MOTIF
		XawStringSourceDestroy(SubjectSource);
#endif
	    }

#ifndef MOTIF
	    XtSetArg(sargs[0], XtNstring, SubjectString);
	    XtSetArg(sargs[1], XtNlength, utStrlen(SubjectString) + 1);
	    XtSetArg(sargs[2], XtNeditType, XawtextRead);
	    XtSetArg(sargs[3], XtNuseStringInPlace, True);
	    XtSetArg(sargs[4], XtNtype, XawAsciiString);
	    SubjectSource = XtCreateWidget("subjectTextSource",
				       asciiSrcObjectClass,
				       Text, sargs, XtNumber(sargs));
	    XawTextSetSource(Text, SubjectSource, ArticleTop);
#else
	    XawTextSetMotifString(Text, SubjectString);
#endif
	    info(error_buffer);
	    foundArticle(filename, question, artNum);
	    return;
	  case DONE:
          infoNow(ERROR_SUBJ_EXH_MSG);
	    return;
	  case EXIT:
	    maybeExitArticleMode();
	    return;
	}
    }
    moveBeginning(SubjectString, &left);
    ArtPosition = left;
    artNum = atol(&SubjectString[ArtPosition+2]);
    gotoArticle(artNum);
    if (getArticle(&filename, &question) != XRN_OKAY) {
	mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, artNum);
#ifndef MOTIF
	removeLine(SubjectString, Text, &SubjectSource, ArtPosition,
		   &ArticleTop);
#else
	removeLine(SubjectString, Text, (Widget *)&TextMotifString, ArtPosition,
		   &ArticleTop);
#endif
	if (getNearbyArticle(art_NEXT, &filename, &question, &artNum) == art_DONE) {
	    maybeExitArticleMode();
	    return;
	}
	infoNow(error_buffer);
	foundArticle(filename, question, artNum);
	return;
    }
    infoNow(error_buffer);
    foundArticle(filename, question, artNum);
    
    return;
}

char *SubjectKilled;

/*
 * Allow user to mark all articles with the current subject as read
 *
 * XXX get subject, kill using data structures, rebuild SubjectString
 */
static void _artKillSession _ARGUMENTS((Widget));

/*ARGSUSED*/
static void _artKillSession(widget)
    Widget widget;
{
    XawTextPosition left, right, save;
    char *subject;
    char *cursubject;
    char *filename, *question;
    long artNum;
#ifndef MOTIF
    Arg sargs[5];
#endif

    if (Mode != ARTICLE_MODE) {
	return;
    }
    if (!getSelection(Text, SubjectString, &left, &right)) {
	return;
    }
    ArtPosition = left;
    save = ArtPosition;
#ifdef ellen
    XawTextGetSelectionPos(Text, &left, &right);
    XawTextUnsetSelection(Text);
    if (left == right) {
	ArtPosition = XawTextGetInsertionPoint(Text);
	if (SubjectString[ArtPosition] == '\0') {
	    return;
	}
	left = ArtPosition;
    } else {
	ArtPosition = left;
    }
    moveBeginning(SubjectString, &ArtPosition);
#endif
    artNum = atol(&SubjectString[ArtPosition+2]);
    subject = XtNewString(getSubject(artNum));
    SubjectKilled = XtNewString(subject);
    ArtPosition = 0;
    while (SubjectString[ArtPosition] != '\0') {
	artNum = atol(&SubjectString[ArtPosition+2]);
	cursubject = getSubject(artNum);
	/* only kill those that have not been marked as unread */
	if ((STREQ(subject, cursubject)) &&
	    (SubjectString[ArtPosition] != 'u')) {
	    markArticleAsRead(artNum);
	    (void) markStringRead(SubjectString, ArtPosition);
	}
	if (!moveCursor(FORWARD, SubjectString, &ArtPosition)) {
	    break;
	}
    }
    /* set the cursor back to the beginning of the subject screen */
    ArtPosition = save;
    FREE(subject);
    infoNow(ERROR_SUB_KILL_MSG);
    if (getNearbyArticle(art_UNREAD, &filename, &question, &artNum)
	== art_DONE) {
	maybeExitArticleMode();
	return;
    }
#ifndef MOTIF
    if (SubjectSource != 0)
#else
    if (TextMotifString != 0)
#endif
    {
	ArticleTop = XawTextTopPosition(Text);
#ifndef MOTIF
	XawStringSourceDestroy(SubjectSource);
#endif
    }

#ifndef MOTIF
    XtSetArg(sargs[0], XtNstring, SubjectString);
    XtSetArg(sargs[1], XtNlength, utStrlen(SubjectString) + 1);
    XtSetArg(sargs[2], XtNeditType, XawtextRead);
    XtSetArg(sargs[3], XtNuseStringInPlace, True);
    XtSetArg(sargs[4], XtNtype, XawAsciiString);
    SubjectSource = XtCreateWidget("subjectTextSource",
				   asciiSrcObjectClass,
				   Text, sargs, XtNumber(sargs));
    XawTextSetSource(Text, SubjectSource, ArticleTop);
#else
    XawTextSetMotifString(Text, SubjectString);
#endif

    foundArticle(filename, question, artNum);
    
    return;
}
    
/*ARGSUSED*/
static void artKillSessionFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
     _artKillSession(widget);
     FREE(SubjectKilled);
     return;
}

/*
 * Allow user to mark all articles with the current author as read
 *
 * XXX get author, kill using data structures, rebuild SubjectString
 * XXX merge this with artKillSession
 */
/*ARGSUSED*/
static void artKillAuthorFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    XawTextPosition left, right;
    char *author;
    char *curauthor;
    char *filename, *question;
    long artNum;
#ifndef MOTIF
    Arg sargs[5];
#endif

    if (Mode != ARTICLE_MODE) {
	return;
    }
    if (!getSelection(Text, SubjectString, &left, &right)) {
	return;
    }
    ArtPosition = left;
#ifdef ellen
    XawTextGetSelectionPos(Text, &left, &right);
    XawTextUnsetSelection(Text);
    if (left == right) {
	ArtPosition = XawTextGetInsertionPoint(Text);
	if (SubjectString[ArtPosition] == '\0') {
	    return;
	}
	left = ArtPosition;
    } else {
	ArtPosition = left;
    }
    moveBeginning(SubjectString, &ArtPosition);
#endif
    artNum = atol(&SubjectString[ArtPosition+2]);
    author = XtNewString(getAuthor(artNum));
    ArtPosition = 0;
    while (SubjectString[ArtPosition] != '\0') {
	artNum = atol(&SubjectString[ArtPosition+2]);
	curauthor = getAuthor(artNum);
	/* only kill those that have not been marked as unread */
	if ((STREQ(author, curauthor)) &&
	    (SubjectString[ArtPosition] != 'u')) {
	    markArticleAsRead(artNum);
	    (void) markStringRead(SubjectString, ArtPosition);
	}
	if (!moveCursor(FORWARD, SubjectString, &ArtPosition)) {
	    break;
	}
    }
    /* set the cursor back to the beginning of the subject screen */
    ArtPosition = 0;
    FREE(author);
    infoNow(ERROR_AUTHOR_KILL_MSG);
    if (getNearbyArticle(art_UNREAD, &filename, &question, &artNum)
	== art_DONE) {
	maybeExitArticleMode();
	return;
    }
#ifndef MOTIF
    if (SubjectSource != 0)
#else
    if (TextMotifString != 0)
#endif
    {
	ArticleTop = XawTextTopPosition(Text);
#ifndef MOTIF
	XawStringSourceDestroy(SubjectSource);
#endif
    }

#ifndef MOTIF
    XtSetArg(sargs[0], XtNstring, SubjectString);
    XtSetArg(sargs[1], XtNlength, utStrlen(SubjectString) + 1);
    XtSetArg(sargs[2], XtNeditType, XawtextRead);
    XtSetArg(sargs[3], XtNuseStringInPlace, True);
    XtSetArg(sargs[4], XtNtype, XawAsciiString);
    SubjectSource = XtCreateWidget("subjectTextSource",
				   asciiSrcObjectClass,
				   Text, sargs, XtNumber(sargs));
    XawTextSetSource(Text, SubjectSource, ArticleTop);
#else
    XawTextSetMotifString(Text, SubjectString);
#endif

    foundArticle(filename, question, artNum);
    
    return;
}

/*ARGSUSED*/
static void artKillLocalFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    struct newsgroup *newsgroup = CurrentGroup;

    if (Mode != ARTICLE_MODE) {
	return;
    }
    _artKillSession(widget);
    killItem(newsgroup, SubjectKilled, KILL_LOCAL);
    FREE(SubjectKilled);
    return;
}

/*ARGSUSED*/
static void artKillGlobalFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    struct newsgroup *newsgroup = CurrentGroup;

    if (Mode != ARTICLE_MODE) {
	return;
    }
    _artKillSession(widget);
    killItem(newsgroup, SubjectKilled, KILL_GLOBAL);
    FREE(SubjectKilled);
    return;
}


#define XRNgotoArticle_ABORT	0
#define XRNgotoArticle_DOIT	1

static Widget GotoArticleBox = (Widget) 0;

/*ARGSUSED*/
static void artListOldFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *filename, *question;
    int SavePosition;
    int status;
    long artNum;
#ifndef MOTIF
    Arg sargs[5];
#endif
    struct newsgroup *newsgroup = CurrentGroup;
    struct article *articles = GETARTICLES(CurrentGroup);
    int first_bad = 0, last_bad = 0;
    char *mesg_name = "artListOldFunction";
    
    busyCursor();
    XawTextUnsetSelection(Text);
    SavePosition = ArtPosition;

    for (artNum = newsgroup->first; artNum <= newsgroup->last; artNum++) {
	if (IS_UNAVAIL(articles[INDEX(artNum)])) {
	    status = NOMATCH;
	}
	else {
	    status = moveToArticle(artNum, &filename, &question);
	}
	switch (status) {
	case NOMATCH:
	case ERROR:
	    if (last_bad) {
		if (last_bad + 1 == artNum) {
		    last_bad++;
		}
		else {
		    if (first_bad == last_bad) {
			mesgPane(XRN_SERIOUS, mesg_name, ART_NOT_AVAIL_MSG,
				 first_bad);
		    }
		    else {
			mesgPane(XRN_SERIOUS, mesg_name, ARTS_NOT_AVAIL_MSG,
				 first_bad, last_bad);
		    }
		    first_bad = last_bad = artNum;
		}
	    }
	    else {
		first_bad = last_bad = artNum;
	    }
	    break;

	case MATCH:
	    break; /* out of the switch, not out of the "for" loop! */
	}
	if (status == MATCH) {
	    break; /* out of the "for" loop */
	}
    }

    if (last_bad) {
	if (first_bad == last_bad) {
	    mesgPane(XRN_SERIOUS, mesg_name, ART_NOT_AVAIL_MSG, first_bad);
	}
	else {
	    mesgPane(XRN_SERIOUS, mesg_name, ARTS_NOT_AVAIL_MSG,
		     first_bad, last_bad);
	}
	first_bad = last_bad = 0;
    }

    if (artNum <= newsgroup->last) {
#ifndef MOTIF
        if (SubjectSource != 0) {
	    XawStringSourceDestroy(SubjectSource);
	}
#endif

	if (SubjectString != NIL(char)) {
	    FREE(SubjectString);
	}

	SubjectString = getSubjects(ALL);

#ifndef MOTIF
	XtSetArg(sargs[0], XtNstring, SubjectString);
	XtSetArg(sargs[1], XtNlength, utStrlen(SubjectString) + 1);
	XtSetArg(sargs[2], XtNeditType, XawtextRead);
	XtSetArg(sargs[3], XtNuseStringInPlace, True);
	XtSetArg(sargs[4], XtNtype, XawAsciiString);
	SubjectSource = XtCreateWidget("subjectTextSource",
				       asciiSrcObjectClass,
				       Text, sargs, XtNumber(sargs));
	XawTextSetSource(Text, SubjectSource, (XawTextPosition) 0);
#else
        XawTextSetMotifString(Text, SubjectString);
#endif

	ArtPosition = 0;
	foundArticle(filename, question, artNum);
    }
    else {
	ArtPosition = SavePosition;
    }

    unbusyCursor();
    return;
}

/*
 * update the .newsrc file
 */
/*ARGSUSED*/
static void artCheckPointFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }

    while (!updatenewsrc())
      ehErrorRetryXRN(ERROR_CANT_UPDATE_NEWSRC_MSG, True);

    return;
}

static void gotoArticleHandler _ARGUMENTS((Widget, XtPointer, XtPointer));

/*ARGSUSED*/
static void gotoArticleHandler(widget, client_data, call_data)
    Widget widget;
    XtPointer client_data;
    XtPointer call_data;
{
    char *numberstr;
    char *filename, *question;
    int SavePosition;
    int status;
    long artNum, firstArt;
#ifndef MOTIF
    Arg sargs[5];
#endif

    if (inCommand) {
	return;
    }
    inCommand = 1;
    busyCursor();
    XawTextUnsetSelection(Text);
    if ((int) client_data == XRNgotoArticle_ABORT) {
	PopDownDialog(GotoArticleBox);
	GotoArticleBox = 0;
	unbusyCursor();
	inCommand = 0;
	return;
    }
    SavePosition = ArtPosition;
    numberstr = GetDialogValue(GotoArticleBox);
    if (numberstr == NIL(char)) {
	mesgPane(XRN_INFO, 0, NO_ART_NUM_MSG);
	PopDownDialog(GotoArticleBox);
	GotoArticleBox = 0;
	unbusyCursor();
	inCommand = 0;
	return;
    }

    artNum = atol(numberstr);
    if (artNum == 0) {
	mesgPane(XRN_SERIOUS, 0, BAD_ART_NUM_MSG, numberstr);
	PopDownDialog(GotoArticleBox);
	GotoArticleBox = 0;
	unbusyCursor();
	inCommand = 0;
	return;
    }
    
    status = moveToArticle(artNum, &filename, &question);

    switch (status) {

      case NOMATCH:
      case ERROR:
	mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, artNum);
	ArtPosition = SavePosition;
	break;

      case MATCH:
#ifndef MOTIF
	if (SubjectSource != 0) {
	    XawStringSourceDestroy(SubjectSource);
	}
#endif

	if (SubjectString != NIL(char)) {
	    /* Don't change start of list unless moving earlier */
	    if ((firstArt = atol(&SubjectString[2])) < artNum) {
		gotoArticle(firstArt);
	    }
	    FREE(SubjectString);
	}

	SubjectString = getSubjects(ALL);

#ifndef MOTIF
	XtSetArg(sargs[0], XtNstring, SubjectString);
	XtSetArg(sargs[1], XtNlength, utStrlen(SubjectString) + 1);
	XtSetArg(sargs[2], XtNeditType, XawtextRead);
	XtSetArg(sargs[3], XtNuseStringInPlace, True);
	XtSetArg(sargs[4], XtNtype, XawAsciiString);
	SubjectSource = XtCreateWidget("subjectTextSource",
				       asciiSrcObjectClass,
				       Text, sargs, XtNumber(sargs));
	XawTextSetSource(Text, SubjectSource, (XawTextPosition) 0);
#else
	XawTextSetMotifString(Text, SubjectString);
#endif

	ArtPosition = 0;
	findArticle(SubjectString, artNum, &ArtPosition);
	gotoArticle(artNum);
	foundArticle(filename, question, artNum);
	break;
    }

    PopDownDialog(GotoArticleBox);
    GotoArticleBox = 0;
    unbusyCursor();
    inCommand = 0;
    return;
}

/*ARGSUSED*/
static void artGotoArticleFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
      {ABORT_STRING,   gotoArticleHandler, (XtPointer) XRNgotoArticle_ABORT},
      {DOIT_STRING, gotoArticleHandler, (XtPointer) XRNgotoArticle_DOIT},
    };
    
    if (Mode != ARTICLE_MODE) {
	return;
    }
    if (GotoArticleBox == (Widget) 0) {
      GotoArticleBox = CreateDialog(TopLevel, ARTICLE_NUMBER_MSG,
				  DIALOG_TEXT, args, XtNumber(args));
    }
    PopUpDialog(GotoArticleBox);
    return;
}

#define XRNsubSearch_ABORT 0
#define XRNsubSearch_FORWARD 1
#define XRNsubSearch_BACK 2

static Widget SubSearchBox = (Widget) 0;

#define CLEANUP \
	if (SubSearchBox) PopDownDialog(SubSearchBox); \
	SubSearchBox = 0; \
	inCommand = 0; \
	unbusyCursor();

static void subSearchHandler _ARGUMENTS((Widget, XtPointer, XtPointer));

/*ARGSUSED*/
static void subSearchHandler(widget, client_data, call_data)
    Widget widget;
    XtPointer client_data;
    XtPointer call_data;
{
    char *regexpr;
    char *filename, *question;
    int SavePosition;
    int status;
    int direction;
    long artNum;
#ifndef MOTIF
    Arg sargs[5];
#endif

    if (inCommand) {
	return;
    }
    inCommand = 1;

    busyCursor();
    XawTextUnsetSelection(Text);
    if ((int) client_data == XRNsubSearch_ABORT) {
	CLEANUP;
	return;
    }
    SavePosition = ArtPosition;
    regexpr = GetDialogValue(SubSearchBox);
    if (*regexpr == 0) {
	if (LastRegexp == NIL(char)) {
	    mesgPane(XRN_INFO, 0, NO_PREV_REGEXP_MSG);
	    CLEANUP;
	    return;	   
	}
	regexpr = LastRegexp;
    } else {
	if (LastRegexp != NIL(char)) {
	    FREE(LastRegexp);
	}
	LastRegexp = XtNewString(regexpr);
    }

    /* XXX */
    if (SubSearchBox) PopDownDialog(SubSearchBox);
    SubSearchBox = 0;
    
    direction = ((int) client_data == XRNsubSearch_FORWARD) ? FORWARD : BACK;
    LastSearch = direction;
#ifndef MOTIF
    status = subjectSearch(direction, &SubjectString, SubjectSource, 
			   &ArtPosition, ArticleTop, LastRegexp,
			   &filename, &question, &artNum);
#else
    status = subjectSearch(direction, &SubjectString, (Widget)TextMotifString, 
			   &ArtPosition, ArticleTop, LastRegexp,
			   &filename, &question, &artNum);
#endif
    switch (status) {
      case ABORT:
      infoNow(ERROR_SUBJ_ABORT_MSG);
	ArtPosition = SavePosition;
	break;

      case NOMATCH:
      (void) sprintf(error_buffer, ERROR_SUBJ_EXPR_MSG,
		       LastRegexp);
	infoNow(error_buffer);
      case ERROR:
	ArtPosition = SavePosition;
	break;

      case MATCH:
      (void) sprintf(error_buffer, ERROR_SEARCH_MSG, LastRegexp);
	infoNow(error_buffer);
	foundArticle(filename, question, artNum);
	break;

      case WINDOWCHANGE:
      (void) sprintf(error_buffer, ERROR_SEARCH_MSG, LastRegexp);
	infoNow(error_buffer);
#ifndef MOTIF
	if (SubjectSource != 0)
#else
	if (TextMotifString != 0)
#endif
        {
	    ArticleTop = XawTextTopPosition(Text);
#ifndef MOTIF
	    XawStringSourceDestroy(SubjectSource);
#endif
	}
#ifndef MOTIF
	XtSetArg(sargs[0], XtNstring, SubjectString);
	XtSetArg(sargs[1], XtNlength, utStrlen(SubjectString) + 1);
	XtSetArg(sargs[2], XtNeditType, XawtextRead);
	XtSetArg(sargs[3], XtNuseStringInPlace, True);
	XtSetArg(sargs[4], XtNtype, XawAsciiString);
	SubjectSource = XtCreateWidget("subjectTextSource",
				       asciiSrcObjectClass,
				       Text, sargs, XtNumber(sargs));
	XawTextSetSource(Text, SubjectSource, ArticleTop);
#else
	XawTextSetMotifString(Text, SubjectString);
#endif
	foundArticle(filename, question, artNum);
	break;

      case EXIT:
	maybeExitArticleMode();
	break;
    }

    CLEANUP;
    return;
}

/*ARGSUSED*/
static void artSubSearchFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
      {ABORT_STRING,   subSearchHandler, (XtPointer) XRNsubSearch_ABORT},
      {FORWARD_STRING, subSearchHandler, (XtPointer) XRNsubSearch_FORWARD},
      {BACK_STRING,    subSearchHandler, (XtPointer) XRNsubSearch_BACK},
    };
    
    if (Mode != ARTICLE_MODE) {
	return;
    }
    if (SubSearchBox == (Widget) 0) {
      SubSearchBox = CreateDialog(TopLevel, REGULAR_EXPR_MSG,
				  DIALOG_TEXT, args, XtNumber(args));
    }
    PopUpDialog(SubSearchBox);
    return;
}

/*
 * Continue a previously started regular expression
 * search of the subject lines.  Search for same
 * regular expression, in same direction.
 */
/*ARGSUSED*/
static void artContinueFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *filename, *question;
    XawTextPosition SavePosition;
    int status;
    long artNum;
#ifndef MOTIF
    Arg sargs[5];
#endif

    if (!LastRegexp) {
	mesgPane(XRN_INFO, 0, NO_PREV_REGEXP_MSG);
	return;
    }
    (void) sprintf(error_buffer, ERROR_SEARCH_MSG, LastRegexp);
    info(error_buffer);
    XawTextUnsetSelection(Text);
    SavePosition = ArtPosition;
#ifndef MOTIF
    status = subjectSearch(LastSearch, &SubjectString, SubjectSource,
			   &ArtPosition, ArticleTop, NIL(char),
			   &filename, &question, &artNum);
#else
    status = subjectSearch(LastSearch, &SubjectString, (Widget)TextMotifString,
			   &ArtPosition, ArticleTop, NIL(char),
			   &filename, &question, &artNum);
#endif
    switch (status) {
      case ABORT:
      infoNow(ERROR_SUBJ_ABORT_MSG);
	ArtPosition = SavePosition;
	return;
      case NOMATCH:
      (void) sprintf(error_buffer, ERROR_SUBJ_EXPR_MSG, LastRegexp);
	infoNow(error_buffer);
      case ERROR:
	ArtPosition = SavePosition;
	return;
      case MATCH:
      (void) sprintf(error_buffer, ERROR_SEARCH_MSG, LastRegexp);
	infoNow(error_buffer);    
	foundArticle(filename, question, artNum);
	return;
      case WINDOWCHANGE:
      (void) sprintf(error_buffer, ERROR_SEARCH_MSG, LastRegexp);
	infoNow(error_buffer);
#ifndef MOTIF	
	if (SubjectSource != 0)
#else
	if (TextMotifString != 0)
#endif
        {
	    ArticleTop = XawTextTopPosition(Text);
#ifndef MOTIF
	    XawStringSourceDestroy(SubjectSource);
#endif
	}

#ifndef MOTIF
	XtSetArg(sargs[0], XtNstring, SubjectString);
	XtSetArg(sargs[1], XtNlength, utStrlen(SubjectString) + 1);
	XtSetArg(sargs[2], XtNeditType, XawtextRead);
	XtSetArg(sargs[3], XtNuseStringInPlace, True);
	XtSetArg(sargs[4], XtNtype, XawAsciiString);
	SubjectSource = XtCreateWidget("subjectTextSource",
				       asciiSrcObjectClass,
				       Text, sargs, XtNumber(sargs));
	XawTextSetSource(Text, SubjectSource, ArticleTop);
#else
	XawTextSetMotifString(Text, SubjectString);
#endif
	foundArticle(filename, question, artNum);
	return;
      case EXIT:
	maybeExitArticleMode();
	return;
    }
}
	
/*
 * Display the article accessed before the current one
 */
/*ARGSUSED*/
static void artLastFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *filename, *question;

    if (Mode != ARTICLE_MODE) {
	return;
    }
    if (PrevArticle == 0) {
	mesgPane(XRN_INFO, 0, NO_PREV_ART_MSG);
	return;
    }
    ArtPosition = 0;
    findArticle(SubjectString, PrevArticle, &ArtPosition);
    gotoArticle(PrevArticle);
    if (getArticle(&filename, &question) != XRN_OKAY) {
	mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, PrevArticle);
#ifndef MOTIF
	removeLine(SubjectString, Text, &SubjectSource, ArtPosition, &ArticleTop);
#else
	removeLine(SubjectString, Text, (Widget *)&TextMotifString, ArtPosition, &ArticleTop);
#endif
    } else {
	foundArticle(filename, question, PrevArticle);
    }
    
    return;
}

/*
 * Exit from the current newsgroup, marking all articles as
 * unread
 */
/*ARGSUSED*/
static void artExitFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    XawTextPosition beg, end;

    if (Mode != ARTICLE_MODE) {
	return;
    }
    beg = (XawTextPosition) 0;
    end = (XawTextPosition) 0;
    endInsertionPoint(SubjectString, &end);
    moveEnd(SubjectString, &end);
    markArticles(SubjectString, beg, end, ' ');
    exitArticleMode();
    
    return;
}

/*
 * unsubscribe to the remaining groups and exit add mode
 */
/*ARGSUSED*/
static void addQuitFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char newGroup[GROUP_NAME_SIZE];
    int left, right, nbytes;
    int status = UNSUBSCRIBE;

    if (Mode != ADD_MODE) {
	return;
    }
    left = 0;

    /*
     * go through the remaining groups, add them
     * to the end of the newsrc and unsubscribe them
     */
    while (AddGroupsString[left] != '\0') {
	for (right = left; AddGroupsString[right] != '\n'; right++);
	nbytes = right - left;
	(void) strncpy(newGroup, &AddGroupsString[left], nbytes);
	newGroup[nbytes] = '\0';
	(void) addToNewsrcEnd(newGroup, status);
	left = right+1;
    }

    exitAddMode();
    return;
}

/*
 * Find selected group(s) and add them to the .newsrc in the first position.
 * Move the cursor to the next group.
 * Update the AddGroupsString, going into newsgroup mode if it
 * is NULL.  Update the text window, update the insertion point.
 *
 */
/*ARGSUSED*/
static void addFirstFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char newGroup[GROUP_NAME_SIZE];
    char oldGroup[GROUP_NAME_SIZE];
    int status = SUBSCRIBE;
    XawTextPosition gbeg, left, right;

    if (Mode != ADD_MODE) {
	return;
    }
    if (getSelection(Text, AddGroupsString, &left, &right)) {
	gbeg = left;
	currentGroup(Mode, AddGroupsString, newGroup, gbeg);
	(void) addToNewsrcBeginning(newGroup, status);
	(void) strcpy(oldGroup, newGroup);
	(void) moveCursor(FORWARD, AddGroupsString, &gbeg);
	while (gbeg <= right) {
	    currentGroup(Mode, AddGroupsString, newGroup, gbeg);
	    (void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
	    (void) strcpy(oldGroup, newGroup);
	    if (!moveCursor(FORWARD, AddGroupsString, &gbeg)) {
		break;
	    }
	}
	(void) strcpy(&AddGroupsString[left], &AddGroupsString[right+1]);
	if (setCursorCurrent(AddGroupsString, &left)) {
	    /* update the text window */
	    redrawAddTextWidget(left);
	} else {
	    exitAddMode();
	}
    } else {
	(void) moveUpWrap(AddGroupsString, &left);
	XawTextSetInsertionPoint(Text, left);
    }
    
    return;
}

/*
 * add the currently selected group(s) to the end of the .newsrc file
 * and subscribe to them.
 */
/*ARGSUSED*/
static void addLastFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char newGroup[GROUP_NAME_SIZE];
    int status = SUBSCRIBE;
    XawTextPosition gbeg, left, right;

    if (Mode != ADD_MODE) {
	return;
    }
    if (getSelection(Text, AddGroupsString, &left, &right)) {
	gbeg = left;
	while (gbeg <= right) {
	    currentGroup(Mode, AddGroupsString, newGroup, gbeg);
	    (void) addToNewsrcEnd(newGroup, status);
	    if (!moveCursor(FORWARD, AddGroupsString, &gbeg)) {
		break;
	    }
	}
	(void) strcpy(&AddGroupsString[left], &AddGroupsString[right+1]);
	if (setCursorCurrent(AddGroupsString, &left)) {
	    redrawAddTextWidget(left);
	} else {
	    exitAddMode();
	}
    } else {
	(void) moveUpWrap(AddGroupsString, &left);
	XawTextSetInsertionPoint(Text, left);
    }
    
    return;
}

/* entering the name of a newsgroup to add after */

#define XRNadd_ADD 1
#define XRNadd_ABORT 0

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
    char newGroup[GROUP_NAME_SIZE];
    char oldGroup[GROUP_NAME_SIZE];
    int status = SUBSCRIBE;
    XawTextPosition gbeg, left, right;

    if (inCommand) {
	return;
    }
    inCommand = 1;
    busyCursor();
    if (getSelection(Text, AddGroupsString, &left, &right)) {
	if ((int) client_data == XRNadd_ADD) {
	    gbeg = left;
	    currentGroup(Mode, AddGroupsString, newGroup, gbeg);
	    if (addToNewsrcAfterGroup(newGroup,
				      GetDialogValue(AddBox),
				      status) == GOOD_GROUP) {
		(void) moveCursor(FORWARD, AddGroupsString, &gbeg);
		while (gbeg <= right) {
		    (void) strcpy(oldGroup, newGroup);
		    currentGroup(Mode, AddGroupsString, newGroup, gbeg);
		    (void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
		    if (!moveCursor(FORWARD,AddGroupsString, &gbeg)) {
			break;
		    }
		}
		(void) strcpy(&AddGroupsString[left],
			      &AddGroupsString[right+1]);
	    }
	}
	if (setCursorCurrent(AddGroupsString, &left)) {
	    redrawAddTextWidget(left);
	} else {
	    exitAddMode();
	}
    } else {
	XawTextSetInsertionPoint(Text, (XawTextPosition) 0);
    }
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
static void addAfterFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
      {ABORT_STRING, addHandler, (XtPointer) XRNadd_ABORT},
      {ADD_STRING,   addHandler, (XtPointer) XRNadd_ADD},
    };

    if (Mode != ADD_MODE) {
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
static void addUnsubFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char newGroup[GROUP_NAME_SIZE];
    int status = UNSUBSCRIBE;
    XawTextPosition gbeg, left, right;

    if (Mode != ADD_MODE) {
	return;
    }
    if (getSelection(Text, AddGroupsString, &left, &right)) {
	gbeg = left;
	while (gbeg <= right) {
	    currentGroup(Mode, AddGroupsString, newGroup, gbeg);
	    (void) addToNewsrcEnd(newGroup, status);
	    if (!moveCursor(FORWARD, AddGroupsString, &gbeg)) {
		break;
	    }
	}
	(void) strcpy(&AddGroupsString[left], &AddGroupsString[right+1]);
	if (setCursorCurrent(AddGroupsString, &left)) {
	    redrawAddTextWidget(left);
	} else {
	    exitAddMode();
	}
    } else {
	(void) moveUpWrap(AddGroupsString, &left);
	XawTextSetInsertionPoint(Text, left);
    }
    return;
}

#define XRNsave_ABORT          0
#define XRNsave_SAVE           1

static Widget SaveBox = (Widget) 0;  /* box for typing in the name of a file */

/*
 * handler for the save box
 */
static void saveHandler _ARGUMENTS((Widget, XtPointer, XtPointer));

/*ARGSUSED*/
static void saveHandler(widget, client_data, call_data)
    Widget widget;
    XtPointer client_data;
    XtPointer call_data;
{
    if (inCommand) {
	return;
    }
    inCommand = 1;
    busyCursor();
    if ((int) client_data != XRNsave_ABORT) {
	XawTextPosition left, right;

	(void) getSelection(Text, SubjectString, &left, &right);
	if (left == right) {
	    if (saveCurrentArticle(GetDialogValue(SaveBox))) {
		SubjectString[ArtPosition+1] = 'S';
		XawTextInvalidate(Text, ArtPosition, ArtPosition + 2);
	    }
	} else {
	    long article;
	    char *template = GetDialogValue(SaveBox);
	    char buffer[1024];

	    while (left < right) {
		article = atol(&SubjectString[left + 2]);
		(void) sprintf(buffer, template, article);
		if (saveArticleByNumber(buffer, article)) {
		    SubjectString[left + 1] = 'S';
		    XawTextInvalidate(Text, left, left + 2);
		}
		(void) moveCursor(FORWARD, SubjectString, &left);
	    }
	    (void) moveCursor(BACK, SubjectString, &left);
	}
	if (SaveString && (SaveString != app_resources.saveString)) {
		XtFree(SaveString);
	}
	SaveString = XtNewString(GetDialogValue(SaveBox));
    }
    PopDownDialog(SaveBox);
    SaveBox = 0;
    unbusyCursor();
    inCommand = 0;
    return;
}    

/*
 * query the user about saving an article
 *
 *    brings up a dialog box
 *
 *    returns: void
 *
 */
/*ARGSUSED*/
static void artSaveFunction(widget, ev, params, num_params)
    Widget widget;
    XEvent *ev;
    String *params;
    Cardinal *num_params;
{
    static struct DialogArg args[] = {
      {ABORT_STRING, saveHandler, (XtPointer) XRNsave_ABORT},
      {SAVE_STRING,  saveHandler, (XtPointer) XRNsave_SAVE},
    };

    if (Mode != ARTICLE_MODE) {
	return;
    }
    if (num_params && *num_params == 1) {
	XawTextPosition left, right;
	char *ssstring;
	ssstring = SaveString;
	SaveString = params[0];

	busyCursor();

	(void) getSelection(Text, SubjectString, &left, &right);
	if (left == right) {
	    if (saveCurrentArticle(SaveString)) {
	        SubjectString[ArtPosition + 1] = 'S';
	        XawTextInvalidate(Text, ArtPosition, ArtPosition + 2);
	    }
	} else {
	    long article;
	    char buffer[1024];

	    while (left < right) {
	    	article = atol(&SubjectString[left + 2]);
	    	(void) sprintf(buffer, SaveString, article);
	    	if (saveArticleByNumber(buffer, article)) {
	    	    SubjectString[left + 1] = 'S';
	    	    XawTextInvalidate(Text, left, left + 2);
	    	}
	    	(void) moveCursor(FORWARD, SubjectString, &left);
	    }
	    (void) moveCursor(BACK, SubjectString, &left);
	}
	SaveBox = 0;
	unbusyCursor();
	inCommand = 0;
	SaveString = ssstring;
	return;
    }
    if (SaveBox == (Widget) 0) {
	if (!SaveString && app_resources.saveString) {
		SaveString = XtNewString(app_resources.saveString);
	}
	SaveBox = CreateDialog(TopLevel, " FileName, +FolderName, or @FolderName? ",
				  SaveString == NULL ? DIALOG_TEXT
				   : SaveString, args, XtNumber(args));
    }
    PopUpDialog(SaveBox);
    return;
}

#ifdef XLATE

/*
 * translate an article
 */
/*ARGSUSED*/
static void artXlateFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *filename, *question;

    if (Mode != ARTICLE_MODE) {
	return;
    }
    if (toggleXlation(&filename, &question) == XRN_OKAY) {
	LastArticle = NIL(char);
	redrawArticleWidget(filename, question);
    }
    return;
}

#endif /* XLATE */


/*ARGSUSED*/
static void artPrintFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    XawTextPosition left, right;
    char buffer[1024];
    struct newsgroup *newsgroup = CurrentGroup;
    art_num art = newsgroup->current;
#ifdef VMS

    /* XXX the VMS does not handle multiple selected articles ... ricks */
    int status;
    short msglen;
    struct dsc$descriptor_s buf_desc = { sizeof(buffer)-1,
	DSC$K_DTYPE_T, DSC$K_CLASS_S, buffer };

    (void) sprintf(buffer, "%sARTICLE-%u.LIS", app_resources.tmpDir,
	CurrentArticle);
    if (saveCurrentArticle(buffer)) {
	(void) sprintf(buffer, "%s %sARTICLE-%u.LIS",
		       app_resources.printCommand, app_resources.tmpDir,
		       CurrentArticle);
	status = system(buffer);
	if (status & 1) {
          info(ARTICLE_QUEUED_MSG);
	    SubjectString[ArtPosition+1] = 'P';
	    XawTextInvalidate(Text, ArtPosition, ArtPosition + 2);
	    SET_PRINTED(newsgroup->articles[INDEX(art)]);
	} else {
	    status = SYS$GETMSG(status, &msglen, &buf_desc, 0, 0);
	    buffer[msglen] = NULL;
	    info(buffer);
	}
    }

#else /* Not VMS */
    (void) sprintf(buffer, "| %s", app_resources.printCommand);
    (void) getSelection(Text, SubjectString, &left, &right);
    if (left == right) {
	if (saveCurrentArticle(buffer)) {
	    SubjectString[ArtPosition+1] = 'P';
	    XawTextInvalidate(Text, ArtPosition, ArtPosition + 2);
	    SET_PRINTED(newsgroup->articles[INDEX(art)]);
	}
    } else {
	long article;

	while (left < right) {
	    article = atol(&SubjectString[left + 2]);
	    if (saveArticleByNumber(buffer, article)) {
		SubjectString[left + 1] = 'P';
		XawTextInvalidate(Text, left, left + 2);
		SET_PRINTED(newsgroup->articles[INDEX(article)]);
	    }
	    (void) moveCursor(FORWARD, SubjectString, &left);
	}
	(void) moveCursor(BACK, SubjectString, &left);
    }
#endif /* VMS */
    return;
}


/*
 * Allow user to post a reply to the currently posted article
 */
/*ARGSUSED*/
static void artReplyFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
    reply();
    return;
}

/*
 * Allow user to forward an article to a user(s)
 */
/*ARGSUSED*/
static void artForwardFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
    forward();
    return;
}

/*
 * Allow user to gripe
 */
/*ARGSUSED*/
static void artGripeFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
    gripe();
    return;
}

/*
 * Allow user to post a followup to the currently displayed article
 */
/*ARGSUSED*/
static void artFollowupFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
    followup();
    return;
}

/*
 * Allow user to both post and mail a response to the currently
 * displayed article
 */
/*ARGSUSED*/
static void artFollowupAndReplyFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
    followup_and_reply();
    return;
}

/*
 * Allow user to cancel the currently displayed article
 */
/*ARGSUSED*/
static void artCancelFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ARTICLE_MODE) {
	return;
    }
    cancelArticle();
    return;
}

/*
 * decrypt a joke
 */
/*ARGSUSED*/
static void artRot13Function(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *filename, *question;

    if (Mode != ARTICLE_MODE) {
	return;
    }
    if (toggleRotation(&filename, &question) == XRN_OKAY) {
	LastArticle = NIL(char);
	redrawArticleWidget(filename, question);
    }
    return;
}

/*ARGSUSED*/
static void artHeaderFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *filename, *question;

    if (Mode != ARTICLE_MODE) {
	return;
    }
    if (toggleHeaders(&filename, &question) == XRN_OKAY) {
	LastArticle = NIL(char);
	redrawArticleWidget(filename, question);
    }
    return;
}


/*
 * Quit all groups mode.
 */
/*ARGSUSED*/
static void allQuitFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ALL_MODE) {
	return;
    }
    resetSelection();
    /* destory string and source and stuff in the dummy source */
    
#ifndef MOTIF
    if (AllSource != 0) {
	XawStringSourceDestroy(AllSource);
	AllSource = 0;
    }
#else
    ArticleTextMotifString = 0;
#endif

    if (AllGroupsString != NIL(char)) {
	FREE(AllGroupsString);
	AllGroupsString = NIL(char);
    }

#ifndef MOTIF
    if (DummySource == 0) {
	static Arg sargs[] = {
	    {XtNstring, (XtArgVal) ""},
	    {XtNlength, (XtArgVal) 2},
	    {XtNeditType, (XtArgVal) XawtextRead},
	    {XtNuseStringInPlace, (XtArgVal) True},
	    {XtNtype, (XtArgVal) XawAsciiString},
	};
	DummySource = XtCreateWidget("dummyTextSource",
				   asciiSrcObjectClass,
				   ArticleText, sargs, XtNumber(sargs));
    }
    XawTextSetSource(ArticleText, DummySource, (XawTextPosition) 0);
#else
    XawTextSetMotifString(ArticleText, NULL);
    ChooseText(True);
#endif
    switchToNewsgroupMode();
    return;
}

/*
 * Make the selected group(s) subscribed to, and leave them in
 * their current position in the newsrc file.
 */
/*ARGSUSED*/
static void allSubFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char newGroup[GROUP_NAME_SIZE];
    XawTextPosition gbeg, left, right;
    int ret;

    if (Mode != ALL_MODE) {
	return;
    }
    resetSelection();
    
    if (getSelection(ArticleText, AllGroupsString, &left, &right)) {
	gbeg = left;
	while (gbeg <= right) {
	    currentGroup(Mode, AllGroupsString, newGroup, gbeg);
	    ret = enterNewsgroup(newGroup, ENTER_UNSUBBED);
	    if (ret == BAD_GROUP) {
		mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, newGroup);
		return;
	    }
	    else if (ret != GOOD_GROUP) {
		mesgPane(XRN_SERIOUS, 0, UNKNOWN_ENTER_NG_RESPONSE_MSG, ret,
			 "allSubFunction");
		return;
	    }
	    subscribe();
	    if (!moveCursor(FORWARD, AllGroupsString, &gbeg)) {
		break;
	    }
	}
      markAllString(AllGroupsString, left, right, SUBED_MSG);
	updateAllWidget(left, right);
    } else {
	(void) moveUpWrap(AllGroupsString, &left);
	XawTextSetInsertionPoint(ArticleText, left);
    }
    return;
}

/*
 * Mark the selected group(s) as subscribed to, and move them to the
 * beginning of the newsrc file.
 */
/*ARGSUSED*/
static void allFirstFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char newGroup[GROUP_NAME_SIZE];
    char oldGroup[GROUP_NAME_SIZE];
    int status = SUBSCRIBE;
    XawTextPosition gbeg, left, right;

    if (Mode != ALL_MODE) {
	return;
    }
    resetSelection();
    
    if (getSelection(ArticleText, AllGroupsString, &left, &right)) {
	gbeg = left;
	currentGroup(Mode, AllGroupsString, newGroup, gbeg);
	(void) addToNewsrcBeginning(newGroup, SUBSCRIBE);
	(void) strcpy(oldGroup, newGroup);
	(void) moveCursor(FORWARD, AllGroupsString, &gbeg);
	while (gbeg <= right) {
	    currentGroup(Mode, AllGroupsString, newGroup, gbeg);
	    (void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
	    (void) strcpy(oldGroup, newGroup);
	    if (!moveCursor(FORWARD, AllGroupsString, &gbeg)) {
		break;
	    }
	}
      markAllString(AllGroupsString, left, right, SUBED_MSG);
	if (AllStatus == 0) {
	    redrawAllWidget((XawTextPosition) 0);
	} else {
	    updateAllWidget(left, right);
	}
    } else {
	(void) moveUpWrap(AllGroupsString, &left);
	XawTextSetInsertionPoint(ArticleText, left);
	return;
    }
    return;
}

/*
 * Mark the selected group(s) as subscribed to, and move them
 * to the end of the newsrc file.
 */
/*ARGSUSED*/
static void allLastFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char newGroup[GROUP_NAME_SIZE];
    int status = SUBSCRIBE;
    XawTextPosition gbeg, left, right;

    if (Mode != ALL_MODE) {
	return;
    }
    resetSelection();
    
    if (getSelection(ArticleText, AllGroupsString, &left, &right)) {
	gbeg = left;
	while (gbeg <= right) {
	    currentGroup(Mode, AllGroupsString, newGroup, gbeg);
	    (void) addToNewsrcEnd(newGroup, status);
	    if (!moveCursor(FORWARD, AllGroupsString, &gbeg)) {
		break;
	    }
	}
      markAllString(AllGroupsString, left, right, SUBED_MSG);
	if (AllStatus == 0) {
	    redrawAllWidget(left);
	} else {
	    updateAllWidget(left, right);
	}
    } else {
	(void) moveUpWrap(AllGroupsString, &left);
	XawTextSetInsertionPoint(ArticleText, left);
	return;
    }
    return;
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
    char newGroup[GROUP_NAME_SIZE];
    char oldGroup[GROUP_NAME_SIZE];
    XawTextPosition gbeg, left, right;
    int all = 0;

    if (inCommand) {
	return;
    }
    inCommand = 1;
    busyCursor();
    if (getSelection(ArticleText, AllGroupsString, &left, &right)) {
	if ((int) client_data == XRNadd_ADD) {
	    gbeg = left;
	    currentGroup(Mode, AllGroupsString, newGroup, gbeg);
	    if (addToNewsrcAfterGroup(newGroup,
				      GetDialogValue(AllBox),
				      SUBSCRIBE) == GOOD_GROUP) {
		(void) moveCursor(FORWARD, AllGroupsString, &gbeg);
		while (gbeg <= right) {
		    (void) strcpy(oldGroup, newGroup);
		    currentGroup(Mode, AllGroupsString, newGroup, gbeg);
		    (void) addToNewsrcAfterGroup(newGroup, oldGroup, SUBSCRIBE);
		    if (!moveCursor(FORWARD, AllGroupsString, &gbeg)) {
			break;
		    }
		}
              markAllString(AllGroupsString, left, right, SUBED_MSG);
		if (AllStatus == 0) {
		    redrawAllWidget(left);
		} else {
		    updateAllWidget(left, right);
		}
		all = 1;
	    }
	}
    } else {
	(void) moveUpWrap(AllGroupsString, &left);
    }
    if (!all) {
	XawTextSetInsertionPoint(ArticleText, left);
    }
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
static void allAfterFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
      {ABORT_STRING,     allHandler, (XtPointer) XRNadd_ABORT},
      {SUB_STRING, allHandler, (XtPointer) XRNadd_ADD},
    };

    if (Mode != ALL_MODE) {
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
static void allUnsubFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char newGroup[GROUP_NAME_SIZE];
    XawTextPosition gbeg, left, right;
    int ret;

    if (Mode != ALL_MODE) {
	return;
    }
    resetSelection();
    
    if (getSelection(ArticleText, AllGroupsString, &left, &right)) {
	gbeg = left;
	while (gbeg <= right) {
	    currentGroup(Mode, AllGroupsString, newGroup, gbeg);
	    ret = enterNewsgroup(newGroup, ENTER_UNSUBBED);
	    if (ret == BAD_GROUP) {
		mesgPane(XRN_SERIOUS, 0, NO_SUCH_NG_DELETED_MSG, newGroup);
		return;
	    }
	    else if (ret != GOOD_GROUP) {
		mesgPane(XRN_SERIOUS, 0, UNKNOWN_ENTER_NG_RESPONSE_MSG, ret,
			 "allUnsubFunction");
		return;
	    }
	    unsubscribe();
	    if (!moveCursor(FORWARD, AllGroupsString, &gbeg)) {
		return;
	    }
	}
      markAllString(AllGroupsString, left, right, UNSUBED_MSG);
	updateAllWidget(left, right);
    } else {
	(void) moveCursor(BACK, AllGroupsString, &left);
	XawTextSetInsertionPoint(ArticleText, left);
    }
    return;
}

/*
 * called when the user wants to scroll the all groups window
 */
/*ARGSUSED*/
static void allScrollFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ALL_MODE) {
	return;
    }
#if !defined(MOTIF)
    XtCallActionProc(ArticleText, "next-page", 0, 0, 0);
#else
    {
      int topItemPosition, itemCount, visibleItemCount;
      Arg args[3];
      
      XtSetArg(args[0], XmNtopItemPosition, &topItemPosition);
      XtSetArg(args[1], XmNitemCount, &itemCount);
      XtSetArg(args[2], XmNvisibleItemCount, &visibleItemCount);
      XtGetValues(ArticleText, args, 3);
      if (topItemPosition+visibleItemCount-1 < itemCount) {
	XmListSetPos(ArticleText, topItemPosition+visibleItemCount-1);
      } else {
	XmListSetPos(ArticleText, itemCount);
      }
    }
#endif
    return;
}

/*
 * called when the user wants to scroll the all groups window
 */
/*ARGSUSED*/
static void allScrollBackFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ALL_MODE) {
	return;
    }
#if !defined(MOTIF)
    XtCallActionProc(ArticleText, "previous-page", 0, 0, 0);
#else
    {
      int topItemPosition;
      Arg args[1];
      
      XtSetArg(args[0], XmNtopItemPosition, &topItemPosition);
      XtGetValues(ArticleText, args, 1);
      XmListSetBottomPos(ArticleText, topItemPosition);
    }
#endif
    return;
}

/*
 * Go to the current newsgroup.  The current
 * group is either the first group of a selection,
 * or, if there is no selection, the group the cursor
 * is currently on (if any).
 */
/*ARGSUSED*/
static void allGotoFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char newGroup[GROUP_NAME_SIZE];
    XawTextPosition left, right;
    int ret;

    if (Mode != ALL_MODE) {
	return;
    }
    resetSelection();
    
    /* get the current group name */

    if (getSelection(ArticleText, AllGroupsString, &left, &right)) {
	currentGroup(Mode, AllGroupsString, newGroup, left);
    } else {
	/* if at the end of the string, move to the beginning and quit */
	(void) moveUpWrap(AllGroupsString, &left);
	XawTextSetInsertionPoint(ArticleText, left);
	return;
    }

    /* jump to the newsgroup */

    ret = enterNewsgroup(newGroup, ENTER_SETUP | ENTER_UNSUBBED |
			 ENTER_JUMPING);
    if (ret == XRN_NOUNREAD) {
	mesgPane(XRN_INFO, 0, DISPLAYING_LAST_UNREAD_MSG, newGroup);
	ret = GOOD_GROUP;
    }

    if (ret == GOOD_GROUP) {
	(void) strcpy(LastGroup, newGroup);
	
	/* free source */

#ifndef MOTIF
	if (AllSource != 0) {
	    XawStringSourceDestroy(AllSource);
	    AllSource = 0;
	}
#else
	ArticleTextMotifString = 0;
#endif

	/* free string */

	if (AllGroupsString != NIL(char)) {
	    FREE(AllGroupsString);
	    AllGroupsString = NIL(char);
	}
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

    return;
}


/*
 * Post to the current newsgroup.  The current
 * group is either the first group of a selection,
 * or, if there is no selection, the group the cursor
 * is currently on (if any).
 */
/*ARGSUSED*/
static void allPostFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char newGroup[GROUP_NAME_SIZE];
    XawTextPosition left, right;
    int ret;

    if (Mode != ALL_MODE) {
	return;
    }
    resetSelection();
    
    /* get the current group name */

    if (getSelection(ArticleText, AllGroupsString, &left, &right)) {
	currentGroup(Mode, AllGroupsString, newGroup, left);
    } else {
	/* if at the end of the string, move to the beginning and quit */
	(void) moveUpWrap(AllGroupsString, &left);
	XawTextSetInsertionPoint(ArticleText, left);
	return;
    }

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

    return;
}


/*
 * Make note of the groups that were selected
 * to be moved.
 */
/*ARGSUSED*/
static void allSelectFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (Mode != ALL_MODE) {
	return;
    }
    resetSelection();
    
    (void) getSelection(ArticleText, AllGroupsString, &First, &Last);
    return;
}

/*
 * Move the groups in the last selection to
 * the current cursor position (before the
 * current selection.
 */
/*ARGSUSED*/
static void allMoveFunction(widget, event, string, count)
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
    XawTextPosition left, right;
    XawTextPosition stringPoint;
    XawTextPosition cursorSpot;
    XawTextPosition ngGroupPosition;
    int direction = 0;
    int numGroups = 0;

    if (Mode != ALL_MODE) {
	return;
    }
    if (First == Last) {
	mesgPane(XRN_INFO, 0, NO_GROUPS_SELECTED_MSG);
	return;
    }
    
    buildString(&newString, First, Last, AllGroupsString);
    stringPoint = 0;
    (void) getSelection(ArticleText, AllGroupsString, &left, &right);
    if ((left >= First) && (left <= Last+1)) {
	mesgPane(XRN_SERIOUS, 0, NG_NOT_MOVED_MSG);
	resetSelection();
	return;
    }
    ngGroupPosition = GroupPosition;
    GroupPosition = cursorSpot = left;
    if (left > First) {
	direction = 1;
    }
    currentMode(newString, newGroup, &status, stringPoint);
    if (!moveCursor(BACK, AllGroupsString, &left)) {
	(void) addToNewsrcBeginning(newGroup, status);
	(void) strcpy(oldGroup, newGroup);
	(void) moveCursor(FORWARD, newString, &stringPoint);
	numGroups++;
    } else {
	currentMode(AllGroupsString, oldGroup, &dummy, left);
	(void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
	(void) strcpy(oldGroup, newGroup);
	(void) moveCursor(FORWARD, newString, &stringPoint);
	numGroups++;
    }
    while (newString[stringPoint] != '\0') {
	numGroups++;
	currentMode(newString, newGroup, &status, stringPoint);
	(void) addToNewsrcAfterGroup(newGroup, oldGroup, status);
	(void) strcpy(oldGroup, newGroup);
	if (!moveCursor(FORWARD, newString, &stringPoint)) {
	    break;
	}
    }
    redrawAllWidget(left);
    if (direction) {
	GroupPosition = cursorSpot;
	while (numGroups > 0) {
	    (void) moveCursor(BACK, AllGroupsString, &GroupPosition);
	    numGroups--;
	}
    }
    GroupPosition = ngGroupPosition;
    resetSelection();
    return;
}

/* 
 * Change the order the groups appear on the screen.
 */
/*ARGSUSED*/
static void allToggleFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
#ifndef MOTIF
    Arg sargs[5];
#endif

    if (Mode != ALL_MODE) {
	return;
    }
    resetSelection();

#ifndef MOTIF
    if (AllSource != 0) {
	XawStringSourceDestroy(AllSource);
	AllSource = 0;
    }
#else
    ArticleTextMotifString = 0;
#endif

    if (AllGroupsString != NIL(char)) {
	FREE(AllGroupsString);
	AllGroupsString = NIL(char);
    }

    AllStatus = (AllStatus == 0) ? 1 : 0;

    /* make the new string and source */

    AllGroupsString = getStatusString(AllStatus);

#ifndef MOTIF
    XtSetArg(sargs[0], XtNstring, AllGroupsString);
    XtSetArg(sargs[1], XtNlength, utStrlen(AllGroupsString) + 1);
    XtSetArg(sargs[2], XtNeditType, XawtextRead);
    XtSetArg(sargs[3], XtNuseStringInPlace, True);
    XtSetArg(sargs[4], XtNtype, XawAsciiString);
    AllSource = XtCreateWidget("allTextSource",
				   asciiSrcObjectClass,
				   ArticleText, sargs, XtNumber(sargs));
    XawTextSetSource(ArticleText, AllSource, (XawTextPosition) 0);
    XtSetValues(ArticleText, lineSelArgs, XtNumber(lineSelArgs));
#else
    XawTextSetMotifString(ArticleText, AllGroupsString);
#endif
    return;
}

void determineMode()
/*
 * determine the initial mode and set up Text, TopButtonBox, and Question
 */
{
    /* set mode, handle text and question */
    PreviousMode = Mode;
    
    if ((AddGroupsString = newGroups()) != NIL(char)) {
	Mode = ADD_MODE;
	setTopInfoLine(SEL_GROUPS_ADDSUB_MSG);
	redrawAddTextWidget((XawTextPosition) 0);
    } else {
	Mode = NEWSGROUP_MODE;

	updateNewsgroupMode();

	/* update the question */
	if (utStrlen(NewsGroupsString) == 0) {
	    setTopInfoLine(NO_MORE_UNREAD_ART_MSG);
	} else {
	    setTopInfoLine(OPEARATION_APPLY_CURSOR_MSG);
	}
    }
    setBottomInfoLine("");

    /* switch buttons */
    swapMode();
    
    return;
}
