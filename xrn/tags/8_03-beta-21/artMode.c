#include <X11/Intrinsic.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Paned.h>

#include <assert.h>

#include "config.h"
#include "artMode.h"
#include "buttons.h"
#include "news.h"
#include "butexpl.h"
#include "Text.h"
#include "InfoLine.h"
#include "ButtonBox.h"
#include "mesg.h"
#include "cursor.h"
#include "mesg_strings.h"
#include "internals.h"
#include "resources.h"
#include "cancel.h"
#include "modes.h"
#include "ngMode.h"
#include "xthelper.h"
#include "newsrcfile.h"
#include "error_hnds.h"
#include "compose.h"
#include "dialogs.h"
#include "save.h"
#include "allMode.h"
#include "snapshot.h"
#include "utils.h"
#include "artstruct.h"
#include "killfile.h"

static int getPrevious _ARGUMENTS((art_num *));
static long findArticle _ARGUMENTS((char *, art_num, Boolean));

/*
  The string to which the list of newsgroups is moved when switching
  from newsgroup mode to article mode, so that artNextGroupFunction
  can behave consistently with what was displayed when in newsgroup
  mode.
*/
static char *ArticleNewsGroupsString = 0;

static String SubjectString;
static art_num FirstListedArticle;
static Widget ArticleFrame;
static Widget SubjectText, SubjectInfoLine, SubjectButtonBox;
static Widget ArticleText, ArticleInfoLine, ArtSpecButtonBox;

/*
  These two variables are maintained by exitArticleMode,
  artNextGroupFunction and foundArticle.  Nothing else should modify
  them.
  */
static art_num PrevArticle;	/* the number of the article displayed */
				/* before the current one */
static art_num CurrentArticle;	/* the number of the article currently */
				/* displayed 			       */

static int ArtStatus = 0;	/* keeps track of what kind of article to */
				/* to search for: next, previous, or next */
				/* unread */

static char *LastRegexp;	/* last regular expression searched for */
static int LastSearch;		/* the direction of the last regular */
				/* expression search */

static char *SaveString = 0;	/* last input to save box */

/* article mode "modes" ... determines what to do: jump out of article */
/* mode, change the subject string, or do nothing */
#define art_DONE 0
#define art_CHANGE 1
#define art_NOCHANGE 2

/* keeps track of which type of article to search for: Next, Unread, or */
/* previous */
#define art_FORWARD	(1<<0)	/* search forward */
#define art_BACKWARD	(1<<1)	/* search backward */
#define art_CURRENT	(1<<2)	/* start on the current line; otherwise,
				   move before starting the search */
#define art_WRAP	(1<<3)	/* if searching forward, wrap around
				   to the beginning if no article
				   found until the end; if searching
				   backward, fetch previous articles
				   and add them to the beginning of
				   the subject index */
#define art_UNREAD	(1<<4)	/* retrieve only unread articles */

static Boolean cursorOnCurrent _ARGUMENTS((void));

BUTTON(artQuit,quit);
BUTTON(artNextUnread,next unread);
BUTTON(artNext,next);
BUTTON(artPrev,prev);
BUTTON(artLast,last);
BUTTON(artCurrent,current);
BUTTON(artUp,up);
BUTTON(artDown,down);
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
BUTTON(artKillSubject,subject kill);
BUTTON(artKillAuthor,author kill);
BUTTON(artSubSearch,subject search);
BUTTON(artContinue,continue);
BUTTON(artPost,post);
BUTTON(artPostAndMail,post and mail);
BUTTON(artMail,mail);
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

XtActionsRec ArtActions[] = {
    {"artQuit",		artQuitAction},
    {"artNextUnread",	artNextUnreadAction},
    {"artNext",		artNextAction},
    {"artPrev",		artPrevAction},
    {"artLast",		artLastAction},
    {"artCurrent",	artCurrentAction},
    {"artUp",		artUpAction},
    {"artDown",		artDownAction},
    {"artNextGroup",	artNextGroupAction},
    {"artGotoArticle",	artGotoArticleAction},
    {"artCatchUp",	artCatchUpAction},
    {"artFedUp",	artFedUpAction},
    {"artMarkRead",	artMarkReadAction},
    {"artMarkUnread",	artMarkUnreadAction},
    {"artUnsub",	artUnsubAction},
    {"artScroll",	artScrollAction},
    {"artScrollBack",	artScrollBackAction},
    {"artScrollLine",	artScrollLineAction},
    {"artScrollBackLine",	artScrollBackLineAction},
    {"artScrollEnd",	artScrollEndAction},
    {"artScrollBeginning",	artScrollBeginningAction},
    {"artScrollIndex",	artScrollIndexAction},
    {"artScrollIndexBack",	artScrollIndexBackAction},
    {"artSubNext",	artSubNextAction},
    {"artSubPrev",	artSubPrevAction},
    {"artKillSubject",	artKillSubjectAction},
    {"artKillAuthor",	artKillAuthorAction},
    {"artSubSearch",	artSubSearchAction},
    {"artContinue",	artContinueAction},
    {"artPost",		artPostAction},
    {"artPostAndMail",	artPostAndMailAction},
    {"artMail",		artMailAction},
    {"artExit",		artExitAction},
    {"artCheckPoint",	artCheckPointAction},
    {"artGripe",	artGripeAction},
    {"artListOld",	artListOldAction},
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
};

int ArtActionsCount = XtNumber(ArtActions);

ButtonList ArtButtonList[] = {
    {"artQuit",			artQuitCallbacks,		ARTQUIT_EXSTR},
    {"artNextUnread",		artNextUnreadCallbacks,		ARTNEXTUNREAD_EXSTR},
    {"artNext",			artNextCallbacks,		ARTNEXT_EXSTR},
    {"artPrev",			artPrevCallbacks,		ARTPREV_EXSTR},
    {"artLast",			artLastCallbacks,		ARTLAST_EXSTR},
    {"artCurrent",		artCurrentCallbacks,		ARTCURRENT_EXSTR},
    {"artUp",			artUpCallbacks,			ARTUP_EXSTR},
    {"artDown",			artDownCallbacks,		ARTDOWN_EXSTR},
    {"artNextGroup",		artNextGroupCallbacks,		ARTNEXTGROUP_EXSTR},
    {"artGotoArticle",		artGotoArticleCallbacks,	ARTGOTOARTICLE_EXSTR},
    {"artCatchUp",		artCatchUpCallbacks,		ARTCATCHUP_EXSTR},
    {"artFedUp",		artFedUpCallbacks,		ARTFEEDUP_EXSTR},
    {"artMarkRead",		artMarkReadCallbacks,		ARTMARKREAD_EXSTR},
    {"artMarkUnread",		artMarkUnreadCallbacks,		ARTMARKUNREAD_EXSTR},
    {"artUnsub",		artUnsubCallbacks,		ARTUNSUB_EXSTR},
    {"artScroll",		artScrollCallbacks,		ARTSCROLL_EXSTR},
    {"artScrollBack",		artScrollBackCallbacks,		ARTSCROLLBACK_EXSTR},
    {"artScrollLine",		artScrollLineCallbacks,		ARTSCROLLLINE_EXSTR},
    {"artScrollBackLine",	artScrollBackLineCallbacks,	ARTSCROLLBACKLINE_EXSTR},
    {"artScrollEnd",		artScrollEndCallbacks,		ARTSCROLLEND_EXSTR},
    {"artScrollBeginning",	artScrollBeginningCallbacks,	ARTSCROLLBEGINNING_EXSTR},
    {"artScrollIndex",		artScrollIndexCallbacks,	ARTSCROLLINDEX_EXSTR},
    {"artScrollIndexBack",	artScrollIndexBackCallbacks,	ARTSCROLLINDEXBACK_EXSTR},
    {"artSubNext",		artSubNextCallbacks,		ARTSUBNEXT_EXSTR},
    {"artSubPrev",		artSubPrevCallbacks,		ARTSUBPREV_EXSTR},
    {"artKillSubject",		artKillSubjectCallbacks,	ARTKILLSUBJECT_EXSTR},
    {"artKillAuthor",		artKillAuthorCallbacks,		ARTKILLAUTHOR_EXSTR},
    {"artSubSearch",		artSubSearchCallbacks,		ARTSUBSEARCH_EXSTR},
    {"artContinue",		artContinueCallbacks,		ARTCONTINUE_EXSTR},
    {"artPost",			artPostCallbacks,		ARTPOST_EXSTR},
    {"artPostAndMail",		artPostAndMailCallbacks,	ARTPOST_AND_MAIL_EXSTR},
    {"artMail",			artMailCallbacks,		MAIL_EXSTR},
    {"artExit",			artExitCallbacks,		ARTEXIT_EXSTR},
    {"artCheckPoint",		artCheckPointCallbacks,		ARTCHECKPOINT_EXSTR},
    {"artGripe",		artGripeCallbacks,		ARTGRIPE_EXSTR},
    {"artListOld",		artListOldCallbacks,		ARTLISTOLD_EXSTR},
};

int ArtButtonListCount = XtNumber(ArtButtonList);

ButtonList ArtSpecButtonList[] = {
    {"artSave",			artSaveCallbacks,		ARTSAVE_EXSTR},
    {"artReply",		artReplyCallbacks,		ARTREPLY_EXSTR},
    {"artForward",		artForwardCallbacks,		ARTFORWARD_EXSTR},
    {"artFollowup",		artFollowupCallbacks,		ARTFOLLOWUP_EXSTR},
    {"artFollowupAndReply",	artFollowupAndReplyCallbacks,	ARTFOLLOWUPANDREPLY_EXSTR},
    {"artCancel",		artCancelCallbacks,		ARTCANCEL_EXSTR},
    {"artRot13",		artRot13Callbacks,		ARTROT13_EXSTR},
#ifdef XLATE
    {"artXlate",		artXlateCallbacks,		ARTXLATE_EXSTR},
#endif /*XLATE */
    {"artHeader",		artHeaderCallbacks,		ARTHEADER_EXSTR},
    {"artPrint",		artPrintCallbacks,		ARTPRINT_EXSTR},
};

