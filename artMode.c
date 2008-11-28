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
#include "sort.h"
#include "file_cache.h"

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

/* Is the article list currently sorted? */
static Boolean list_sorted_p;

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
static void setListSorted _ARGUMENTS((Boolean));
static void setListed _ARGUMENTS((art_num, art_num, Boolean));

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
BUTTON(artSub,subscribe);
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
BUTTON(artThreadParent,parent);
BUTTON(artKillSubject,subject kill);
BUTTON(artKillAuthor,author kill);
BUTTON(artKillThread,thread kill);
BUTTON(artKillSubthread,subthread kill);
BUTTON(artSubSearch,subject search);
BUTTON(artContinue,continue);
BUTTON(artPost,post);
BUTTON(artPostAndMail,post and mail);
BUTTON(artMail,mail);
BUTTON(artExit,exit);
BUTTON(artCheckPoint,checkpoint);
BUTTON(artGripe,gripe);
BUTTON(artListOld,list old);
BUTTON(artResort, resort);

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
    {"artSub",		artSubAction},
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
    {"artThreadParent",	artThreadParentAction},
    {"artKillSubject",	artKillSubjectAction},
    {"artKillAuthor",	artKillAuthorAction},
    {"artKillThread",	artKillThreadAction},
    {"artKillSubthread",	artKillSubthreadAction},
    {"artSubSearch",	artSubSearchAction},
    {"artContinue",	artContinueAction},
    {"artPost",		artPostAction},
    {"artPostAndMail",	artPostAndMailAction},
    {"artMail",		artMailAction},
    {"artExit",		artExitAction},
    {"artCheckPoint",	artCheckPointAction},
    {"artGripe",	artGripeAction},
    {"artListOld",	artListOldAction},
    {"artResort",	artResortAction},
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

static ButtonList ArtButtonList[] = {
  {"artQuit",		 artQuitCallbacks,	      ARTQUIT_EXSTR,	     True},
  {"artNextUnread",	 artNextUnreadCallbacks,      ARTNEXTUNREAD_EXSTR,   True},
  {"artNext",		 artNextCallbacks,	      ARTNEXT_EXSTR,	     True},
  {"artPrev",		 artPrevCallbacks,	      ARTPREV_EXSTR,	     True},
  {"artLast",		 artLastCallbacks,	      ARTLAST_EXSTR,	     True},
  {"artCurrent",	 artCurrentCallbacks,	      ARTCURRENT_EXSTR,	     True},
  {"artUp",		 artUpCallbacks,	      ARTUP_EXSTR,	     True},
  {"artDown",		 artDownCallbacks,	      ARTDOWN_EXSTR,	     True},
  {"artNextGroup",	 artNextGroupCallbacks,	      ARTNEXTGROUP_EXSTR,    True},
  {"artGotoArticle",	 artGotoArticleCallbacks,     ARTGOTOARTICLE_EXSTR,  True},
  {"artCatchUp",	 artCatchUpCallbacks,	      ARTCATCHUP_EXSTR,	     True},
  {"artFedUp",		 artFedUpCallbacks,	      ARTFEEDUP_EXSTR,	     True},
  {"artMarkRead",	 artMarkReadCallbacks,	      ARTMARKREAD_EXSTR,     True},
  {"artMarkUnread",	 artMarkUnreadCallbacks,      ARTMARKUNREAD_EXSTR,   True},
  {"artSub",		 artSubCallbacks,	      ARTSUB_EXSTR,	     True},
  {"artUnsub",		 artUnsubCallbacks,	      ARTUNSUB_EXSTR,	     True},
  {"artScroll",		 artScrollCallbacks,	      ARTSCROLL_EXSTR,	     True},
  {"artScrollBack",	 artScrollBackCallbacks,      ARTSCROLLBACK_EXSTR,   True},
  {"artScrollLine",	 artScrollLineCallbacks,      ARTSCROLLLINE_EXSTR,   True},
  {"artScrollBackLine",	 artScrollBackLineCallbacks,  ARTSCROLLBCKLN_EXSTR,  True},
  {"artScrollEnd",	 artScrollEndCallbacks,	      ARTSCROLLEND_EXSTR,    True},
  {"artScrollBeginning", artScrollBeginningCallbacks, ARTSCROLLBEG_EXSTR,    True},
  {"artScrollIndex",	 artScrollIndexCallbacks,     ARTSCROLLINDEX_EXSTR,  True},
  {"artScrollIndexBack", artScrollIndexBackCallbacks, ARTSCROLLINDBCK_EXSTR, True},
  {"artSubNext",	 artSubNextCallbacks,	      ARTSUBNEXT_EXSTR,	     True},
  {"artSubPrev",	 artSubPrevCallbacks,	      ARTSUBPREV_EXSTR,	     True},
  {"artThreadParent",	 artThreadParentCallbacks,    ARTPARENT_EXSTR,	     True},
  {"artKillSubject",	 artKillSubjectCallbacks,     ARTKILLSUBJECT_EXSTR,  True},
  {"artKillAuthor",	 artKillAuthorCallbacks,      ARTKILLAUTHOR_EXSTR,   True},
  {"artKillThread",	 artKillThreadCallbacks,      ARTKILLTHREAD_EXSTR,   True},
  {"artKillSubthread",	 artKillSubthreadCallbacks,   ARTKILLSUBTHREAD_EXSTR,True},
  {"artSubSearch",	 artSubSearchCallbacks,	      ARTSUBSEARCH_EXSTR,    True},
  {"artContinue",	 artContinueCallbacks,	      ARTCONTINUE_EXSTR,     True},
  {"artPost",		 artPostCallbacks,	      ARTPOST_EXSTR,	     True},
  {"artPostAndMail",	 artPostAndMailCallbacks,     ARTPOST_MAIL_EXSTR,    True},
  {"artMail",		 artMailCallbacks,	      MAIL_EXSTR,	     True},
  {"artExit",		 artExitCallbacks,	      ARTEXIT_EXSTR,	     True},
  {"artCheckPoint",	 artCheckPointCallbacks,      ARTCHECKPOINT_EXSTR,   True},
  {"artGripe",		 artGripeCallbacks,	      ARTGRIPE_EXSTR,	     True},
  {"artListOld",	 artListOldCallbacks,	      ARTLISTOLD_EXSTR,	     True},
  {"artResort",		 artResortCallbacks,	      ARTRESORT_EXSTR,	     True},
};

