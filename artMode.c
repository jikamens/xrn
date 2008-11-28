#include <assert.h>

#include "artMode.h"
#include "buttons.h"
#include "news.h"
#include "butexpl.h"
#include "Text.h"
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
#include "ngMode.h"
#include "allMode.h"
#include "snapshot.h"
#include "utils.h"

static int getPrevious _ARGUMENTS((art_num *));

/*
  The string to which the list of newsgroups is moved when switching
  from newsgroup mode to article mode, so that artNextGroupFunction
  can behave consistently with what was displayed when in newsgroup
  mode.
*/
char *ArticleNewsGroupsString = 0;

static String SubjectString;
static art_num FirstListedArticle;

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

XtActionsRec ArtActions[] = {
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

int ArtActionsCount = XtNumber(ArtActions);

ButtonList ArtButtonList[] = {
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
};

int ArtButtonListCount = XtNumber(ArtButtonList);

ButtonList ArtSpecButtonList[] = {
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
};

int ArtSpecButtonListCount = XtNumber(ArtSpecButtonList);

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
					    long, /* Boolean */ int));
    
static void updateSubjectWidget(string, left, right, update_top)
    String string;
    long left, right;
    Boolean update_top;
{
    Boolean disable = False;

    if ((left != right) || update_top)
	disable = True;

    if (disable)
	TextDisableRedisplay(Text);

    if (left == right)
	TextSetString(Text, string);
    else {
	long point = TextGetInsertionPoint(Text);
	TextInvalidate(Text, string, left, right);
	TextSetInsertionPoint(Text, point);
    }

    if (update_top)
	adjustMinMaxLines(SubjectString);

    if (disable)
	TextEnableRedisplay(Text);
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
    long ArtPosition = TextGetInsertionPoint(Text);
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
	    TextRemoveLine(Text, ArtPosition);
	    removeLine(SubjectString, &ArtPosition);
	    if (status & art_FORWARD)
		move = False;
	    continue;
	}
	ret = art_CHANGE;
	break;
    }

    TextSetInsertionPoint(Text, ArtPosition);
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
    long ArtPosition = TextGetInsertionPoint(Text);

    char *newsubject;
    char *newLine;
    char *tmp;
    char *newSubjects = 0;
    int count = 0;
    int ret;

    startSearch();
    abortClear();
    
    for (;;) {
	count++;

	if (count == app_resources.cancelCount) {
	    cancelCreate();
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
	    newsubject = getSubject(*artNum);
	    if (utSubjectCompare(newsubject, subject) == 0) {
		gotoArticle(*artNum);
		if (getArticle(filename, question) != XRN_OKAY) {
		    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
		    TextRemoveLine(Text, ArtPosition);
		    removeLine(SubjectString, &ArtPosition);
		    continue;
		}
		cancelDestroy();
		TextSetInsertionPoint(Text, ArtPosition);
		ret = NOCHANGE;
		goto done;
	    }
	    continue;
	} else {
	    if ((newLine = getPrevSubject()) == NIL(char)) {
		failedSearch();
		cancelDestroy();
		ret = DONE;
		FREE(newSubjects);
		goto done;
	    }
	    *artNum = atol(&newLine[2]);
	    newsubject = getSubject(*artNum);
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

		    TextDisableRedisplay(Text);
		    TextSetString(Text, tmp);

		    FREE(SubjectString);
		    SubjectString = tmp;

		    FirstListedArticle = *artNum;
		    TextSetInsertionPoint(Text, ArtPosition);
		    TextEnableRedisplay(Text);

		    cancelDestroy();
		    ret = CHANGE;
		    goto done;
		}
	    }
	}
    }
  done:
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
	    cancelCreate();
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
	newsubject = getSubject(*artNum);
	if (utSubjectCompare(newsubject, subject) == 0) {
	    gotoArticle(*artNum);
	    if (getArticle(filename, question) != XRN_OKAY) {
		mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
		TextRemoveLine(Text, ArtPosition);
		removeLine(SubjectString, &ArtPosition);
		continue;
	    }
	    cancelDestroy();
	    TextSetInsertionPoint(Text, ArtPosition);
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

    if ((newLine = getPrevSubject()) != NIL(char)) {
	newString = XtMalloc(utStrlen(SubjectString) + utStrlen(newLine) + 1);
	(void) strcpy(newString, newLine);
	(void) strcat(newString, SubjectString);
	FREE(SubjectString);
	SubjectString = newString;

	TextSetString(Text, SubjectString);
	*artNum = atol(newLine + 2);
	FirstListedArticle = *artNum;
	return TRUE;
    }
    
    return FALSE;
}