int ArtSpecButtonListCount = XtNumber(ArtSpecButtonList);

/*
  Adjust the upper text window so that the number of lines above the
  cursor is greater than or equal to minLines and less than or equal
  to maxLines, if possible.  If the number of lines is within the
  valid range,	don't do anything; otherwise, scroll so that
  defaultLines are above the cursor, if defaultLines is within the
  valid range, or just enough to put us within the valid range,
  otherwise.  In any case, the cursor will be visible when done.
  */
static void adjustMinMaxLines()
{
    long CursorPosition = TextGetInsertionPoint(SubjectText);
    long top = TextGetTopPosition(SubjectText), TopPosition = top, end;
    int height = TextGetLines(SubjectText);
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
	int ret = moveCursor(FORWARD, SubjectString, &top);
	assert(ret);
    }

    /*
      Figure out how many lines are below the cursor.
      */
    below = 0;
    end = top;
    while (moveCursor(FORWARD, SubjectString, &end) && (below < height))
	below++;

    /*
      If necessary, reposition.
      */
    if ((numLines < min) || (max < numLines)) {
	for (numLines = 1; numLines < def; numLines++) {
	    if (! moveCursor(BACK, SubjectString, &top))
		break;
	    
	}
	for (below += numLines; below <= height; below++) {
	    if (! moveCursor(BACK, SubjectString, &top))
		break;
	}
	if (TopPosition != top)
	    TextSetTopPosition(SubjectText, top);
    }
}

/*
  If left == right, then set the text in the subject window to the
  specified string, preserving the current insertion point.
  Otherwise, invalidate only the specified region of the subject
  window, copying the next text from the specified string into the
  widget.

  After setting the new text, scroll so that the menLines and maxLines
  constraints are met, if update_top is true.
  */
static void updateSubjectWidget _ARGUMENTS((String, long,
					    long, Boolean, Boolean));
    
static void updateSubjectWidget(
				_ANSIDECL(String,	string),
				_ANSIDECL(long,		left),
				_ANSIDECL(long,		right),
				_ANSIDECL(Boolean,	update_top),
				_ANSIDECL(Boolean,	preserve_top)
				)
     _KNRDECL(String,	string)
     _KNRDECL(long,	left)
     _KNRDECL(long,	right)
     _KNRDECL(Boolean,	update_top)
     _KNRDECL(Boolean,	preserve_top)
{
    Boolean disable = False;
    long top WALL(= 0);

    if (preserve_top)
	top = TextGetTopPosition(SubjectText);

    if ((left != right) || update_top || preserve_top)
	disable = True;

    if (disable)
	TextDisableRedisplay(SubjectText);

    if (left == right)
	TextSetString(SubjectText, string);
    else {
	long point = TextGetInsertionPoint(SubjectText);
	TextInvalidate(SubjectText, string, left, right);
	TextSetInsertionPoint(SubjectText, point);
    }

    if (preserve_top)
	TextSetTopPosition(SubjectText, top);
    else if (update_top)
	adjustMinMaxLines();

    if (disable)
	TextEnableRedisplay(SubjectText);
}


/*
  Get the nearest article to the cursor.  If there is no article
  on the current line, search forward or backwards for a valid
  article, depending on the value of status.  Return the filename,
  question and number of the article obtained.
  */
static int getNearbyArticle _ARGUMENTS((int, char **, char **, long *));

static int getNearbyArticle(status, filename, question, artNum)
    int status;
    char **filename, **question;
    long *artNum;
{
    int mesg_name = newMesgPaneName();
    long ArtPosition = TextGetInsertionPoint(SubjectText);
    long beginning = ArtPosition;
    int ret;
    Boolean move = (! (status & art_CURRENT));

    while (1) {
	if (move) {
	    ret = moveCursor((status & art_FORWARD) ? FORWARD : BACK,
			     SubjectString, &ArtPosition);
	    if (! ret) {
		if (status & art_WRAP) {
		    if ((status & art_BACKWARD) && (! getPrevious(artNum))) {
			ret = art_DONE;
			break;
		    }
		    ArtPosition = 0;
		}
		else {
		    ret = art_DONE;
		    break;
		}
	    }
	    if ((status & art_FORWARD) && (ArtPosition == beginning)) {
		ret = art_DONE;
		break;
	    }
	}
	move = True;

	if (! SubjectString[ArtPosition])
	    continue;

	if ((status & art_UNREAD) && (SubjectString[ArtPosition] != ' '))
	    continue;

	*artNum = atol(&SubjectString[ArtPosition + 2]);

	gotoArticle(*artNum);

	if (getArticle(filename, question) != XRN_OKAY) {
	    mesgPane(XRN_SERIOUS, mesg_name, ART_NOT_AVAIL_MSG, *artNum);
	    TextRemoveLine(SubjectText, ArtPosition);
	    removeLine(SubjectString, &ArtPosition);
	    if (status & art_FORWARD)
		move = False;
	    continue;
	}
	ret = art_CHANGE;
	break;
    }

    TextSetInsertionPoint(SubjectText, ArtPosition);
    return ret;
}

#define CHANGE 0		/* subject window has changed */
#define NOCHANGE 1		/* subject window has not changed */
#define DONE 2			/* no new article was found */
				/* EXIT is already defined, it implies */
				/* there are no articles left at all */

/*
 *
 */
static int isPrevSubject _ARGUMENTS((char *, char **, char **, art_num *));

static int isPrevSubject(subject, filename, question, artNum)
    char *subject;
    char **filename, **question;
    art_num *artNum;
{
    long ArtPosition = TextGetInsertionPoint(SubjectText);

    char *newsubject;
    char *newLine;
    char *tmp;
    char *newSubjects = 0;
    int count = 0;
    int ret;
    int line_length = TextGetColumns(SubjectText);

    startSearch();
    abortClear();
    
    for (;;) {
	count++;

	if (count == app_resources.cancelCount) {
	    cancelCreate(0);
	}

	if (abortP()) {
	    failedSearch();
	    cancelDestroy();
	    ret = ABORT;
	    goto done;
	}
	if (SubjectString[ArtPosition] == '\0') {
	    cancelDestroy();
	    ret = EXIT;
	    goto done;
	}
	if (ArtPosition != 0) {
	    (void) moveCursor(BACK, SubjectString, &ArtPosition);
	    *artNum = atol(&SubjectString[ArtPosition+2]);
	    if (! (newsubject = getSubject(*artNum)))
		goto art_unavailable;
	    if (utSubjectCompare(newsubject, subject) == 0) {
		gotoArticle(*artNum);
		if (getArticle(filename, question) != XRN_OKAY) {
		  art_unavailable:
		    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
		    TextRemoveLine(SubjectText, ArtPosition);
		    removeLine(SubjectString, &ArtPosition);
		    continue;
		}
		cancelDestroy();
		TextSetInsertionPoint(SubjectText, ArtPosition);
		ret = NOCHANGE;
		goto done;
	    }
	    continue;
	} else {
	    if ((newLine = getPrevSubject(line_length)) == NIL(char)) {
		failedSearch();
		cancelDestroy();
		ret = DONE;
		goto done;
	    }
	    *artNum = atol(&newLine[2]);
	    if (! (newsubject = getSubject(*artNum))) {
		mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
		continue;
	    }
	    if (newSubjects) {
		tmp = XtMalloc(utStrlen(newSubjects) + utStrlen(newLine) + 1);
		(void) strcpy(tmp, newLine);
		(void) strcat(tmp, newSubjects);
		FREE(newSubjects);
		newSubjects = tmp;
	    } else {
		newSubjects = XtNewString(newLine);
	    }
	    if (utSubjectCompare(newsubject, subject) == 0) {
		/* found a match, go with it */

		gotoArticle(*artNum);
		if (getArticle(filename, question) != XRN_OKAY) {
		    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
		    (void) strcpy(newSubjects, index(newSubjects, '\n'));
		}
		else {
		    tmp = XtMalloc(utStrlen(newSubjects) +
				   utStrlen(SubjectString) + 1);
		    (void) strcpy(tmp, newSubjects);
		    (void) strcat(tmp, SubjectString);
		    FREE(newSubjects);

		    TextDisableRedisplay(SubjectText);
		    TextSetString(SubjectText, tmp);

		    FREE(SubjectString);
		    SubjectString = tmp;

		    FirstListedArticle = *artNum;
		    TextSetInsertionPoint(SubjectText, ArtPosition);
		    TextEnableRedisplay(SubjectText);

		    cancelDestroy();
		    ret = CHANGE;
		    goto done;
		}
	    }
	}
    }
  done:
    FREE(newSubjects);
    return ret;
}


/*
 *
 */
static int isNextSubject _ARGUMENTS((char *, long, char **, char **, art_num *));