static int ArtButtonListCount = XtNumber(ArtButtonList);

static ButtonList ArtSpecButtonList[] = {
  {"artSave",             artSaveCallbacks,             ARTSAVE_EXSTR,	     True},
  {"artReply",            artReplyCallbacks,            ARTREPLY_EXSTR,	     True},
  {"artForward",          artForwardCallbacks,          ARTFORWARD_EXSTR,    True},
  {"artFollowup",         artFollowupCallbacks,         ARTFOLLOWUP_EXSTR,   True},
  {"artFollowupAndReply", artFollowupAndReplyCallbacks, ARTFOLLOWREPL_EXSTR, True},
  {"artCancel",           artCancelCallbacks,           ARTCANCEL_EXSTR,     True},
  {"artRot13",            artRot13Callbacks,            ARTROT13_EXSTR,	     True},
#ifdef XLATE
  {"artXlate",            artXlateCallbacks,            ARTXLATE_EXSTR,	     True},
#endif /*XLATE */
  {"artHeader",           artHeaderCallbacks,           ARTHEADER_EXSTR,     True},
  {"artPrint",            artPrintCallbacks,            ARTPRINT_EXSTR,	     True},
};

static int ArtSpecButtonListCount = XtNumber(ArtSpecButtonList);

/*
  Adjust the upper text window so that the number of lines above the
  cursor is greater than or equal to minLines and less than or equal
  to maxLines, if possible.  If the number of lines is within the
  valid range,	don't do anything; otherwise, scroll so that
  defaultLines are above the cursor, if defaultLines is within the
  valid range, or just enough to put us within the valid range,
  otherwise.  In any case, the cursor will be visible when done.

  If motion is FORWARD, then we'll only scroll forwards as long as the
  cursor is visible.  If motion os BACK, then we'll only scroll
  backwards as long as the cursor is visible.
  */
static void adjustMinMaxLines(motion)
     int motion;
{
    long CursorPosition = TextGetInsertionPoint(SubjectText);
    long top = TextGetTopPosition(SubjectText), TopPosition = top, end;
    int height = TextGetLines(SubjectText);
    int min = app_resources.minLines, max = app_resources.maxLines;
    int def = app_resources.defaultLines;
    int numLines, below;
    Boolean off_screen = False;

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
    if (top > CursorPosition) {
	top = CursorPosition;
	off_screen = True;
    }

    /*
      Figure out how many lines are above the cursor.
      */
    for (numLines = 1; top < CursorPosition; numLines++) {
	int ret = moveCursor(FORWARD, SubjectString, &top);
	assert(ret);
    }

    while (numLines > height) {
      /* We've scrolled down, but the Text widget hasn't realized it yet. */
      numLines -= 1;
      (void) moveCursor(FORWARD, SubjectString, &TopPosition);
      off_screen = True;
    }

    /*
      Figure out how many lines are below the cursor.
      */
    below = 0;
    end = top;
    while (moveCursor(FORWARD, SubjectString, &end) && (below < height))
	below++;
    if (! below) 
      /* Special case: if the cursor is at the end of the Subject
	string, it'll be on a line by itself, and "below" will
	therefore be 0 (since there's nothing after the cursor), but
	we want the empty line that the cursor is on to count as a
	line so that it'll be displayed properly.  */
      below = 1;

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
	if ((TopPosition != top) &&
	    (off_screen ||
	     (motion == JUMP) ||
	     (motion == FORWARD && TopPosition < top) ||
	     (motion == BACK && top < TopPosition)))
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
					    long, Boolean, Boolean,
					    int));

static void updateSubjectWidget(
				_ANSIDECL(String,	string),
				_ANSIDECL(long,		left),
				_ANSIDECL(long,		right),
				_ANSIDECL(Boolean,	update_top),
				_ANSIDECL(Boolean,	preserve_top),
				_ANSIDECL(int,		motion)
				)
     _KNRDECL(String,	string)
     _KNRDECL(long,	left)
     _KNRDECL(long,	right)
     _KNRDECL(Boolean,	update_top)
     _KNRDECL(Boolean,	preserve_top)
     _KNRDECL(int,	motion)
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
	adjustMinMaxLines(motion);

    if (disable)
	TextEnableRedisplay(SubjectText);
}


/*
  Get the nearest article to the cursor.  If there is no article
  on the current line, search forward or backwards for a valid
  article, depending on the value of status.  Return the filename,
  question and number of the article obtained.
  */
static int getNearbyArticle _ARGUMENTS((int, file_cache_file **, char **, long *));

static int getNearbyArticle(status, file, question, artNum)
    int status;
    file_cache_file **file;
    char **question;
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

	if (getArticle(CurrentGroup, *artNum, file, question) != XRN_OKAY) {
	    mesgPane(XRN_SERIOUS, mesg_name, ART_NOT_AVAIL_MSG, *artNum);
	    TextRemoveLine(SubjectText, ArtPosition);
	    removeLine(SubjectString, &ArtPosition);
	    setListed(*artNum, *artNum, False);
	    if (status & art_FORWARD)
		move = False;
	    continue;
	}
	CurrentGroup->current = *artNum;
	ret = art_CHANGE;
	break;
    }

    TextSetInsertionPoint(SubjectText, ArtPosition);
    return ret;
}

static void setListSorted(
			  _ANSIDECL(Boolean,	sorted)
			  )
     _KNRDECL(Boolean, 	sorted)
{
  setButtonSensitive(SubjectButtonBox, "artResort", !sorted);
  list_sorted_p = sorted;
}