/*
 */
static Boolean selectedArticle _ARGUMENTS((int));

static Boolean selectedArticle(status)
    int status;
{
    long left, right;

    if (TextGetSelectedLines(Text, &left, &right)) {
	TextDisableRedisplay(Text);
	TextUnsetSelection(Text);
	TextSetInsertionPoint(Text, left);
	TextEnableRedisplay(Text);
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
    SubjectString = getSubjects(UNREAD, FirstListedArticle);

    if (! (SubjectString && *SubjectString)) {
	/*
	  We must be entering a newsgroup with no unread articles --
	  display the last read article.
	  */
	FREE(SubjectString);
	SubjectString = getSubjects(ALL, FirstListedArticle);
    }

    if (! (SubjectString && *SubjectString)) {
	mesgPane(XRN_INFO, 0, PROBABLY_EXPIRED_MSG, CurrentGroup->name);
	/*
	 * the sources and strings have been destroyed at this point
	 * have to recreate them - the redraw routines check the mode
	 * so we can call all of them and only the one that is for the
	 * current mode will do something
	 */
	redrawAllWidget();
	redrawNewsgroupTextWidget(0, True, True);
	FREE(SubjectString);
	return BAD_GROUP;
    }

    /* get rid of previous groups save file string */
    if (SaveString && app_resources.resetSave) {
	if (SaveString != app_resources.saveString) {
		XtFree(SaveString);
	}
	SaveString = XtNewString(app_resources.saveString);
    }	

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
    setTopInfoLine(OPEARATION_APPLY_CURSOR_MSG);

    TextDisableRedisplay(Text);

    /* get and display the article */
    updateSubjectWidget(SubjectString, 0, 0, True);
    ArtStatus = art_FORWARD | art_CURRENT | art_WRAP;
    artNextFunction(0, 0, 0, 0);

    TextEnableRedisplay(Text);

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
	/* force the screen to update before prefetching */
	/* Currently disabled because of a bug in the Xaw Text widget. */
	/* xthHandlePendingExposeEvents(); */
	
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

    setBottomInfoLine("");
    
    releaseNewsgroupResources(CurrentGroup);
    if (app_resources.updateNewsrc == TRUE) {
	while (!updatenewsrc())
          ehErrorRetryXRN(ERROR_CANT_UPDATE_NEWSRC_MSG, True);
    }

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
    long ArtPosition = TextGetInsertionPoint(Text);

    PrevArticle = CurrentArticle;
    gotoArticle(markStringRead(SubjectString, ArtPosition));
    CurrentArticle = currentArticle();

    updateSubjectWidget(SubjectString, ArtPosition, ArtPosition + 1, True);

    /*
      This is necessary because of a bug in the Xaw Text widget -- it
      doesn't redisplay properly if expose events are processed while
      redisplay is disabled, and redrawArticleWidget() causes pending
      expose events to be processed.
      */
    TextDisplay(Text);

    redrawArticleWidget(file, ques);
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
    long ArtPosition = TextGetInsertionPoint(Text);

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
    updateSubjectWidget(SubjectString, 0, ArtPosition, True);
    if (getNearbyArticle(art_FORWARD | art_CURRENT | art_WRAP | art_UNREAD,
			 &filename, &question, &artNum) ==
	art_DONE) {
	exitArticleMode();
	return;
    }
    foundArticle(filename, question);
    
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

    TextDisableRedisplay(Text);

    if (selectedArticle(ArtStatus))
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
    TextEnableRedisplay(Text);
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
    TextScrollPage(Text, FORWARD);
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
    TextScrollPage(Text, BACK);
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

    TextDisableRedisplay(Text);

    ArtStatus = art_BACKWARD | art_WRAP;

    if (selectedArticle(ArtStatus))
	ArtStatus |= art_CURRENT;

    if (getNearbyArticle(ArtStatus, &filename, &question, &artNum) ==
	art_DONE) {
	maybeExitArticleMode();
	goto done;
    }
    foundArticle(filename, question);

  done:
    TextEnableRedisplay(Text);
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
    char name[GROUP_NAME_SIZE];
    struct newsgroup *LastGroupStruct = CurrentGroup;
    int ret;
    char *p;
    int mesg_name = newMesgPaneName();
    long GroupPosition;

    PrevArticle = CurrentArticle = 0;

    if (! ArticleNewsGroupsString)
	ArticleNewsGroupsString = unreadGroups(NewsgroupDisplayMode);

    /* Use name instead of CurrentIndexGroup to prevent CurrentIndexGroup
       from being overwritten by getNearbyNewsgroup. */
    (void) strcpy(name, CurrentIndexGroup);
    GroupPosition = getNearbyNewsgroup(ArticleNewsGroupsString, name);

    while (1) {
	currentGroup(CurrentMode, ArticleNewsGroupsString, name, GroupPosition);
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
	    (void) strcpy(CurrentIndexGroup, name);
	    continue;
	}
	else if (ret != GOOD_GROUP) {
	    mesgPane(XRN_SERIOUS, mesg_name, UNKNOWN_ENTER_NG_RESPONSE_MSG,
		     ret);
	    (void) strcpy(CurrentIndexGroup, name);
	    continue;
	}

	 if (switchToArticleMode() == GOOD_GROUP) {
	      releaseNewsgroupResources(LastGroupStruct);
	      (void) strcpy(LastGroup, name);
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

    if (TextGetSelectedLines(Text, &left, &right))
	TextUnsetSelection(Text);
    if (left == right) {
	confirmBox(OK_CATCHUP_MSG, ARTICLE_MODE, ART_CATCHUP, catchUpART);
    }
    else {
	TextSetInsertionPoint(Text, right);
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
static void markFunction _ARGUMENTS((/* char */ int));

static void markFunction(marker)
    char marker;
{
    long left, right;
    long ArtPosition = 0;

    TextDisableRedisplay(Text);

    if (TextGetSelectedOrCurrentLines(Text, &left, &right))
	TextUnsetSelection(Text);
    markArticles(SubjectString, left, right, marker);
    findArticle(SubjectString, currentArticle(), &ArtPosition);
    TextSetInsertionPoint(Text, ArtPosition);
    updateSubjectWidget(SubjectString, left, right, True);

    TextEnableRedisplay(Text);

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
    post(1);
    
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
    long left, right, ArtPosition;
    char *filename, *question;
    char *subject = 0;
    art_num artNum;
    int status;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    TextDisableRedisplay(Text);

    if (TextGetSelectedLines(Text, &left, &right)) {
	TextUnsetSelection(Text);
	TextSetInsertionPoint(Text, left);
	if (getNearbyArticle(art_FORWARD | art_CURRENT, &filename,
			      &question, &artNum) == art_DONE)
	    maybeExitArticleMode();
    }
    else {
	ArtPosition = TextGetInsertionPoint(Text);

	if (! SubjectString[ArtPosition])
	    goto done;

	artNum = atol(&SubjectString[ArtPosition+2]);
	subject = getSubject(artNum);
	subject = XtNewString(subject);

	(void) moveCursor(FORWARD, SubjectString, &ArtPosition);

	status = isNextSubject(subject, ArtPosition, &filename, &question,
			       &artNum);
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
	    TextSetInsertionPoint(Text, 0);
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
    TextEnableRedisplay(Text);
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
    long left, right;
    char *subject = 0;
    art_num artNum;
    char *filename, *question;
    int status;
    long ArtPosition;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    TextDisableRedisplay(Text);

    if (TextGetSelectedLines(Text, &left, &right)) {
	TextUnsetSelection(Text);
	TextSetInsertionPoint(Text, left);
	if (getNearbyArticle(art_FORWARD | art_CURRENT, &filename,
			     &question, &artNum) == art_DONE)
	    maybeExitArticleMode();
    }
    else {
	ArtPosition = TextGetInsertionPoint(Text);

	if (! SubjectString[ArtPosition])
	    goto done;
	
	artNum = atol(&SubjectString[ArtPosition+2]);
	subject = getSubject(artNum);
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
    TextEnableRedisplay(Text);
    return;
}

/*
  Mark all articles with the current subject or author as read.
  */
static String _artKillSession _ARGUMENTS((Widget, /* Boolean */ int));

/*ARGSUSED*/
static String _artKillSession(widget, author)
    Widget widget;
    Boolean author;
{
    long left, right;
    char *subject = 0;
    char *cursubject;
    char *filename, *question;
    art_num artNum;

    if (CurrentMode != ARTICLE_MODE) {
	return 0;
    }

    TextDisableRedisplay(Text);

    if (TextGetSelectedOrCurrentLines(Text, &left, &right)) {
	TextUnsetSelection(Text);
	TextSetInsertionPoint(Text, left);
    }
    else
	goto done;
    
    artNum = atol(&SubjectString[left+2]);
    subject = author ? getAuthor(artNum) : getSubject(artNum);
    subject = XtNewString(subject);

    left = 0;
    while (SubjectString[left] != '\0') {
	artNum = atol(&SubjectString[left+2]);
	cursubject = author ? getAuthor(artNum) : getSubject(artNum);
	/* only kill those that have not been marked as unread */
	if ((STREQ(subject, cursubject)) &&
	    (SubjectString[left] != UNREAD_MARKER)) {
	    markArticleAsRead(artNum);
	    (void) markStringRead(SubjectString, left);
	    TextInvalidate(Text, SubjectString, left, left + 1);
	}
	if (!moveCursor(FORWARD, SubjectString, &left)) {
	    break;
	}
    }

    infoNow(ERROR_SUB_KILL_MSG);

    if (getNearbyArticle(art_FORWARD | art_CURRENT | art_WRAP | art_UNREAD,
			 &filename, &question, &artNum)
	== art_DONE) {
	maybeExitArticleMode();
	goto done;
    }

    foundArticle(filename, question);

  done:
    TextEnableRedisplay(Text);
    return subject;
}
    
/*ARGSUSED*/
void artKillSessionFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    String SubjectKilled;

    if (CurrentMode != ARTICLE_MODE)
	return;

    SubjectKilled = _artKillSession(widget, False);
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
void artKillAuthorFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    String AuthorKilled = _artKillSession(widget, True);
    FREE(AuthorKilled);
}

/*ARGSUSED*/
void artKillLocalFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    struct newsgroup *newsgroup = CurrentGroup;
    String SubjectKilled;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    SubjectKilled = _artKillSession(widget, False);
    killItem(newsgroup, SubjectKilled, KILL_LOCAL);
    FREE(SubjectKilled);
    return;
}

/*ARGSUSED*/
void artKillGlobalFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    struct newsgroup *newsgroup = CurrentGroup;
    String SubjectKilled;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    SubjectKilled = _artKillSession(widget, False);
    killItem(newsgroup, SubjectKilled, KILL_GLOBAL);
    FREE(SubjectKilled);
    return;
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
    long ArtPosition = 0;

    busyCursor();
    TextUnsetSelection(Text);

    fillUpArray(firstArticle());

    /*
      This is cheating, perhaps, but it works.  Set the current
      article to the first article, so that getSubjects() will
      retrieve all articles in the newsgroup.
      */
    FREE(SubjectString);
    FirstListedArticle = firstArticle();
    SubjectString = getSubjects(ALL, FirstListedArticle);

    findArticle(SubjectString, currentArticle(), &ArtPosition);

    TextDisableRedisplay(Text);
    TextSetString(Text, SubjectString);
    TextSetInsertionPoint(Text, ArtPosition);
    adjustMinMaxLines(SubjectString);
    TextEnableRedisplay(Text);

    unbusyCursor();
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
    busyCursor();
    TextUnsetSelection(Text);
    if ((int) client_data == XRNgotoArticle_ABORT) {
	PopDownDialog(GotoArticleBox);
	GotoArticleBox = 0;
	unbusyCursor();
	inCommand = 0;
	return;
    }
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
	break;

    case MATCH:
	TextDisableRedisplay(Text);

	FirstListedArticle = MIN(FirstListedArticle, artNum);
	new = getSubjects(ALL, FirstListedArticle);
	if (strcmp(SubjectString, new)) {
	    FREE(SubjectString);
	    SubjectString = new;
	    TextSetString(Text, SubjectString);
	}
	else {
	    FREE(new);
	}

	ArtPosition = 0;
	findArticle(SubjectString, artNum, &ArtPosition);

	TextSetInsertionPoint(Text, ArtPosition);

	foundArticle(filename, question);

	TextEnableRedisplay(Text);

	break;
    }

    PopDownDialog(GotoArticleBox);
    GotoArticleBox = 0;
    unbusyCursor();
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
    static char *reRet = 0;	/* returned by re_comp/regcmp */
#ifndef NDEBUG
    static int done_search = 0;
#endif
    char *newsubject;		/* subject of current line */
    char *oldString, *newString; /* strings used to build up new text string */
    char *newLine;

    oldString = NIL(char);

    abortClear();
    cancelCreate();

    if (expr != NIL(char)) {
#ifdef SYSV_REGEX
	FREE(reRet);
	if ((reRet = regcmp(expr, NULL)) == NULL)
#else
	if ((reRet = re_comp(expr)) != NULL)
#endif
        {
	    /* bad regular expression */
#ifdef SYSV_REGEX
	    mesgPane(XRN_SERIOUS, 0, UNKNOWN_REGEXP_ERROR_MSG, expr);
#else
	    mesgPane(XRN_SERIOUS, 0, KNOWN_REGEXP_ERROR_MSG, expr, reRet);
#endif
	    failedSearch();
	    cancelDestroy();
	    return ERROR;
	}
#ifndef NDEBUG
	done_search++;
#endif
    }
#ifndef NDEBUG
    else {
	assert(done_search);
    }
#endif

    if (dir == FORWARD) {
	for (;;) {
	    if (abortP()) {
		cancelDestroy();
		return ABORT;
	    }
	    if (SubjectString[*position] == '\0') {
		cancelDestroy();
		if (*position == 0) {

		    /* the string is null, no more articles are left */

		    return EXIT;
		}
		return NOMATCH;
	    }
	    (void) moveCursor(FORWARD, SubjectString, position);
	    if (SubjectString[*position] == '\0') {

		/* reached end of string */
		cancelDestroy();
		return NOMATCH;
	    }
	    *artNum = atol(&SubjectString[*position + 2]);
	    newsubject = getSubject(*artNum);

#ifdef SYSV_REGEX
	    if (regex(reRet, newsubject) != NULL)
#else
	    if (re_exec(newsubject))
#endif
            {
		/* found a match to the regular expression */

		gotoArticle(*artNum);
		if (getArticle(file, ques) != XRN_OKAY) {
		    /* the matching article was invalid */

		    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);

		    TextRemoveLine(Text, *position);
		    removeLine(SubjectString, position);
		    continue;
		}
		cancelDestroy();
		TextSetInsertionPoint(Text, *position);
		return MATCH;
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
		return ABORT;
	    }
	    if ((*position == 0) && (SubjectString[*position] == '\0')) {

		/* no more articles remain, return to Newgroup mode */
		cancelDestroy();
		return EXIT;
	    }
	    if (*position != 0) {

		/* we are still within the subject list */

		(void) moveCursor(BACK, SubjectString, position);
		*artNum = atol(&SubjectString[*position + 2]);
		newsubject = getSubject(*artNum);

#ifdef SYSV_REGEX
		if (regex(reRet, newsubject) != NULL)
#else
		if (re_exec(newsubject))
#endif
                {
		    /* an article matching the regular expression was found */

		    gotoArticle(*artNum);
		    if (getArticle(file, ques) != XRN_OKAY) {
			/* article is invalid, remove it from the text string*/

			mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);

			TextRemoveLine(Text, *position);
			removeLine(SubjectString, position);
			continue;
		    }
		    cancelDestroy();
		    TextSetInsertionPoint(Text, *position);
		    return MATCH;
		}
	    } else {

		/* must query the news server for articles not shown */
		/* on the current subject screen */

		if ((newLine = getPrevSubject()) == NIL(char)) {
		    
		    /* all articles have been exhausted, reset variables */
		    /* to what they were before the search was started */

		    failedSearch();
		    cancelDestroy();
		    FREE(oldString);
		    return NOMATCH;
		}
		*artNum = atol(&newLine[2]);
		newsubject = getSubject(*artNum);
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

#ifdef SYSV_REGEX
		if (regex(reRet, newsubject) != NULL)
#else
		if (re_exec(newsubject))
#endif
                {
		    gotoArticle(*artNum);
		    if (getArticle(file, ques) != XRN_OKAY) {
			mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
			removeLine(newString, 0);
			continue;
		    }
		    TextReplace(Text, newString, strlen(newString), 0, 0);
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
		    return MATCH;
		}
		oldString = newString;
		continue;
	    }
	}
    }
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

    TextUnsetSelection(Text);
    ArtPosition = TextGetInsertionPoint(Text);

    status = subjectSearch(direction, &ArtPosition,
			   regexp, &filename, &question, &artNum);
    switch (status) {
    case ABORT:
	infoNow(ERROR_SUBJ_ABORT_MSG);
	break;

    case NOMATCH:
	(void) sprintf(error_buffer, ERROR_SUBJ_EXPR_MSG,
		       regexp ? regexp : LastRegexp);
	infoNow(error_buffer);
    case ERROR:
	break;

    case MATCH:
	(void) sprintf(error_buffer, ERROR_SEARCH_MSG,
		       regexp ? regexp : LastRegexp);
	infoNow(error_buffer);
	foundArticle(filename, question);
	break;

    case EXIT:
	maybeExitArticleMode();
	break;
    }
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

    if (inCommand) {
	return;
    }
    inCommand = 1;
    busyCursor();

    if ((int) client_data == XRNsubSearch_ABORT) {
	goto done;
    }
    regexpr = GetDialogValue(SubSearchBox);
    if (*regexpr == 0) {
	if (LastRegexp == NIL(char)) {
	    mesgPane(XRN_INFO, 0, NO_PREV_REGEXP_MSG);
	    goto done;
	}
	regexpr = LastRegexp;
    } else {
	if (LastRegexp != NIL(char)) {
	    FREE(LastRegexp);
	}
	LastRegexp = XtNewString(regexpr);
    }

    direction = ((int) client_data == XRNsubSearch_FORWARD) ? FORWARD : BACK;
    LastSearch = direction;
    doSubSearch(LastRegexp, direction);

  done:
    if (SubSearchBox)
	PopDownDialog(SubSearchBox);
    SubSearchBox = 0;
    inCommand = 0;
    unbusyCursor();
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

    doSubSearch(0, LastSearch);
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

    TextDisableRedisplay(Text);

    ArtPosition = 0;
    findArticle(SubjectString, PrevArticle, &ArtPosition);
    gotoArticle(PrevArticle);
    if (getArticle(&filename, &question) != XRN_OKAY) {
	mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, PrevArticle);
	TextRemoveLine(Text, ArtPosition);
	removeLine(SubjectString, &ArtPosition);
    } else {
	TextSetInsertionPoint(Text, ArtPosition);
	foundArticle(filename, question);
    }

    TextEnableRedisplay(Text);
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

static void doSave(template, printing)
    String template;
    Boolean printing;
{
    String name;
    char buffer[1024];
    long left;
    art_num article;

    TextDisableRedisplay(Text);

    if (articleIterator(SubjectString, True, False, &left)) {
	while ((name = articleIterator(SubjectString, False, False, &left))) {
	    article = atol(name);
	    (void) sprintf(buffer, template, article);
	    if (saveArticleByNumber(buffer, article, printing)) {
		SubjectString[left + 1] = (printing ? PRINTED_MARKER : SAVED_MARKER);
		TextInvalidate(Text, SubjectString,
				 left + 1, left + 2);
		
	    }
	}
	left = 0;
	findArticle(SubjectString, currentArticle(), &left);
	TextSetInsertionPoint(Text, left);
	adjustMinMaxLines(SubjectString);
    }

    TextEnableRedisplay(Text);
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
    busyCursor();

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