static int isNextSubject(subject, ArtPosition, filename, question, artNum)
    char *subject;
    long ArtPosition;
    char **filename, **question;
    art_num *artNum;
{
    char *newsubject;
    int count = 0;
    int ret;

    abortClear();
    
    for (;;) {
	count++;

	if (count == app_resources.cancelCount) {
	    cancelCreate(0);
	}

	if ((count >= app_resources.cancelCount) && ((count % 10) == 0) &&
	    abortP()) {
	    failedSearch();
	    cancelDestroy();
	    ret = ABORT;
	    goto done;
	}
	if (SubjectString[ArtPosition] == '\0') {
	    cancelDestroy();
	    if (ArtPosition == 0) {
		return EXIT;
	    }
	    ret = DONE;
	    goto done;
	}
	*artNum = atol(&SubjectString[ArtPosition+2]);
	if (! (newsubject = getSubject(*artNum)))
	    goto art_unavailable;
	if (utSubjectCompare(newsubject, subject) == 0) {
	    gotoArticle(*artNum);
	    if (getArticle(filename, question) != XRN_OKAY) {
	      art_unavailable:
		mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
		TextRemoveLine(SubjectText, ArtPosition);
		removeLine(SubjectString, &ArtPosition);
		continue;
	    }
	    cancelDestroy();
	    TextSetInsertionPoint(SubjectText, ArtPosition);
	    ret = NOCHANGE;
	    goto done;
	} else {
	    if (!moveCursor(FORWARD, SubjectString, &ArtPosition)) {
		cancelDestroy();
		ret = EXIT;
		goto done;
	    }
	}
    }
  done:
    return ret;
}


static int getPrevious(artNum)
    art_num *artNum;
{
    char *newLine;
    char *newString;
    int line_length = TextGetColumns(SubjectText);

    if ((newLine = getPrevSubject(line_length)) != NIL(char)) {
	newString = XtMalloc(utStrlen(SubjectString) + utStrlen(newLine) + 1);
	(void) strcpy(newString, newLine);
	(void) strcat(newString, SubjectString);
	FREE(SubjectString);
	SubjectString = newString;

	TextSetString(SubjectText, SubjectString);
	*artNum = atol(newLine + 2);
	FirstListedArticle = *artNum;
	return TRUE;
    }
    
    return FALSE;
}


/*
 */
static Boolean selectedArticle _ARGUMENTS((void));

static Boolean selectedArticle()
{
    long left, right;

    if (TextGetSelectedLines(SubjectText, &left, &right)) {
	TextDisableRedisplay(SubjectText);
	TextUnsetSelection(SubjectText);
	TextSetInsertionPoint(SubjectText, left);
	TextEnableRedisplay(SubjectText);
	return True;
    }
    return False;
}

/*
 * install the article mode buttons (and delete the previous mode buttons),
 * build the subject line screen, and call ARTICLE_MODE function 'next unread'
 */
int switchToArticleMode()
{
    int oldMode;

    FREE(SubjectString);
    FirstListedArticle = currentArticle();

    oldMode = PreviousMode;
    
    PreviousMode = CurrentMode;
    CurrentMode = ARTICLE_MODE;

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

    SubjectString = getSubjects(TextGetColumns(SubjectText),
				UNREAD, FirstListedArticle);

    if (! (SubjectString && *SubjectString)) {
	/*
	  We must be entering a newsgroup with no unread articles --
	  display the last read article.
	  */
	FREE(SubjectString);
	SubjectString = getSubjects(TextGetColumns(SubjectText),
				    ALL, FirstListedArticle);
    }

    if (! (SubjectString && *SubjectString)) {
	mesgPane(XRN_INFO, 0, PROBABLY_EXPIRED_MSG, CurrentGroup->name);
	CurrentMode = PreviousMode;
	swapMode();
	/*
	 * the sources and strings have been destroyed at this point
	 * have to recreate them - the redraw routines check the mode
	 * so we can call all of them and only the one that is for the
	 * current mode will do something
	 */
	redrawAllWidget();
	redrawNewsgroupTextWidget(0, True);
	FREE(SubjectString);
	exitNewsgroup();
	return BAD_GROUP;
    }

    /* get rid of previous groups save file string */
    if (SaveString && app_resources.resetSave) {
	if (SaveString != app_resources.saveString) {
		XtFree(SaveString);
	}
	SaveString = XtNewString(app_resources.saveString);
    }	

    /* XXX need to detect when height changes and update the variable */
    server_page_height = TextGetLines(ArticleText);

    setTopInfoLine(OPEARATION_APPLY_CURSOR_MSG);

    if (! (app_resources.leaveHeaders || app_resources.stripHeaders))
	setButtonSensitive(ArtSpecButtonBox, "artHeader", False);
    setButtonSensitive(SubjectButtonBox, "artLast", False);
    setButtonSensitive(SubjectButtonBox, "artContinue", False);
    setButtonSensitive(SubjectButtonBox, "artListOld", True);

    TextDisableRedisplay(SubjectText);

    /* get and display the article */
    updateSubjectWidget(SubjectString, 0, 0, True, False);
    ArtStatus = art_FORWARD | art_CURRENT | art_WRAP;
    artNextFunction(0, 0, 0, 0);

    TextEnableRedisplay(SubjectText);

    return GOOD_GROUP;
}


/*
 * If the article to be displayed has changed, update the article
 * window and redraw the mode line
 */

static void redrawArticleWidget _ARGUMENTS((char *, char *));

static void redrawArticleWidget(filename, question)
    char *filename, *question;
{
    String old = TextGetFile(ArticleText);
    
    if ((! old) || strcmp(old, filename)) {
	TextSetFile(ArticleText, filename);

	setBottomInfoLine(question);

#ifdef CANCEL_CHECK
	if (canCancelArticle())
	    setButtonSensitive(ArtSpecButtonBox, "artCancel", True);
	else
	    setButtonSensitive(ArtSpecButtonBox, "artCancel", False);
#endif

	/* force the screen to update before prefetching */
	xthHandlePendingExposeEvents();
	
	prefetchNextArticle();
    }
    FREE(old);
}

/*
 * release the storage associated with article mode, unlink the article files,
 * and go to newsgroup mode
 */
static void exitArticleMode()
{
    PrevArticle = CurrentArticle = 0;

    FREE(SubjectString);
    TextSetString(SubjectText, "");
    TextSetString(ArticleText, "");

    releaseNewsgroupResources(CurrentGroup);
    if (app_resources.updateNewsrc == TRUE) {
	while (!updatenewsrc())
          ehErrorRetryXRN(ERROR_CANT_UPDATE_NEWSRC_MSG, True);
    }
    exitNewsgroup();

    if (PreviousMode == NEWSGROUP_MODE) {
	switchToNewsgroupMode(True);
    } else {
	switchToAllMode();
    }

    FREE(LastRegexp);

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



/*
 * Display new article, mark as read.
 */
static void foundArticle _ARGUMENTS((char *, char *));

static void foundArticle(file, ques)
    char *file, *ques;
{
    long ArtPosition = TextGetInsertionPoint(SubjectText);

    if ((PrevArticle = CurrentArticle))
	setButtonSensitive(SubjectButtonBox, "artLast", True);

    gotoArticle(markStringRead(SubjectString, ArtPosition));
    CurrentArticle = currentArticle();

    updateSubjectWidget(SubjectString, ArtPosition, ArtPosition + 1, True, False);

    /*
      This is necessary because of a bug in the Xaw Text widget -- it
      doesn't redisplay properly if expose events are processed while
      redisplay is disabled, and redrawArticleWidget() causes pending
      expose events to be processed.
      */
    TextDisplay(SubjectText);

    redrawArticleWidget(file, ques);
}


/*
 * Mark articles not marked as unread between 0 and ArtPosition as read.
 * Get the next unread article and display it, quit
 * if there are no more unread articles.
 */
void _catchUpPartART(exit_mode)
    Boolean exit_mode;
{
    long ArtPosition = TextGetInsertionPoint(SubjectText);

    char *filename, *question;
    long artNum;
    long left = 0;

    while (left < ArtPosition) {
	if (SubjectString[left] != UNREAD_MARKER) {
	    SubjectString[left] = READ_MARKER;
	    markArticleAsRead(atol(&SubjectString[left+2]));
	}
	(void) moveCursor(FORWARD, SubjectString, &left);
    }
    (void) moveCursor(BACK, SubjectString, &left);
    updateSubjectWidget(SubjectString, 0, ArtPosition, True, False);
    if (getNearbyArticle(art_FORWARD | art_CURRENT | art_WRAP | art_UNREAD,
			 &filename, &question, &artNum) ==
	art_DONE) {
	if (exit_mode)
	    exitArticleMode();
	else
	    artNextGroupFunction(NULL, NULL, NULL, NULL);
	return;
    }
    foundArticle(filename, question);
    
    return;
}

void catchUpPartART()
{
    _catchUpPartART(True);
}


/*
 * Catch up group, and exit article mode
 */
void catchUpART()
{
    TextDisableRedisplay(SubjectText);
    TextSetInsertionPoint(SubjectText, TextGetLength(SubjectText));
    _catchUpPartART(True);
    TextEnableRedisplay(SubjectText);
}


void fedUpART()
{
    TextDisableRedisplay(SubjectText);
    TextSetInsertionPoint(SubjectText, TextGetLength(SubjectText));
    _catchUpPartART(False);
    TextEnableRedisplay(SubjectText);
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

/*
 * called when the user wants to quit the current newsgroup and go to
 * the next one
 */
/*ARGSUSED*/
void artQuitFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    exitArticleMode();
    
    return;
}

/*
 * called when the user wants to read the next article
 */
/*ARGSUSED*/
void artNextFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *filename;		/* name of the article file */
    char *question;		/* question to put in the question box */
    long artNum;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    TextDisableRedisplay(SubjectText);

    if (selectedArticle() || !cursorOnCurrent())
	ArtStatus |= art_CURRENT;

    if (getNearbyArticle(ArtStatus, &filename, &question,
			 &artNum) == art_DONE) {
	maybeExitArticleMode();
	goto done;
    }
    /* update the text window */
    foundArticle(filename, question);

    ArtStatus = art_FORWARD;

  done:
    TextEnableRedisplay(SubjectText);
    return;
}