static void setListed(
		      _ANSIDECL(art_num,	first),
		      _ANSIDECL(art_num,	last),
		      _ANSIDECL(Boolean,	listed)
		      )
     _KNRDECL(art_num,	first)
     _KNRDECL(art_num,	last)
     _KNRDECL(Boolean,	listed)
{
  art_num i;

  if (! first)
    return;

  for (i = first; i <= last; i++) {
    struct article *art = artStructGet(CurrentGroup, i, True);
    if (IS_MAYBE_LISTED(art) || IS_LISTED(art)) {
      /* Clear MAYBE_LISTED */
      SET_UNLISTED(art);
      /* Maybe set LISTED */
      if (listed)
	SET_LISTED(art);
    }
    artStructSet(CurrentGroup, &art);
  }
  return;
}

#define CHANGE 0		/* subject window has changed */
#define NOCHANGE 1		/* subject window has not changed */
#define DONE 2			/* no new article was found */
				/* EXIT is already defined, it implies */
				/* there are no articles left at all */

static int isPrevSubject _ARGUMENTS((char *, file_cache_file **, char **,
				     art_num *));

static int isPrevSubject(subject, file, question, artNum)
    char *subject;
    file_cache_file **file;
    char **question;
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
    art_num first_new = 0, last_new = 0;

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
		if (getArticle(CurrentGroup, *artNum, file, question)
		    != XRN_OKAY) {
		  art_unavailable:
		    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
		    TextRemoveLine(SubjectText, ArtPosition);
		    removeLine(SubjectString, &ArtPosition);
		    setListed(*artNum, *artNum, False);
		    continue;
		}
		CurrentGroup->current = *artNum;
		cancelDestroy();
		TextSetInsertionPoint(SubjectText, ArtPosition);
		ret = NOCHANGE;
		goto done;
	    }
	    continue;
	} else {
	    struct article *art;

	    if ((newLine = getPrevSubject(line_length)) == NIL(char)) {
		failedSearch();
		cancelDestroy();
		setListed(first_new, last_new, False);
		ret = DONE;
		goto done;
	    }
	    *artNum = atol(&newLine[2]);
	    if (! (newsubject = getSubject(*artNum))) {
		mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
		continue;
	    }

	    if (! last_new)
	      first_new = last_new = *artNum;
	    else
	      first_new = *artNum;

	    art = artStructGet(CurrentGroup, *artNum, True);
	    SET_MAYBE_LISTED(art);

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

		if (getArticle(CurrentGroup, *artNum, file, question)
		    != XRN_OKAY) {
		    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
		    (void) strcpy(newSubjects, index(newSubjects, '\n'));
		    setListed(*artNum, *artNum, False);
		}
		else {
		    CurrentGroup->current = *artNum;
		    setListed(first_new, last_new, True);
		    setListSorted(False);

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
static int isNextSubject _ARGUMENTS((char *, long, file_cache_file **,
				     char **, art_num *));

static int isNextSubject(subject, ArtPosition, file, question, artNum)
    char *subject;
    long ArtPosition;
    file_cache_file **file;
    char **question;
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
	    if (getArticle(CurrentGroup, *artNum, file, question) != XRN_OKAY) {
	      art_unavailable:
		mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
		TextRemoveLine(SubjectText, ArtPosition);
		removeLine(SubjectString, &ArtPosition);
		setListed(*artNum, *artNum, False);
		continue;
	    }
	    CurrentGroup->current = *artNum;
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
        struct article *art;

	newString = XtMalloc(utStrlen(SubjectString) + utStrlen(newLine) + 1);
	(void) strcpy(newString, newLine);
	(void) strcat(newString, SubjectString);
	FREE(SubjectString);
	SubjectString = newString;

	TextSetString(SubjectText, SubjectString);
	*artNum = atol(newLine + 2);
	FirstListedArticle = *artNum;

	art = artStructGet(CurrentGroup, *artNum, True);
	SET_LISTED(art);
	artStructSet(CurrentGroup, &art);
	setListSorted(False);

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
    struct newsgroup *newsgroup = CurrentGroup;
    int oldMode;
    struct article *art;
    int i;

    FREE(SubjectString);
    FirstListedArticle = newsgroup->current;

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

    art = artStructGet(newsgroup, FirstListedArticle, True);
    SET_LISTED(art);
    artStructSet(newsgroup, &art);

    for (i = FirstListedArticle + 1; i <= newsgroup->last; i++) {
      art = artStructGet(newsgroup, i, False);
      if (IS_AVAIL(art) && IS_UNREAD(art)) {
	struct article copy = *art;
	SET_LISTED(&copy);
	artStructReplace(newsgroup, &art, &copy, i);
      }
    }

    SubjectString = getSubjects(TextGetColumns(SubjectText), FirstListedArticle,
				False);

    if (! (SubjectString && *SubjectString)) {
	mesgPane(XRN_INFO, 0, PROBABLY_EXPIRED_MSG, newsgroup->name);
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

    setButtonSensitive(SubjectButtonBox, "artLast", False);
    setButtonSensitive(SubjectButtonBox, "artContinue", False);
    setButtonSensitive(SubjectButtonBox, "artListOld", True);
    setButtonSensitive(SubjectButtonBox, "artSub", !IS_SUBSCRIBED(newsgroup));

    setListSorted(True);

    TextDisableRedisplay(SubjectText);

    /* get and display the article */
    updateSubjectWidget(SubjectString, 0, 0, True, False, JUMP);
    ArtStatus = art_FORWARD | art_CURRENT | art_WRAP;
    artNextFunction(0, 0, 0, 0);

    TextEnableRedisplay(SubjectText);

    return GOOD_GROUP;
}


/*
 * If the article to be displayed has changed, update the article
 * window and redraw the mode line
 */

static void redrawArticleWidget _ARGUMENTS((file_cache_file *, char *));

static void redrawArticleWidget(file, question)
    file_cache_file *file;
    char *question;
{
    String old = TextGetFile(ArticleText);

    if ((! old) || strcmp(old, file_cache_file_name(FileCache, *file))) {
	TextSetFile(ArticleText, file_cache_file_name(FileCache, *file));

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
static void foundArticle _ARGUMENTS((file_cache_file *, char *, int));

static void foundArticle(file, ques, motion)
    file_cache_file *file;
    char *ques;
    int motion;
{
    long ArtPosition = TextGetInsertionPoint(SubjectText);

    if ((PrevArticle = CurrentArticle)) {
	struct article *art = artStructGet(CurrentGroup, PrevArticle, False);

	setButtonSensitive(SubjectButtonBox, "artLast", True);

	if (art->file)
	  file_cache_file_unlock(FileCache, *art->file);
    }

    CurrentGroup->current = markStringRead(SubjectString, ArtPosition);
    CurrentArticle = CurrentGroup->current;

    updateSubjectWidget(SubjectString, ArtPosition, ArtPosition + 1,
			True, False, motion);

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

    file_cache_file *file;
    char *question;
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
    updateSubjectWidget(SubjectString, 0, ArtPosition, True, False, JUMP);
    if (getNearbyArticle(art_FORWARD | art_CURRENT | art_WRAP | art_UNREAD,
			 &file, &question, &artNum) ==
	art_DONE) {
	if (exit_mode)
	    exitArticleMode();
	else
	    artNextGroupFunction(NULL, NULL, NULL, NULL);
	return;
    }
    foundArticle(file, question, JUMP);

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
    file_cache_file *file;	/* the article file */
    char *question;		/* question to put in the question box */
    long artNum;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    TextDisableRedisplay(SubjectText);

    if (selectedArticle() || !cursorOnCurrent())
	ArtStatus |= art_CURRENT;

    if (getNearbyArticle(ArtStatus, &file, &question, &artNum) == art_DONE) {
	maybeExitArticleMode();
	goto done;
    }
    /* update the text window */
    foundArticle(file, question, FORWARD);

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
    file_cache_file *file;	/* name of the article file */
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

    if (getNearbyArticle(ArtStatus, &file, &question,
			 &artNum) == art_DONE) {
	maybeExitArticleMode();
	goto done;
    }
    /* update the text window */
    foundArticle(file, question, JUMP);

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
    return(under_cursor == CurrentGroup->current);
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
    file_cache_file *file;
    char *question;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    TextDisableRedisplay(SubjectText);

    ArtStatus = art_BACKWARD | art_WRAP;

    if (selectedArticle() || !cursorOnCurrent())
	ArtStatus |= art_CURRENT;

    if (getNearbyArticle(ArtStatus, &file, &question, &artNum) ==
	art_DONE) {
	goto done;
    }
    foundArticle(file, question, BACK);

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
 * called when the user wants to subscribe to the current group
 */
/*ARGSUSED*/
void artSubFunction(widget, event, string, count)
     Widget widget;
     XEvent *event;
     String *string;
     Cardinal *count;
{
  struct newsgroup *newsgroup = CurrentGroup;

  if (CurrentMode != ARTICLE_MODE)
    return;

  if (IS_SUBSCRIBED(newsgroup))
    return;

  if (IS_NOENTRY(newsgroup))
    addToNewsrcEnd(newsgroup->name, SUBSCRIBE);

  SET_SUB(newsgroup);

  mesgPane(XRN_INFO, 0, SUB_DONE_MSG, newsgroup->name);
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
    ArtPosition = findArticle(SubjectString, CurrentGroup->current, False);
    TextSetInsertionPoint(SubjectText, ArtPosition);
    updateSubjectWidget(SubjectString, left, right,
			app_resources.subjectScrollBack,
			!app_resources.subjectScrollBack, JUMP);

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
    file_cache_file *file;
    char *question;
    char *subject = 0;
    art_num artNum;
    int status;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    TextDisableRedisplay(SubjectText);

    if (selectedArticle() || !cursorOnCurrent()) {
	if (getNearbyArticle(art_FORWARD | art_CURRENT | art_WRAP, &file,
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

	    status = isNextSubject(subject, ArtPosition, &file, &question,
				   &artNum);
	}

	switch (status) {
	case ABORT:
	    infoNow(ERROR_SUBJ_ABORT_MSG);
	    goto done;

	case NOCHANGE:
	    (void) sprintf(error_buffer, ERROR_SUBJ_SEARCH_MSG, subject);
	    INFO(error_buffer);
	    foundArticle(file, question, FORWARD);
	    goto done;

	case DONE:
	    infoNow(ERROR_SUBJ_EXH_MSG);
	    TextSetInsertionPoint(SubjectText, 0);
	    if (getNearbyArticle(art_FORWARD | art_CURRENT | art_WRAP |
				 art_UNREAD, &file,
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

    foundArticle(file, question, FORWARD);

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
    file_cache_file *file;
    char *question;
    int status;
    long ArtPosition;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }

    TextDisableRedisplay(SubjectText);

    if (selectedArticle() || !cursorOnCurrent()) {
	if (getNearbyArticle(art_FORWARD | art_CURRENT | art_WRAP, &file,
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

	status = isPrevSubject(subject, &file, &question, &artNum);
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

    foundArticle(file, question, BACK);

  done:
    FREE(subject);
    TextEnableRedisplay(SubjectText);
    return;
}

void artThreadParentFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
  long left, right;
  art_num artNum, parent = 0;
  struct article *art;
  char *refs = 0, *begin_id, *end_id;
  file_cache_file *file;
  char *question;
  Boolean cancel_created = False;
  Boolean prefer_listed = True;

  if (CurrentMode != ARTICLE_MODE)
    return;

  if (event || (widget && (event = XtLastEventProcessed(XtDisplay(widget))))) {
    unsigned int *state = 0;
    if ((event->type == KeyPress) || (event->type == KeyRelease))
      state = &event->xkey.state;
    else if ((event->type == ButtonPress) || (event->type == ButtonRelease))
      state = &event->xbutton.state;
    assert(state);
    if (*state & ShiftMask)
      prefer_listed = False;
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

  if (prefer_listed && (parent = art->parent) &&
      articleIsAvailable(CurrentGroup, parent))
    goto done;

  if (! ((refs = XtNewString(art->references)) && *refs))
    goto done;

  for (end_id = strchr(refs, '\0'); end_id > refs; end_id--) {
    art_num block_end, block_start, checkee;

    while ((end_id > refs) && (*end_id != '>'))
      end_id--;
    if (end_id == refs)
      break;
    end_id[1] = '\0';
    for (begin_id = end_id; (begin_id >= refs) && (*begin_id != '<'); begin_id--)
      /* empty */;
    if (begin_id < refs)
      break;
    if ((parent = getArticleNumberFromIdXref(CurrentGroup, begin_id)))
      if ((parent < 0) || !articleIsAvailable(CurrentGroup, parent)) {
	parent = 0;
	continue;
      }
      else
	break;
    if (! cancel_created) {
      abortClear();
      cancelCreate("CancelThreadParent");
      cancel_created = True;
    }
    for (block_end = CurrentGroup->last; block_end >= CurrentGroup->first;
	 block_end = block_start - 1) {
      block_start = MAX(block_end - 10, CurrentGroup->first);
      (void) getidlist(CurrentGroup, block_start, block_end, False, 0);
      for (checkee = block_end ; checkee >= block_start; checkee--) {
	art = artStructGet(CurrentGroup, checkee, False);
	if (art->id && !strcmp(art->id, begin_id) &&
	    articleIsAvailable(CurrentGroup, checkee)) {
	  parent = checkee;
	  break;
	}
      }
      if (abortP()) {
	parent = -1;
	break;
      }
    }
  }

done:
  if (cancel_created)
    cancelDestroy();
  if (parent > 0) {
    long ArtPosition;

    (void) fillUpArray(CurrentGroup, parent, parent, False, False);
    art = artStructGet(CurrentGroup, parent, True);
    SET_LISTED(art);
    artStructSet(CurrentGroup, &art);

    if ((ArtPosition = findArticle(SubjectString, parent, True)) < 0) {
      FREE(SubjectString);
      FirstListedArticle = MIN(FirstListedArticle, parent);
      SubjectString = getSubjects(TextGetColumns(SubjectText), FirstListedArticle,
				  True);
      TextSetString(SubjectText, SubjectString);
      ArtPosition = findArticle(SubjectString, parent, False);
    }
    TextSetInsertionPoint(SubjectText, ArtPosition);

    if (getArticle(CurrentGroup, parent, &file, &question) != XRN_OKAY) {
      mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, parent);
      TextRemoveLine(SubjectText, ArtPosition);
      removeLine(SubjectString, &ArtPosition);
      setListed(parent, parent, False);
    }
    else {
      foundArticle(file, question, BACK);
    }
  }
  else if (parent == 0) {
    infoNow((refs && *refs) ? ERROR_PARENT_UNAVAIL_MSG : ERROR_NO_PARENT_MSG);
  }
  XtFree(refs);
  TextEnableRedisplay(SubjectText);
  return;
}

typedef char * (*_fixfunction) _ARGUMENTS((char *));

/*
  Mark all articles with the current subject or author as read.
  */
static String artKillSession _ARGUMENTS((int, _fixfunction, int, Boolean));

/*ARGSUSED*/
static String artKillSession(
			     _ANSIDECL(int,		source_offset),
			     _ANSIDECL(_fixfunction,	source_fix_function),
			     _ANSIDECL(int,		target_offset),
			     _ANSIDECL(Boolean,		show_error)
			     )
     _KNRDECL(int,		source_offset)
     _KNRDECL(_fixfunction,	source_fix_function)
     _KNRDECL(int,		target_offset)
     _KNRDECL(Boolean,		show_error)
{
    long left, right;
    char *match_val = 0;
    char *cur_val;
    file_cache_file *file;
    char *question;
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

    match_val = *(char **)((char *)art + source_offset);
    if (source_fix_function)
      match_val = (*source_fix_function)(match_val);
    if (! match_val) {
      if (show_error)
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
		cur_val = *(char **)((char *)art + target_offset);
		if (! cur_val) {
		  /* XXX See above. */
		    struct article copy = *art;
		    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, artNum);
		    TextRemoveLine(SubjectText, left);
		    removeLine(SubjectString, &left);
		    SET_UNLISTED(&copy);
		    artStructReplace(CurrentGroup, &art, &copy, artNum);
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
	free(reRet);
# endif
#endif

	if (getNearbyArticle(art_FORWARD | art_CURRENT | art_WRAP | art_UNREAD,
			     &file, &question, &artNum)
	    == art_DONE) {
	  maybeExitArticleMode();
	  goto done;
	}

	foundArticle(file, question, FORWARD);
    }

  done:
    TextEnableRedisplay(SubjectText);
    return match_val;
}

static Boolean do_field_kill _ARGUMENTS((Widget, char *, int, _fixfunction, int,
					 XEvent *, String *, Cardinal *, Boolean));

static Boolean do_field_kill(
			     _ANSIDECL(Widget,		widget),
			     _ANSIDECL(char *,		field_name),
			     _ANSIDECL(int,		source_offset),
			     _ANSIDECL(_fixfunction,	source_fix_function),
			     _ANSIDECL(int,		target_offset),
			     _ANSIDECL(XEvent *,	event),
			     _ANSIDECL(String *,	string),
			     _ANSIDECL(Cardinal *,	count),
			     _ANSIDECL(Boolean,		show_error)
			  )
     _KNRDECL(Widget,		widget)
     _KNRDECL(char *,		field_name)
     _KNRDECL(int,		source_offset)
     _KNRDECL(_fixfunction,	source_fix_function)
     _KNRDECL(int,		target_offset)
     _KNRDECL(XEvent *,		event)
     _KNRDECL(String *,		string)
     _KNRDECL(Cardinal *,	count)
     _KNRDECL(Boolean,		show_error)
{
  struct newsgroup *newsgroup = CurrentGroup;
  int what = KILL_SESSION;
  unsigned int *state = 0, *button = 0;
  char *value_killed;

  if (CurrentMode != ARTICLE_MODE)
    return True;

  if (count && *count) {
    if (! strcasecmp(string[0], "session"))
      what = KILL_SESSION;
    else if (! strcasecmp(string[0], "local"))
      what = KILL_LOCAL;
    else if (! strcasecmp(string[0], "global"))
      what = KILL_GLOBAL;
    else {
      mesgPane(XRN_SERIOUS, 0, UNKNOWN_KILL_TYPE_MSG, string[0],
	       field_name);
      return True;
    }
  }
  else if (event || (widget &&
		     (event = XtLastEventProcessed(XtDisplay(widget))))) {
    if ((event->type == KeyPress) || (event->type == KeyRelease)) {
      state = &event->xkey.state;
    }
    else if ((event->type == ButtonPress) ||
	     (event->type == ButtonRelease)) {
      state = &event->xbutton.state;
      button = &event->xbutton.button;
    }
    assert(state);

    if ((*state & ShiftMask) || (button && (*button == 3)))
      what = KILL_LOCAL;
    else if ((*state & ControlMask) || (button && *button == 2))
      what = KILL_GLOBAL;
  }

  value_killed = artKillSession(source_offset, source_fix_function,
				target_offset, show_error);
  if (value_killed && (what != KILL_SESSION))
      add_kill_entry(newsgroup, what, field_name, value_killed);
  return value_killed ? True : False;
}

#define SUBJ_OFFSET XtOffsetOf(struct article, subject)
#define FROM_OFFSET XtOffsetOf(struct article, from)
#define REFS_OFFSET XtOffsetOf(struct article, references)
#define ID_OFFSET XtOffsetOf(struct article, id)

void artKillSubjectFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
  (void) do_field_kill(widget, "Subject", SUBJ_OFFSET, subjectStrip, SUBJ_OFFSET,
		       event, string, count, True);
}

void artKillAuthorFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
  (void) do_field_kill(widget, "From", FROM_OFFSET, 0, FROM_OFFSET,
		       event, string, count, True);
}

static char *getFirstReference _ARGUMENTS((char *));

static char *getFirstReference(references)
     char *references;
{
  static char *ref_buf = NULL;
  static int buf_size = 0;
  char *lbrace, *rbrace;

  if (! (references && *references))
    return NULL;

  if (! (lbrace = strchr(references, '<')))
    return NULL;

  if (! (rbrace = strchr(lbrace, '>')))
    return NULL;

  rbrace++;

  if (rbrace - lbrace + 1 > buf_size) {
    buf_size = rbrace - lbrace + 1;
    ref_buf = XtRealloc(ref_buf, buf_size);
  }

  strncpy(ref_buf, lbrace, rbrace - lbrace);
  ref_buf[rbrace - lbrace] = '\0';

  return ref_buf;
}

void artKillThreadFunction(widget, event, string, count)
     Widget widget;
     XEvent *event;
     String *string;
     Cardinal *count;
{
  if (! art_sort_need_threads())
    return;

  if (! do_field_kill(widget, "References", REFS_OFFSET, getFirstReference,
		      REFS_OFFSET, event, string, count, False))
    (void) do_field_kill(widget, "References", ID_OFFSET, getFirstReference,
			 REFS_OFFSET, event, string, count, True);
}

void artKillSubthreadFunction(widget, event, string, count)
     Widget widget;
     XEvent *event;
     String *string;
     Cardinal *count;
{
  if (! art_sort_need_threads())
    return;

  (void) do_field_kill(widget, "References", ID_OFFSET, getFirstReference,
		       REFS_OFFSET, event, string, count, True);
}

#define XRNgotoArticle_ABORT	0
#define XRNgotoArticle_DOIT	1
#define XRNgotoArticle_DEFAULT	2

static Widget GotoArticleBox = (Widget) 0;

void artListOldHandler(widget, client_data, call_data)
    Widget widget;
    XtPointer client_data;
    XtPointer call_data;
{
    long ArtPosition;
    int finished;
    art_num artNum;
    char *numberstr;

    if (inCommand && ((int) client_data != XRNgotoArticle_DEFAULT))
      return;
    inCommand = 1;
    xrnBusyCursor();

    TextUnsetSelection(SubjectText);

    if ((int) client_data == XRNgotoArticle_ABORT) {
      goto finished;
    }
    else if ((int) client_data == XRNgotoArticle_DEFAULT) {
      artNum = CurrentGroup->first;
    }
    else {
      numberstr = GetDialogValue(GotoArticleBox);
      if (! (numberstr && *numberstr))
	artNum = FirstListedArticle;
      else {
	if (*numberstr == '+')
	  artNum = atol(numberstr + 1);
	else
	  artNum = atol(numberstr);
	if (! artNum) {
	  mesgPane(XRN_SERIOUS, 0, BAD_ART_NUM_MSG, numberstr);
	  goto finished;
	}
	if (*numberstr == '+')
	  artNum = MAX(FirstListedArticle - artNum, CurrentGroup->first);
      }
    }

    abortClear();
    cancelCreate("CancelListOld");

    finished = ((! abortP()) &&
		(fillUpArray(CurrentGroup, artNum, 0, True, False) != ABORT));

    if (finished) {
      int i;
      struct article *art, copy;

      FREE(SubjectString);

      for (i = artNum; i <= CurrentGroup->last; i++) {
	art = artStructGet(CurrentGroup, i, False);
	if (IS_AVAIL(art) && !IS_LISTED(art)) {
	  copy = *art;
	  SET_LISTED(&copy);
	  artStructReplace(CurrentGroup, &art, &copy, i);
	  FirstListedArticle = MIN(FirstListedArticle, i);
	}
      }
      SubjectString = getSubjects(TextGetColumns(SubjectText),
				  FirstListedArticle, True);
    }

    ArtPosition = findArticle(SubjectString, CurrentGroup->current, False);
    TextDisableRedisplay(SubjectText);

    if (finished) {
      TextSetString(SubjectText, SubjectString);
      TextSetInsertionPoint(SubjectText, ArtPosition);
    }

    adjustMinMaxLines(JUMP);
    TextEnableRedisplay(SubjectText);

    if (finished) {
      if (artNum == CurrentGroup->first)
	setButtonSensitive(SubjectButtonBox, "artListOld", False);
      setListSorted(True);
    }

finished:
    if (GotoArticleBox) {
      PopDownDialog(GotoArticleBox);
      GotoArticleBox = 0;
    }
    xrnUnbusyCursor();
    cancelDestroy();
    inCommand = 0;
    return;
}

void artListOldFunction(widget, event, string, count)
  Widget widget;
  XEvent *event;
  String *string;
  Cardinal *count;
{
  static struct DialogArg args[] = {
    {ABORT_STRING, artListOldHandler, (XtPointer) XRNgotoArticle_ABORT},
    {DOIT_STRING, artListOldHandler, (XtPointer) XRNgotoArticle_DOIT},
  };
  unsigned int *state = 0;

  if (CurrentMode != ARTICLE_MODE)
    return;

  if (event || (widget && (event = XtLastEventProcessed(XtDisplay(widget))))) {
    if ((event->type == KeyPress) || (event->type == KeyRelease))
      state = &event->xkey.state;
    else if ((event->type == ButtonPress) || (event->type == ButtonRelease))
      state = &event->xbutton.state;
    assert(state);
  }

  if ((state && (*state & ControlMask)) || (count && *count)) {
    if (! GotoArticleBox)
      GotoArticleBox = CreateDialog(TopLevel, LIST_OLD_NUMBER_MSG,
				    DIALOG_TEXT, args, XtNumber(args));
    PopUpDialog(GotoArticleBox);
  }
  else
    artListOldHandler(0, (XtPointer) XRNgotoArticle_DEFAULT, 0);
}
    
void artResortFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
  long ArtPosition;
  char *newsort = NULL;

  if (CurrentMode != ARTICLE_MODE)
    return;

  if (count && *count) {
    int i, len;
    for (len = 0, i = 0; i < *count; i++)
      len += strlen(string[i]) + 1;
    newsort = XtMalloc(len);
    for (len = 0, i = 0; i < *count; i++) {
      (void) strcpy(&newsort[len], string[i]);
      len += strlen(string[i]);
      newsort[len++] = ' ';
    }
    newsort[--len] = '\0';
  }

  if (list_sorted_p && !newsort)
    return;

  xrnBusyCursor();

  if (newsort)
    art_sort_parse_sortlist(newsort);

  TextUnsetSelection(SubjectText);
  TextDisableRedisplay(SubjectText);
  FREE(SubjectString);
  SubjectString = getSubjects(TextGetColumns(SubjectText),
			      FirstListedArticle, True);
  TextSetString(SubjectText, SubjectString);

  ArtPosition = findArticle(SubjectString, CurrentArticle, False);
  TextSetInsertionPoint(SubjectText, ArtPosition);

  adjustMinMaxLines(JUMP);

  if (newsort) {
    XtFree(newsort);
    art_sort_parse_sortlist(app_resources.sortedSubjects);
    setListSorted(False);
  } else
    setListSorted(True);

  TextEnableRedisplay(SubjectText);
}

/*
 * Update the .newsrc file
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
    file_cache_file *file;
    char *question;
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
    if (! (numberstr && *numberstr)) {
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

    status = moveToArticle(CurrentGroup, artNum, &file, &question);

    switch (status) {

    case NOMATCH:
    case ERROR:
	mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, artNum);
	break;

    case MATCH:
	TextDisableRedisplay(SubjectText);

	if ((ArtPosition = findArticle(SubjectString, artNum, True)) == -1) {
	  struct article *art = artStructGet(CurrentGroup, artNum, True);
	  SET_LISTED(art);
	  artStructSet(CurrentGroup, &art);
	  FirstListedArticle = MIN(FirstListedArticle, artNum);
	  new = getSubjects(TextGetColumns(SubjectText), FirstListedArticle,
			    True);
	  FREE(SubjectString);
	  SubjectString = new;
	  TextSetString(SubjectText, SubjectString);

	  setListSorted(True);
	}

	ArtPosition = findArticle(SubjectString, artNum, False);

	TextSetInsertionPoint(SubjectText, ArtPosition);

	foundArticle(file, question, JUMP);

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

static int subjectSearch _ARGUMENTS((int, long *, char *, file_cache_file **,
				     char **, art_num *));

static int subjectSearch(dir, position, expr, file, ques, artNum)
    int dir;			     /* direction, either FORWARD or BACK */
    long *position;	     /* cursor position */
    char *expr;			     /* regular expression to search for */
    file_cache_file **file;
    char **ques;	     /* status line for new article */
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
		setListed(*artNum, *artNum, False);
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

		if (getArticle(CurrentGroup, *artNum, file, ques) != XRN_OKAY) {
		    /* the matching article was invalid */
		    mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);

		    TextRemoveLine(SubjectText, *position);
		    removeLine(SubjectString, position);
		    setListed(*artNum, *artNum, False);
		    continue;
		}
		CurrentGroup->current = *artNum;
		cancelDestroy();
		TextSetInsertionPoint(SubjectText, *position);
		ret = MATCH;
		goto done;
	    }
	}
    } else {
	art_num first_new = 0, last_new = 0;

	startSearch();
	for (;;) {
	    if (abortP()) {

		/* reset pointers back to where we began, since the */
		/* search was aborted */

		failedSearch();
		cancelDestroy();
		FREE(oldString);
		setListed(first_new, last_new, False);
		ret = ABORT;
		goto done;
	    }
	    if ((*position == 0) && (SubjectString[*position] == '\0')) {

		/* no more articles remain, return to Newgroup mode */
		FREE(oldString);
		setListed(first_new, last_new, False);
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
		    setListed(*artNum, *artNum, False);
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

		    if (getArticle(CurrentGroup, *artNum, file, ques) != XRN_OKAY) {
			/* article is invalid, remove it from the text string*/
			mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);

			TextRemoveLine(SubjectText, *position);
			removeLine(SubjectString, position);
			setListed(*artNum, *artNum, False);
			continue;
		    }
		    CurrentGroup->current = *artNum;
		    cancelDestroy();
		    TextSetInsertionPoint(SubjectText, *position);
		    ret = MATCH;
		    goto done;
		}
	    } else {
		struct article *art;

		/* must query the news server for articles not shown */
		/* on the current subject screen */

		if ((newLine = getPrevSubject(line_length)) == NIL(char)) {

		    /* all articles have been exhausted, reset variables */
		    /* to what they were before the search was started */

		    failedSearch();
		    cancelDestroy();
		    FREE(oldString);
		    setListed(first_new, last_new, False);
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

		art = artStructGet(CurrentGroup, *artNum, True);
		SET_MAYBE_LISTED(art);
		if (! last_new)
		  first_new = last_new = *artNum;
		else
		  first_new = *artNum;

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
		    if (getArticle(CurrentGroup, *artNum, file, ques) != XRN_OKAY) {
			mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, *artNum);
			removeLine(newString, 0);
			SET_UNLISTED(art);
			artStructSet(CurrentGroup, &art);
			continue;
		    }

		    CurrentGroup->current = *artNum;
		    setListed(first_new, last_new, True);
		    setListSorted(False);

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
    free(reRet);
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
    file_cache_file *file;
    char *question;
    art_num artNum;
    int status;

    TextDisableRedisplay(SubjectText);

    TextUnsetSelection(SubjectText);
    ArtPosition = TextGetInsertionPoint(SubjectText);

    status = subjectSearch(direction, &ArtPosition,
			   regexp, &file, &question, &artNum);
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
	foundArticle(file, question, direction);
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
    file_cache_file *file;
    char *question;
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
    if (getArticle(CurrentGroup, PrevArticle, &file, &question) != XRN_OKAY) {
	mesgPane(XRN_SERIOUS, 0, ART_NOT_AVAIL_MSG, PrevArticle);
	TextRemoveLine(SubjectText, ArtPosition);
	removeLine(SubjectString, &ArtPosition);
	setListed(PrevArticle, PrevArticle, False);
    } else {
	CurrentGroup->current = PrevArticle;
	TextSetInsertionPoint(SubjectText, ArtPosition);
	foundArticle(file, question, JUMP);
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
	left = findArticle(SubjectString, CurrentGroup->current, False);
	TextSetInsertionPoint(SubjectText, left);
	if (app_resources.subjectScrollBack)
	    adjustMinMaxLines(JUMP);
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
    file_cache_file *file;
    char *question;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    if (toggleXlation(&file, &question) == XRN_OKAY) {
	TextClear(ArticleText);
	redrawArticleWidget(file, question);
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
    file_cache_file *file;
    char *question;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    if (toggleRotation(&file, &question) == XRN_OKAY) {
	TextClear(ArticleText);
	redrawArticleWidget(file, question);
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
    file_cache_file *file;
    char *question;

    if (CurrentMode != ARTICLE_MODE) {
	return;
    }
    if (toggleHeaders(&file, &question) == XRN_OKAY) {
	TextClear(ArticleText);
	redrawArticleWidget(file, question);
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

  if (event->type == ConfigureNotify && CurrentArticle) {
    TextDisableRedisplay(SubjectText);
    FREE(SubjectString);
    SubjectString = getSubjects(TextGetColumns(SubjectText),
				FirstListedArticle, False);
    TextSetString(SubjectText, SubjectString);

    ArtPosition = findArticle(SubjectString, CurrentArticle, False);

    TextSetInsertionPoint(SubjectText, ArtPosition);

    adjustMinMaxLines(JUMP);

    setListSorted(True);

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

	setButtonActive(ArtButtonList, "artPost", PostingAllowed);
	setButtonActive(ArtButtonList, "artPostAndMail", PostingAllowed);
	setButtonActive(ArtButtonList, "artKillThread", art_sort_need_threads());
	setButtonActive(ArtButtonList, "artKillSubthread", art_sort_need_threads());
	setButtonActive(ArtSpecButtonList, "artFollowup", PostingAllowed);
	setButtonActive(ArtSpecButtonList, "artFollowupAndReply", PostingAllowed);
	setButtonActive(ArtSpecButtonList, "artCancel", PostingAllowed);
	setButtonActive(ArtSpecButtonList, "artHeader",
			app_resources.displayLocalTime ||
			app_resources.leaveHeaders || app_resources.stripHeaders);
	setButtonActive(ArtButtonList, "artResort", app_resources.sortedSubjects &&
			*app_resources.sortedSubjects);

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
	}

#define TOP_INFO_LINE() {\
	  SubjectInfoLine = InfoLineCreate("info", 0, ArticleFrame);\
	}

#define BOTTOM_BUTTON_BOX() {\
	  ArtSpecButtonBox = ButtonBoxCreate("artButtons", ArticleFrame);\
	  doButtons(app_resources.artSpecButtonList, ArtSpecButtonBox,\
		    ArtSpecButtonList, &ArtSpecButtonListCount, BOTTOM);\
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
	XawPanedAllowResize(TEXT_PANE_CHILD(SubjectText), True);
	TextSetLines(SubjectText, app_resources.topLines);
	XawPanedAllowResize(TEXT_PANE_CHILD(SubjectText), False);
	XtVaGetValues(SubjectText, XtNheight, &height, 0);
	XtVaSetValues(SubjectText, XtNpreferredPaneSize, height, 0);

	TopInfoLine = SubjectInfoLine;
	BottomInfoLine = ArticleInfoLine;

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
    adjustMinMaxLines(BACK);
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
    adjustMinMaxLines(FORWARD);
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
      if (! (moveCursor(FORWARD, tstring, &position) && tstring[position])) {
	if (allow_failure)
	  return -1;
	else
	  ehErrorExitXRN( ERROR_FINDARTICLE_MSG );
      }
    }
    return position;
}