/*
 * called when the user wants to view the article currently under the cursor
 */
/*ARGSUSED*/
void artCurrentFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *filename;		/* name of the article file */
    char *question;		/* question to put in the question box */
    long artNum;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    TextDisableRedisplay(SubjectText);

    (void) selectedArticle();

    if (cursorOnCurrent())
	goto done;

    ArtStatus |= art_CURRENT;

    if (getNearbyArticle(ArtStatus, &filename, &question,
			 &artNum) == art_DONE) {
	maybeExitArticleMode();
	goto done;
    }
    /* update the text window */
    foundArticle(filename, question);

    ArtStatus = art_FORWARD;

  done:
    TextEnableRedisplay(SubjectText);
    return;
}


/*
 * called when the user wants to scroll the current article
 */
/*ARGSUSED*/
void artScrollFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    TextScrollPage(ArticleText, FORWARD);
    return;
}


/*
 * called when the user wants to scroll the current article
 */
/*ARGSUSED*/
void artScrollBackFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    TextScrollPage(ArticleText, BACK);
    return;
}


/*
 * called when the user wants to scroll the index window
 */
/*ARGSUSED*/
void artScrollIndexFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    TextScrollPage(SubjectText, FORWARD);
    return;
}


/*
 * called when the user wants to scroll the index window
 */
/*ARGSUSED*/
void artScrollIndexBackFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    TextScrollPage(SubjectText, BACK);
    return;
}


/*
 * called when the user wants to scroll the current article
 */
/*ARGSUSED*/
void artScrollLineFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    TextScrollLine(ArticleText, FORWARD);
    return;
}


/*
 * called when the user wants to scroll the current article
 */
/*ARGSUSED*/
void artScrollBackLineFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    TextScrollLine(ArticleText, BACK);
    return;
}


/*
 * called when the user wants to scroll the current article
 */
/*ARGSUSED*/
void artScrollEndFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    TextScrollEntire(ArticleText, FORWARD);
    return;
}


/*
 * called when the user wants to scroll the current article
 */
/*ARGSUSED*/
void artScrollBeginningFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    TextScrollEntire(ArticleText, BACK);
    return;
}

/*
  Checks to see if the cursor is on the currently displayed article,
  and return True if it is.
  */
static Boolean cursorOnCurrent ()
{
    long cursor_position = TextGetInsertionPoint(SubjectText);
    long under_cursor;

    if (! SubjectString[cursor_position])
	return False;

    under_cursor = atoi(SubjectString + cursor_position + 2);
    return(under_cursor == currentArticle());
}
    
  
/*
 * called when the user wants to go to the next unread news
 * article in the current newsgroup
 * 
 */
/*ARGSUSED*/
void artNextUnreadFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    ArtStatus = art_FORWARD | art_UNREAD | art_WRAP;
    artNextFunction(widget, NULL, NULL, NULL);

    return;
}

/*
 * called when the user wants to read the previous article
 */
/*ARGSUSED*/
void artPrevFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    art_num artNum;
    char *filename, *question;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    TextDisableRedisplay(SubjectText);

    ArtStatus = art_BACKWARD | art_WRAP;

    if (selectedArticle() || !cursorOnCurrent())
	ArtStatus |= art_CURRENT;

    if (getNearbyArticle(ArtStatus, &filename, &question, &artNum) ==
	art_DONE) {
	goto done;
    }
    foundArticle(filename, question);

  done:
    TextEnableRedisplay(SubjectText);
    ArtStatus = art_FORWARD;
    return;
}

/*ARGSUSED*/
void artNextGroupFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *name = 0;
    struct newsgroup *LastGroupStruct = CurrentGroup;
    int ret;
    char *p;
    int mesg_name = newMesgPaneName();
    long GroupPosition;

    PrevArticle = CurrentArticle = 0;

    if (! ArticleNewsGroupsString)
	ArticleNewsGroupsString = unreadGroups(0 /* doesn't matter */,
					       NewsgroupDisplayMode);

    /* Use name instead of CurrentIndexGroup to prevent CurrentIndexGroup
       from being overwritten by getNearbyNewsgroup. */
    name = XtNewString(CurrentIndexGroup);
    GroupPosition = getNearbyNewsgroup(ArticleNewsGroupsString, &name);

    while (1) {
	currentGroup(CurrentMode, ArticleNewsGroupsString, &name, GroupPosition);
        if (STREQ(name, CurrentIndexGroup)) {
	    /* last group not fully read */
	    if (! moveCursor(FORWARD, ArticleNewsGroupsString,
			     &GroupPosition)) {
		 artQuitFunction(widget, NULL, NULL, NULL);
		 goto done;
	    }
	    continue;
        }

	 if (*name == '\0') {
	      artQuitFunction(widget, NULL, NULL, NULL);
	      goto done;
	 }

	/*
	 * Efficiency hack.  If NewsgroupDisplayMode is true, then the
	 * odds are that we won't actually want to try to enter most
	 * of the newsgroups we encounter, so we search forward for
	 * the first newsgroup that we think there are unread articles
	 * in.  However, if rescanOnEnter is true, then we don't do
	 * this, because there might be new articles in a group that
	 * we didn't know about when we built ArticleNewsGroupsString.
	 */
	if (NewsgroupDisplayMode && (! app_resources.rescanOnEnter)) {
	    long new_pos;

	    if (! (p = strstr(&ArticleNewsGroupsString[GroupPosition],
			      UNREAD_MSG))) {
		artQuitFunction(widget, 0, 0, 0);
		goto done;
	    }
	    new_pos = p - ArticleNewsGroupsString;
	    moveBeginning(ArticleNewsGroupsString, &new_pos);
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
	    CurrentIndexGroup = XtRealloc(CurrentIndexGroup, strlen(name) + 1);
	    (void) strcpy(CurrentIndexGroup, name);
	    continue;
	}
	else if (ret == XRN_NOMORE) {
	    if ((p = strstr(&ArticleNewsGroupsString[GroupPosition],
			    NEWS_IN_MSG)) &&
		(p < strstr(&ArticleNewsGroupsString[GroupPosition], name))) {
		mesgPane(XRN_INFO, mesg_name, PROBABLY_EXPIRED_MSG, name);
		mesgPane(XRN_INFO | XRN_SAME_LINE, mesg_name,
			 SKIPPING_TO_NEXT_NG_MSG);
	    }
	    CurrentIndexGroup = XtRealloc(CurrentIndexGroup, strlen(name) + 1);
	    (void) strcpy(CurrentIndexGroup, name);
	    continue;
	}
	else if (ret == XRN_NOUNREAD) {
	    if ((p = strstr(&ArticleNewsGroupsString[GroupPosition],
			    UNREAD_MSG)) &&
		(p < strstr(&ArticleNewsGroupsString[GroupPosition], name))) {
		mesgPane(XRN_INFO, mesg_name, PROBABLY_KILLED_MSG, name);
		mesgPane(XRN_INFO | XRN_SAME_LINE, mesg_name,
			 SKIPPING_TO_NEXT_NG_MSG);
	    }
	    CurrentIndexGroup = XtRealloc(CurrentIndexGroup, strlen(name) + 1);
	    (void) strcpy(CurrentIndexGroup, name);
	    continue;
	}
	else if (ret != GOOD_GROUP) {
	    mesgPane(XRN_SERIOUS, mesg_name, UNKNOWN_FUNC_RESPONSE_MSG,
		     ret, "enterNewsgroup", "artNextGroupFunction");
	    CurrentIndexGroup = XtRealloc(CurrentIndexGroup, strlen(name) + 1);
	    (void) strcpy(CurrentIndexGroup, name);
	    continue;
	}

	 if (switchToArticleMode() == GOOD_GROUP) {
	      releaseNewsgroupResources(LastGroupStruct);
	      LastGroup = XtRealloc(LastGroup, strlen(name) + 1);
	      (void) strcpy(LastGroup, name);
	      CurrentIndexGroup = XtRealloc(CurrentIndexGroup, strlen(name) + 1);
	      (void) strcpy(CurrentIndexGroup, name);
	      if (app_resources.updateNewsrc == TRUE) {
		   while (!updatenewsrc())
                     ehErrorRetryXRN(ERROR_CANT_UPDATE_NEWSRC_MSG,
				       True);
	      }
	      goto done;
	 }
	 /*
	  * Normally, I'd put a call to mesgPane in here to tell the
	  * user that the switchToArticleMode failed, but it isn't
	  * necessary because switchToArticleMode will display a
	  * message if there's a problem.
	  */
    }

  done:
    XtFree(name);
    return;
}

/*ARGSUSED*/
void artFedUpFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    confirmBox(ARE_YOU_SURE_MSG, ARTICLE_MODE, ART_FEDUP, fedUpART);
}

/*
 * called when the user wants to mark all articles in the current group as read
 */
/*ARGSUSED*/
void artCatchUpFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    long left, right;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    if (TextGetSelectedLines(SubjectText, &left, &right))
	TextUnsetSelection(SubjectText);
    if (left == right) {
	confirmBox(OK_CATCHUP_MSG, ARTICLE_MODE, ART_CATCHUP, catchUpART);
    }
    else {
	TextSetInsertionPoint(SubjectText, right);
	confirmBox(OK_CATCHUP_CUR_MSG, ARTICLE_MODE, ART_CATCHUP, catchUpPartART);
    }
}

/*
 * called when the user wants to unsubscribe to the current group
 */
/*ARGSUSED*/
void artUnsubFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    confirmBox(OK_TO_UNSUB_MSG, ARTICLE_MODE, ART_UNSUBSCRIBE, unsubscribeART);
}

/*
 * Get selection region, mark articles, redisplay subject window.
 */
static void markFunction _ARGUMENTS((char));

static void markFunction(
			 _ANSIDECL(char,	marker)
			 )
     _KNRDECL(char,	marker)
{
    long left, right;
    long ArtPosition;

    TextDisableRedisplay(SubjectText);

    if (TextGetSelectedOrCurrentLines(SubjectText, &left, &right))
	TextUnsetSelection(SubjectText);
    markArticles(SubjectString, left, right, marker);
    ArtPosition = findArticle(SubjectString, currentArticle(), False);
    TextSetInsertionPoint(SubjectText, ArtPosition);
    updateSubjectWidget(SubjectString, left, right,
			app_resources.subjectScrollBack,
			!app_resources.subjectScrollBack);

    TextEnableRedisplay(SubjectText);

    return;
}

/*
 * Mark selected article(s) as read
 */
/*ARGSUSED*/
void artMarkReadFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char marker = READ_MARKER;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    markFunction(marker);
}

/*
 * Mark selected article(s) as unread
 */
/*ARGSUSED*/
void artMarkUnreadFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char marker = UNREAD_MARKER;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    markFunction(marker);
}

/*
 * allow user to post to the newsgroup currently being read
 */
/*ARGSUSED*/
void artPostFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    post(True);
    
    return;
}


/*
 * allow user to post to the newsgroup currently being read and send
 * the same message via mail
 */
/*ARGSUSED*/
void artPostAndMailFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    post_and_mail(True);
    
    return;
}


/*
 * allow user to send a mail message
 */
/*ARGSUSED*/
void artMailFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    mail();
    
    return;
}


/*
 *
 */
/*ARGSUSED*/
void artSubNextFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    long ArtPosition;
    char *filename, *question;
    char *subject = 0;
    art_num artNum;
    int status;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    TextDisableRedisplay(SubjectText);

    if (selectedArticle() || !cursorOnCurrent()) {
	if (getNearbyArticle(art_FORWARD | art_CURRENT | art_WRAP, &filename,
			     &question, &artNum) == art_DONE)
	    maybeExitArticleMode();
    }
    else {
	ArtPosition = TextGetInsertionPoint(SubjectText);

	if (! SubjectString[ArtPosition])
	    goto done;

	artNum = atol(&SubjectString[ArtPosition+2]);
	if (! (subject = getSubject(artNum))) {
	    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, artNum);
	    status = DONE;
	}
	else {
	    subject = XtNewString(subject);

	    (void) moveCursor(FORWARD, SubjectString, &ArtPosition);

	    status = isNextSubject(subject, ArtPosition, &filename, &question,
				   &artNum);
	}

	switch (status) {
	case ABORT:
	    infoNow(ERROR_SUBJ_ABORT_MSG);
	    goto done;

	case NOCHANGE:
	    (void) sprintf(error_buffer, ERROR_SUBJ_SEARCH_MSG, subject);
	    INFO(error_buffer);
	    foundArticle(filename, question);
	    goto done;

	case DONE:
	    infoNow(ERROR_SUBJ_EXH_MSG);
	    TextSetInsertionPoint(SubjectText, 0);
	    if (getNearbyArticle(art_FORWARD | art_CURRENT | art_WRAP |
				 art_UNREAD, &filename,
				 &question, &artNum) == art_DONE) {
		maybeExitArticleMode();
		goto done;
	    }
	    break;

	case EXIT:
	    maybeExitArticleMode();
	    goto done;
	}
    }

    foundArticle(filename, question);

  done:
    TextEnableRedisplay(SubjectText);
    FREE(subject);
    return;
}

/*
 *
 */
/*ARGSUSED*/
void artSubPrevFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *subject = 0;
    art_num artNum;
    char *filename, *question;
    int status;
    long ArtPosition;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    TextDisableRedisplay(SubjectText);

    if (selectedArticle() || !cursorOnCurrent()) {
	if (getNearbyArticle(art_FORWARD | art_CURRENT | art_WRAP, &filename,
			     &question, &artNum) == art_DONE)
	    maybeExitArticleMode();
    }
    else {
	ArtPosition = TextGetInsertionPoint(SubjectText);

	if (! SubjectString[ArtPosition])
	    goto done;
	
	artNum = atol(&SubjectString[ArtPosition+2]);
	if (! (subject = getSubject(artNum))) {
	    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, artNum);
	    goto done;
	}

	subject = XtNewString(subject);

	status = isPrevSubject(subject, &filename, &question, &artNum);
	switch (status) {
	case ABORT:
	    infoNow(ERROR_SUBJ_ABORT_MSG);
	    goto done;
	case NOCHANGE:
	case CHANGE:
	    (void) sprintf(error_buffer, ERROR_SUBJ_SEARCH_MSG, subject);
	    INFO(error_buffer);
	    break;
	case DONE:
	    infoNow(ERROR_SUBJ_EXH_MSG);
	    goto done;
	case EXIT:
	    maybeExitArticleMode();
	    goto done;
	}
    }

    foundArticle(filename, question);

  done:
    FREE(subject);
    TextEnableRedisplay(SubjectText);
    return;
}

/*
  Mark all articles with the current subject or author as read.
  */
static String artKillSession _ARGUMENTS((int,
					 char * (*) _ARGUMENTS((char *))));

/*ARGSUSED*/
static String artKillSession(field_offset, source_fix_function)
     int field_offset;
     char * (*source_fix_function) _ARGUMENTS((char *));
{
    long left, right;
    char *match_val = 0;
    char *cur_val;
    char *filename, *question;
    art_num artNum;
    struct article *art;

    if (CurrentMode != ARTICLE_MODE) {
	return 0;
    }

    TextDisableRedisplay(SubjectText);

    if (TextGetSelectedOrCurrentLines(SubjectText, &left, &right)) {
	TextUnsetSelection(SubjectText);
	TextSetInsertionPoint(SubjectText, left);
    }
    else
	goto done;
    
    artNum = atol(&SubjectString[left+2]);

    art = artStructGet(CurrentGroup, artNum, False);
    assert(art);

    match_val = *(char **)((char *)art + field_offset);
    if (source_fix_function)
      match_val = (*source_fix_function)(match_val);
    if (! match_val) {
      /* XXX What if it's an optional field which simply doesn't
	 exist in this article?  Right now, that's not a possibility,
	 but later, when we allow other sorts of killing, it could
	 be.
	 */
	mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, artNum);
    }
    else {
#ifdef POSIX_REGEX
	int reRet;
	regex_t reStruct;
#else
	char *reRet;
#endif

	match_val = stringToRegexp(match_val,
				   MAX_KILL_PATTERN_VALUE_LENGTH);

#ifdef POSIX_REGEX
	reRet = regcomp(&reStruct, match_val, REG_NOSUB);
	assert (! reRet);
#else /* ! POSIX_REGEX */
# ifdef SYSV_REGEX
	reRet = regcmp(match_val, 0);
	assert(reRet);
# else /* ! SYSV_REGEX */
	reRet = re_comp(match_val);
	assert(! reRet);
# endif /* SYSV_REGEX */
#endif /* POSIX_REGEX */

	left = 0;
	while (SubjectString[left] != '\0') {
	    if (SubjectString[left] != UNREAD_MARKER) {
		artNum = atol(&SubjectString[left+2]);
		art = artStructGet(CurrentGroup, artNum, False);
		assert(art);
		cur_val = *(char **)((char *)art + field_offset);
		if (! cur_val) {
		  /* XXX See above. */
		    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, artNum);
		    TextRemoveLine(SubjectText, left);
		    removeLine(SubjectString, &left);
		}
		else if
#ifdef POSIX_REGEX
			(! regexec(&reStruct, cur_val, 0, 0, 0))
#else
# ifdef SYSV_REGEX
	                (regex(reRet, cur_val))
# else
	                (re_exec(cur_val))
# endif
#endif
		    {
			markArticleAsRead(artNum);
			(void) markStringRead(SubjectString, left);
			TextInvalidate(SubjectText, SubjectString, left, left + 1);
		    }
	    }
	    if (!moveCursor(FORWARD, SubjectString, &left)) {
		break;
	    }
	}

#ifdef POSIX_REGEX
	regfree(&reStruct);
#else
# ifdef SYSV_REGEX
	XtFree(reRet);
# endif
#endif
    }

    if (getNearbyArticle(art_FORWARD | art_CURRENT | art_WRAP | art_UNREAD,
			 &filename, &question, &artNum)
	== art_DONE) {
	maybeExitArticleMode();
	goto done;
    }

    foundArticle(filename, question);

  done:
    TextEnableRedisplay(SubjectText);
    return match_val;
}

static void do_field_kill _ARGUMENTS((char *, int, char * (*) _ARGUMENTS((char *)),
				      XEvent *, String *, Cardinal *));

static void do_field_kill(field_name, field_offset, source_fix_function,
			  event, string, count)
     char *field_name;
     int field_offset;
     char * (*source_fix_function) _ARGUMENTS((char *));
     XEvent *event;
     String *string;
     Cardinal *count;
{
  struct newsgroup *newsgroup = CurrentGroup;
  int what = KILL_SESSION;
  unsigned int *state = 0;
  char *value_killed;

  if (CurrentMode != ARTICLE_MODE)
    return;

  if (*count) {
    if (! strcasecmp(string[0], "session"))
      what = KILL_SESSION;
    else if (! strcasecmp(string[0], "local"))
      what = KILL_LOCAL;
    else if (! strcasecmp(string[0], "global"))
      what = KILL_GLOBAL;
    else {
      mesgPane(XRN_SERIOUS, 0, UNKNOWN_KILL_TYPE_MSG, string[0],
	       field_name);
      return;
    }
  }
  else if (event) {
    if ((event->type == KeyPress) || (event->type == KeyRelease)) {
      state = &event->xkey.state;
    }
    else if ((event->type == ButtonPress) ||
	     (event->type == ButtonRelease)) {
      state = &event->xbutton.state;
    }
    assert(state);

    if (*state & ShiftMask)
      what = KILL_LOCAL;
    else if (*state & ControlMask)
      what = KILL_GLOBAL;
  }

  value_killed = artKillSession(field_offset, source_fix_function);

  if (what != KILL_SESSION)
    add_kill_entry(newsgroup, what, field_name, value_killed);
}


void artKillSubjectFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
  do_field_kill("Subject", XtOffsetOf(struct article, subject),
		subjectStrip,
		event, string, count);
}

void artKillAuthorFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
  do_field_kill("From", XtOffsetOf(struct article, from), 0,
		event, string, count);
}

#define XRNgotoArticle_ABORT	0
#define XRNgotoArticle_DOIT	1

static Widget GotoArticleBox = (Widget) 0;

/*ARGSUSED*/
void artListOldFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    long ArtPosition;
    int finished;

    xrnBusyCursor();
    TextUnsetSelection(SubjectText);

    abortClear();
    cancelCreate("CancelListOld");

    finished = ((! abortP()) &&
		(fillUpArray(CurrentGroup, firstArticle(), 0,
			     True, False) != ABORT));
    
    if (finished) {
      /*
	This is cheating, perhaps, but it works.  Set the current
	article to the first article, so that getSubjects() will
	retrieve all articles in the newsgroup.
	*/
      FREE(SubjectString);
      FirstListedArticle = firstArticle();
      SubjectString = getSubjects(TextGetColumns(SubjectText), ALL,
				  FirstListedArticle);
    }

    ArtPosition = findArticle(SubjectString, currentArticle(), False);
    TextDisableRedisplay(SubjectText);

    if (finished) {
      TextSetString(SubjectText, SubjectString);
      TextSetInsertionPoint(SubjectText, ArtPosition);
    }
    
    adjustMinMaxLines();
    TextEnableRedisplay(SubjectText);

    if (finished) {
      setButtonSensitive(SubjectButtonBox, "artListOld", False);
    }

    xrnUnbusyCursor();
    cancelDestroy();
    return;
}

/*
 * update the .newsrc file
 */
/*ARGSUSED*/
void artCheckPointFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
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
    int status;
    long artNum;
    String new;
    long ArtPosition;

    if (inCommand) {
	return;
    }
    inCommand = 1;
    xrnBusyCursor();
    TextUnsetSelection(SubjectText);
    if ((int) client_data == XRNgotoArticle_ABORT) {
	PopDownDialog(GotoArticleBox);
	GotoArticleBox = 0;
	xrnUnbusyCursor();
	inCommand = 0;
	return;
    }
    numberstr = GetDialogValue(GotoArticleBox);
    if (numberstr == NIL(char)) {
	mesgPane(XRN_INFO, 0, NO_ART_NUM_MSG);
	PopDownDialog(GotoArticleBox);
	GotoArticleBox = 0;
	xrnUnbusyCursor();
	inCommand = 0;
	return;
    }

    artNum = atol(numberstr);
    if (artNum == 0) {
	mesgPane(XRN_SERIOUS, 0, BAD_ART_NUM_MSG, numberstr);
	PopDownDialog(GotoArticleBox);
	GotoArticleBox = 0;
	xrnUnbusyCursor();
	inCommand = 0;
	return;
    }
    
    status = moveToArticle(CurrentGroup, artNum, &filename, &question);

    switch (status) {

    case NOMATCH:
    case ERROR:
	mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, artNum);
	break;

    case MATCH:
	TextDisableRedisplay(SubjectText);

	if ((ArtPosition = findArticle(SubjectString, artNum, True)) == -1) {
	  FirstListedArticle = MIN(FirstListedArticle, artNum);
	  new = getSubjects(TextGetColumns(SubjectText), ALL, FirstListedArticle);
	  FREE(SubjectString);
	  SubjectString = new;
	  TextSetString(SubjectText, SubjectString);
	}

	ArtPosition = findArticle(SubjectString, artNum, False);

	TextSetInsertionPoint(SubjectText, ArtPosition);

	foundArticle(filename, question);

	TextEnableRedisplay(SubjectText);

	break;
    }

    PopDownDialog(GotoArticleBox);
    GotoArticleBox = 0;
    xrnUnbusyCursor();
    inCommand = 0;
    return;
}

/*ARGSUSED*/
void artGotoArticleFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static struct DialogArg args[] = {
      {ABORT_STRING,   gotoArticleHandler, (XtPointer) XRNgotoArticle_ABORT},
      {DOIT_STRING, gotoArticleHandler, (XtPointer) XRNgotoArticle_DOIT},
    };
    
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    if (GotoArticleBox == (Widget) 0) {
      GotoArticleBox = CreateDialog(TopLevel, ARTICLE_NUMBER_MSG,
				  DIALOG_TEXT, args, XtNumber(args));
    }
    PopUpDialog(GotoArticleBox);
    return;
}

static int subjectSearch _ARGUMENTS((int, long *, char *, char **, char **,
				     art_num *));

static int subjectSearch(dir, position, expr, file, ques, artNum)
    int dir;			     /* direction, either FORWARD or BACK */
    long *position;	     /* cursor position */
    char *expr;			     /* regular expression to search for */
    char **file, **ques;	     /* filename and status line for
					new article */
    art_num *artNum;		     /* number of new article */
{
#ifdef POSIX_REGEX
    regex_t reStruct;
    int reRet;
#else
# ifdef SYSV_REGEX
    char *reRet = 0;
# else
    char *reRet;	/* returned by re_comp/regcmp */
# endif
#endif
    char *newsubject;		/* subject of current line */
    char *oldString = 0, *newString; /* strings used to build up new text string */
    char *newLine;
    int ret;
    int line_length = TextGetColumns(SubjectText);

    assert(expr);

    if (
#ifdef POSIX_REGEX
	(reRet = regcomp(&reStruct, expr, REG_NOSUB))
#else
# ifdef SYSV_REGEX
	! (reRet = regcmp(expr, NULL))
# else
	(reRet = re_comp(expr))
# endif
#endif
	) {
      /* bad regular expression */
#ifdef SYSV_REGEX
      mesgPane(XRN_SERIOUS, 0, UNKNOWN_REGEXP_ERROR_MSG, expr);
#else
# ifdef POSIX_REGEX
      regerror(reRet, &reStruct, error_buffer,
	       sizeof(error_buffer));
# endif
      mesgPane(XRN_SERIOUS, 0, KNOWN_REGEXP_ERROR_MSG, expr,
# ifdef POSIX_REGEX
	       error_buffer
# else
	       reRet
# endif /* POSIX_REGEX */
	       );
#endif /* SYSV_REGEX */
      return ERROR;
    }

    abortClear();
    cancelCreate(0);

    if (dir == FORWARD) {
	for (;;) {
	    if (abortP()) {
		cancelDestroy();
		ret = ABORT;
		goto done;
	    }
	    if (SubjectString[*position] == '\0') {
		cancelDestroy();
		if (*position == 0) {
		    /* the string is null, no more articles are left */
		    ret = EXIT;
		    goto done;
		}
		ret = NOMATCH;
		goto done;
	    }
	    (void) moveCursor(FORWARD, SubjectString, position);
	    if (SubjectString[*position] == '\0') {

		/* reached end of string */
		cancelDestroy();
		ret = NOMATCH;
		goto done;
	    }
	    *artNum = atol(&SubjectString[*position + 2]);
	    newsubject = getSubject(*artNum);
	    if (! newsubject) {
		mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
		TextRemoveLine(SubjectText, *position);
		removeLine(SubjectString, position);
		continue;
	    }

#ifdef POSIX_REGEX
	    if (! regexec(&reStruct, newsubject, 0, 0, 0))
#else
# ifdef SYSV_REGEX
	    if (regex(reRet, newsubject) != NULL)
# else /* ! SYSV_REGEX */
	    if (re_exec(newsubject))
# endif /* SYSV_REGEX */
#endif /* POSIC_REGEX */
            {
		/* found a match to the regular expression */

		gotoArticle(*artNum);
		if (getArticle(file, ques) != XRN_OKAY) {
		    /* the matching article was invalid */

		    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);

		    TextRemoveLine(SubjectText, *position);
		    removeLine(SubjectString, position);
		    continue;
		}
		cancelDestroy();
		TextSetInsertionPoint(SubjectText, *position);
		ret = MATCH;
		goto done;
	    }
	}
    } else {
	startSearch();
	for (;;) {
	    if (abortP()) {

		/* reset pointers back to where we began, since the */
		/* search was aborted */

		failedSearch();
		cancelDestroy();
		FREE(oldString);
		ret = ABORT;
		goto done;
	    }
	    if ((*position == 0) && (SubjectString[*position] == '\0')) {

		/* no more articles remain, return to Newgroup mode */
		FREE(oldString);
		cancelDestroy();
		ret = EXIT;
		goto done;
	    }
	    if (*position != 0) {

		/* we are still within the subject list */

		(void) moveCursor(BACK, SubjectString, position);
		*artNum = atol(&SubjectString[*position + 2]);
		newsubject = getSubject(*artNum);
		if (! newsubject) {
		    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);

		    TextRemoveLine(SubjectText, *position);
		    removeLine(SubjectString, position);
		    continue;
		}

#ifdef POSIX_REGEX
		if (! regexec(&reStruct, newsubject, 0, 0, 0))
#else
# ifdef SYSV_REGEX
		if (regex(reRet, newsubject) != NULL)
# else
		if (re_exec(newsubject))
# endif
#endif
                {
		    /* an article matching the regular expression was found */

		    gotoArticle(*artNum);
		    if (getArticle(file, ques) != XRN_OKAY) {
			/* article is invalid, remove it from the text string*/

			mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);

			TextRemoveLine(SubjectText, *position);
			removeLine(SubjectString, position);
			continue;
		    }
		    cancelDestroy();
		    TextSetInsertionPoint(SubjectText, *position);
		    ret = MATCH;
		    goto done;
		}
	    } else {

		/* must query the news server for articles not shown */
		/* on the current subject screen */

		if ((newLine = getPrevSubject(line_length)) == NIL(char)) {
		    
		    /* all articles have been exhausted, reset variables */
		    /* to what they were before the search was started */

		    failedSearch();
		    cancelDestroy();
		    FREE(oldString);
		    ret = NOMATCH;
		    goto done;
		}
		*artNum = atol(&newLine[2]);
		newsubject = getSubject(*artNum);
		if (! newsubject) {
		    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
		    continue;
		}
		if (oldString != NIL(char)) {

		    /* add the newest subject line (newLine) to the */
		    /* list of new subject lines (oldString) we are */
		    /* building up.  Put the result in newString.   */

		    newString = ARRAYALLOC(char, (utStrlen(oldString) + utStrlen(newLine) + 1));
		    (void) strcpy(newString, newLine);
		    (void) strcat(newString, oldString);
		    FREE(oldString);
		} else {

		    /* the first new subject line has been obtained, */
		    /* allocate space and save it */

		    newString = XtNewString(newLine);
		}

#ifdef POSIX_REGEX
		if (! regexec(&reStruct, newsubject, 0, 0, 0))
#else
# ifdef SYSV_REGEX
		if (regex(reRet, newsubject) != NULL)
# else
		if (re_exec(newsubject))
# endif
#endif
                {
		    gotoArticle(*artNum);
		    if (getArticle(file, ques) != XRN_OKAY) {
			mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
			removeLine(newString, 0);
			continue;
		    }
		    TextReplace(SubjectText, newString, strlen(newString), 0, 0);
		    cancelDestroy();
		    oldString = newString;
		    newString = XtMalloc(strlen(SubjectString) +
					 strlen(oldString) + 1);
		    (void) strcpy(newString, oldString);
		    FREE(oldString);
		    (void) strcat(newString, SubjectString);
		    FREE(SubjectString);
		    SubjectString = newString;
		    FirstListedArticle = *artNum;
		    *position = 0;
		    TextSetInsertionPoint(SubjectText, *position);
		    ret = MATCH;
		    goto done;
		}
		oldString = newString;
		continue;
	    }
	}
    }

  done:
#ifdef POSIX_REGEX
    regfree(&reStruct);
#else
# ifdef SYSV_REGEX
    FREE(reRet);
# endif /* SYSV_REGEX */
#endif /* POSIX_REGEX */
    return ret;
}


#define XRNsubSearch_ABORT 0
#define XRNsubSearch_FORWARD 1
#define XRNsubSearch_BACK 2

static Widget SubSearchBox = (Widget) 0;

static void doSubSearch _ARGUMENTS((String, int));

static void doSubSearch(regexp, direction)
    String regexp;
    int direction;
{
    long ArtPosition;
    char *filename, *question;
    art_num artNum;
    int status;

    TextDisableRedisplay(SubjectText);

    TextUnsetSelection(SubjectText);
    ArtPosition = TextGetInsertionPoint(SubjectText);

    status = subjectSearch(direction, &ArtPosition,
			   regexp, &filename, &question, &artNum);
    switch (status) {
    case ABORT:
	infoNow(ERROR_SUBJ_ABORT_MSG);
	break;

    case NOMATCH:
	(void) sprintf(error_buffer, ERROR_SUBJ_EXPR_MSG,
		       regexp);
	infoNow(error_buffer);
    case ERROR:
	break;

    case MATCH:
	(void) sprintf(error_buffer, ERROR_SEARCH_MSG,
		       regexp);
	infoNow(error_buffer);
	foundArticle(filename, question);
	break;

    case EXIT:
	maybeExitArticleMode();
	break;
    }

    TextEnableRedisplay(SubjectText);
}



static void subSearchHandler _ARGUMENTS((Widget, XtPointer, XtPointer));

/*ARGSUSED*/
static void subSearchHandler(widget, client_data, call_data)
    Widget widget;
    XtPointer client_data;
    XtPointer call_data;
{
    char *regexpr;
    int direction;
    Boolean do_search = True;

    if (inCommand) {
	return;
    }
    inCommand = 1;
    xrnBusyCursor();

    if ((int) client_data == XRNsubSearch_ABORT) {
	do_search = False;
    }
    else {
	regexpr = GetDialogValue(SubSearchBox);
	if (*regexpr == 0) {
	    if (LastRegexp == NIL(char)) {
		mesgPane(XRN_INFO, 0, NO_PREV_REGEXP_MSG);
		do_search = False;
	    }
	    else {
		regexpr = LastRegexp;
	    }
	} else {
	    FREE(LastRegexp);
	    LastRegexp = XtNewString(regexpr);
	    setButtonSensitive(SubjectButtonBox, "artContinue", True);
	}
    }

    if (SubSearchBox)
	PopDownDialog(SubSearchBox);
    SubSearchBox = 0;

    if (do_search) {
	direction = ((int) client_data == XRNsubSearch_FORWARD) ?
	    FORWARD : BACK;
	LastSearch = direction;
	doSubSearch(LastRegexp, direction);
    }

    inCommand = 0;
    xrnUnbusyCursor();
    return;
}

/*ARGSUSED*/
void artSubSearchFunction(widget, event, string, count)
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
    
    if (CurrentMode != ARTICLE_MODE) {
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
void artContinueFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE)
	return;

    if (!LastRegexp) {
	mesgPane(XRN_INFO, 0, NO_PREV_REGEXP_MSG);
	return;
    }

    doSubSearch(LastRegexp, LastSearch);
}
	
/*
 * Display the article accessed before the current one
 */
/*ARGSUSED*/
void artLastFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *filename, *question;
    long ArtPosition;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    if (PrevArticle == 0) {
	mesgPane(XRN_INFO, 0, NO_PREV_ART_MSG);
	return;
    }

    TextDisableRedisplay(SubjectText);

    ArtPosition = findArticle(SubjectString, PrevArticle, False);
    gotoArticle(PrevArticle);
    if (getArticle(&filename, &question) != XRN_OKAY) {
	mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, PrevArticle);
	TextRemoveLine(SubjectText, ArtPosition);
	removeLine(SubjectString, &ArtPosition);
    } else {
	TextSetInsertionPoint(SubjectText, ArtPosition);
	foundArticle(filename, question);
    }

    TextEnableRedisplay(SubjectText);
}


/*
 * Exit from the current newsgroup, marking all articles as
 * unread
 */
/*ARGSUSED*/
void artExitFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    groupSnapshotRestore(CurrentGroup);
    exitArticleMode();
}


#define XRNsave_ABORT          0
#define XRNsave_SAVE           1

static String articleIterator(start, left)
    Boolean start;
    long *left;
{
    return anyIterator(SubjectText, SubjectString, False, start, False, left);
}

static void doSave(template, printing)
    String template;
    Boolean printing;
{
    String name;
    char buffer[1024];
    long left, top WALL(= 0);
    art_num article;

    TextDisableRedisplay(SubjectText);

    if (! app_resources.subjectScrollBack)
	top = TextGetTopPosition(SubjectText);

    if (articleIterator(True, &left)) {
	while ((name = articleIterator(False, &left))) {
	    article = atol(name);
	    (void) sprintf(buffer, template, article);
	    if (saveArticleByNumber(buffer, article, printing)) {
		SubjectString[left + 1] = (printing ? PRINTED_MARKER : SAVED_MARKER);
		TextInvalidate(SubjectText, SubjectString,
				 left + 1, left + 2);
		
	    }
	}
	left = findArticle(SubjectString, currentArticle(), False);
	TextSetInsertionPoint(SubjectText, left);
	if (app_resources.subjectScrollBack)
	    adjustMinMaxLines();
	else
	    TextSetTopPosition(SubjectText, top);
    }

    TextEnableRedisplay(SubjectText);
}


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
    String template;

    if (inCommand) {
	return;
    }
    inCommand = 1;
    xrnBusyCursor();

    if ((int) client_data != XRNsave_ABORT) {
	template = GetDialogValue(SaveBox);
	doSave(template, False);
	if (SaveString && (SaveString != app_resources.saveString)) {
	    XtFree(SaveString);
	}
	SaveString = XtNewString(template);
    }

    PopDownDialog(SaveBox);
    SaveBox = 0;
    xrnUnbusyCursor();
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
void artSaveFunction(widget, ev, params, num_params)
    Widget widget;
    XEvent *ev;
    String *params;
    Cardinal *num_params;
{
    static struct DialogArg args[] = {
      {ABORT_STRING, saveHandler, (XtPointer) XRNsave_ABORT},
      {SAVE_STRING,  saveHandler, (XtPointer) XRNsave_SAVE},
    };

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    
    if (num_params && *num_params == 1) {
	doSave(params[0], False);
    }
    else {
	if (! SaveBox) {
	    if (!SaveString && app_resources.saveString) {
		SaveString = XtNewString(app_resources.saveString);
	    }
	    SaveBox = CreateDialog(TopLevel, ASK_SAVEBOX_MSG,
				   SaveString == NULL ? DIALOG_TEXT
				   : SaveString, args, XtNumber(args));
	}
	PopUpDialog(SaveBox);
    }
}

#ifdef XLATE

/*
 * translate an article
 */
/*ARGSUSED*/
void artXlateFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *filename, *question;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    if (toggleXlation(&filename, &question) == XRN_OKAY) {
	TextClear(ArticleText);
	redrawArticleWidget(filename, question);
    }
    return;
}

#endif /* XLATE */


/*ARGSUSED*/
void artPrintFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char buffer[1024];

    if (CurrentMode != ARTICLE_MODE)
	return;

    (void) sprintf(buffer, "| %s", app_resources.printCommand);
    doSave(buffer, True);
}


/*
 * Allow user to post a reply to the currently posted article
 */
/*ARGSUSED*/
void artReplyFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    reply();
    return;
}

/*
 * Allow user to forward an article to a user(s)
 */
/*ARGSUSED*/
void artForwardFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    forward();
    return;
}

/*
 * Allow user to gripe
 */
/*ARGSUSED*/
void artGripeFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    gripe();
    return;
}

/*
 * Allow user to post a followup to the currently displayed article
 */
/*ARGSUSED*/
void artFollowupFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
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
void artFollowupAndReplyFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    followup_and_reply();
    return;
}

/*
 * Allow user to cancel the currently displayed article
 */
/*ARGSUSED*/
void artCancelFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    cancelArticle();
    return;
}

/*
 * decrypt a joke
 */
/*ARGSUSED*/
void artRot13Function(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *filename, *question;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    if (toggleRotation(&filename, &question) == XRN_OKAY) {
	TextClear(ArticleText);
	redrawArticleWidget(filename, question);
    }
    return;
}

/*ARGSUSED*/
void artHeaderFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    char *filename, *question;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    if (toggleHeaders(&filename, &question) == XRN_OKAY) {
	TextClear(ArticleText);
	redrawArticleWidget(filename, question);
    }
    return;
}

static void resizeSubjectText _ARGUMENTS((Widget, XtPointer, XEvent *,
					  Boolean *));

static void resizeSubjectText(widget, client_data, event,
			      continue_to_dispatch)
     Widget widget;
     XtPointer client_data;
     XEvent *event;
     Boolean *continue_to_dispatch;
{
  long ArtPosition;

  if (event->type == ConfigureNotify) {
    TextDisableRedisplay(SubjectText);
    FREE(SubjectString);
    SubjectString = getSubjects(TextGetColumns(SubjectText), ALL, 
				FirstListedArticle);
    TextSetString(SubjectText, SubjectString);

    ArtPosition = findArticle(SubjectString, CurrentArticle, False);

    TextSetInsertionPoint(SubjectText, ArtPosition);

    adjustMinMaxLines();

    TextEnableRedisplay(SubjectText);
  }
}

void displayArticleWidgets()
{
    ArticleNewsGroupsString = getNewsgroupString();

    if (! ArticleFrame) {
	Dimension height;

	ArticleFrame = XtCreateManagedWidget("artFrame", panedWidgetClass,
					     TopLevel, 0, 0);

	XawPanedSetRefigureMode(ArticleFrame, False);

	/*
	  This Box widget and the one below are managed only after
	  their children have been placed in them because there is a
	  bug in the Xaw Box widget (as of 05/06/95) which prevents it
	  from doing geometry management properly when children are
	  added to it.  A patch has been submitted to the X
	  Consortium, but it'll be a long time before it's in
	  widespread use.
	  */
#define TOP_BUTTON_BOX() {\
	  SubjectButtonBox = ButtonBoxCreate("buttons", ArticleFrame);\
	  doButtons(app_resources.artButtonList, SubjectButtonBox,\
		    ArtButtonList, &ArtButtonListCount, TOP);\
	  XtManageChild(SubjectButtonBox);\
	}

#define TOP_INFO_LINE() {\
	  SubjectInfoLine = InfoLineCreate("info", 0, ArticleFrame);\
	}

#define BOTTOM_BUTTON_BOX() {\
	  ArtSpecButtonBox = ButtonBoxCreate("artButtons", ArticleFrame);\
	  doButtons(app_resources.artSpecButtonList, ArtSpecButtonBox,\
		    ArtSpecButtonList, &ArtSpecButtonListCount, BOTTOM);\
	  XtManageChild(ArtSpecButtonBox);\
	}

#define BOTTOM_INFO_LINE() {\
	  ArticleInfoLine = InfoLineCreate("artInfo", 0, ArticleFrame);\
	}
	
	if (app_resources.buttonsOnTop) {
	  TOP_BUTTON_BOX();
	  TOP_INFO_LINE();
	}
	
	SubjectText = TextCreate("subjects", True, ArticleFrame);

	if (app_resources.buttonsOnTop) {
	  BOTTOM_BUTTON_BOX();
	  BOTTOM_INFO_LINE();
	}
	else {
	  TOP_INFO_LINE();
	  TOP_BUTTON_BOX();
	}
	
	ArticleText = TextCreate("text", True, ArticleFrame);

	if (! app_resources.buttonsOnTop) {
	  BOTTOM_INFO_LINE();
	  BOTTOM_BUTTON_BOX();
	}

#undef TOP_BUTTON_BOX
#undef TOP_INFO_LINE
#undef BOTTOM_BUTTON_BOX
#undef BUTTOM_INFO_LINE

	TextSetLineSelections(SubjectText);
	TextDisableWordWrap(SubjectText);
	XawPanedAllowResize(SubjectText, True);
	TextSetLines(SubjectText, app_resources.topLines);
	XawPanedAllowResize(SubjectText, False);
	XtVaGetValues(SubjectText, XtNheight, &height, 0);
	XtVaSetValues(SubjectText, XtNpreferredPaneSize, height, 0);

	TopInfoLine = SubjectInfoLine;

	setButtonSensitive(SubjectButtonBox, "artPost", PostingAllowed);
	setButtonSensitive(SubjectButtonBox, "artPostAndMail", PostingAllowed);
	
	BottomInfoLine = ArticleInfoLine;

	setButtonSensitive(ArtSpecButtonBox, "artFollowup", PostingAllowed);
	setButtonSensitive(ArtSpecButtonBox, "artFollowupAndReply",
			   PostingAllowed);

	XawPanedSetRefigureMode(ArticleFrame, True);

	XtInstallAccelerators(SubjectText, ArticleText);
	XtSetKeyboardFocus(ArticleFrame, SubjectText);

	XtAddEventHandler(SubjectText, StructureNotifyMask, FALSE, 
			  resizeSubjectText, NULL);
    }
    else {
	TopInfoLine = SubjectInfoLine;
	BottomInfoLine = ArticleInfoLine;
	XtManageChild(ArticleFrame);
    }
}

void hideArticleWidgets()
{
    FREE(ArticleNewsGroupsString);
    XtUnmanageChild(ArticleFrame);
}

void artDoTheRightThing(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (event &&
	(event->type == ButtonPress || event->type == ButtonRelease)) {
	artNextFunction(widget, event, string, count);
    }
    else if (!app_resources.pageArticles) {
	if (app_resources.subjectRead == False) {
	    artNextUnreadFunction(widget, event, string, count);
	} else {
	    artSubNextFunction(widget, event, string, count);
	}
    } else {
	if (TextLastPage(ArticleText)) {
	  next_article:
	    if (app_resources.subjectRead == False) {
		artNextUnreadFunction(widget, event, string, count);
	    } else {
		artSubNextFunction(widget, event, string, count);
	    }
	}
	else {
	    artScrollFunction(widget, event, string, count);
	    if (TextPastLastPage(ArticleText))
		goto next_article;
	}
    }
}

/*
  Move the cursor up a line without changing the current article.
  */
void artUpFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE)
	return;

    TextDisableRedisplay(SubjectText);
    TextMoveLine(SubjectText, BACK);
    adjustMinMaxLines();
    TextEnableRedisplay(SubjectText);
}

/*
  Move the cursor down a line without changing the current article.
  */
void artDownFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    if (CurrentMode != ARTICLE_MODE)
	return;

    TextDisableRedisplay(SubjectText);
    TextMoveLine(SubjectText, FORWARD);
    adjustMinMaxLines();
    TextEnableRedisplay(SubjectText);
}

void resetArticleNewsgroupsList()
{
  FREE(ArticleNewsGroupsString);
}

/*
 Move the cursor to the position of the article "num".  If if the
 article is not found, exit XRN with an error if allow_failure is
 False, or return -1 otherwise.
 */
static long findArticle(
			_ANSIDECL(char *,	tstring),
			_ANSIDECL(art_num,	num),
			_ANSIDECL(Boolean,	allow_failure)
			)
     _KNRDECL(char *,	tstring)
     _KNRDECL(art_num,	num)
     _KNRDECL(Boolean,	allow_failure)
{
    long artNum;		/* number of current article */
    long position = 0;
    long pos;

    while (TRUE) {
      pos = position + 1;
      /* move over S[aved] / P[rinted] marking */
      if ((tstring[pos] == SAVED_MARKER) || (tstring[pos] == PRINTED_MARKER)) {
	pos++;
      }
      artNum = atol(&tstring[pos]);
      if (artNum == num)
	break;
      if (!moveCursor(FORWARD, tstring, &position)) {
	if (allow_failure)
	  return -1;
	else
	  ehErrorExitXRN( ERROR_FINDARTICLE_MSG );
      }
    }
    return position;
}
