#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: resources.c,v 1.59 1997-07-31 12:11:47 jik Exp $";
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
 * resources.c: routines for handling resource management
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#ifdef MOTIF
# include <Xm/Xm.h>
#else
# include <X11/Intrinsic.h>
# include <X11/StringDefs.h>
# include <X11/Shell.h>
#endif
#include <ctype.h>
#include "avl.h"
#include "news.h"
#include "xthelper.h"
#include "mesg.h"
#include "xrn.h"
#include "patchlevel.h"
#include "resources.h"
#include "error_hnds.h"
#include "internals.h"
#include "mesg_strings.h"
#include "sort.h"

#ifndef XRN_APP_CLASS
#define XRN_APP_CLASS "XRn"
#endif

/*
 * resources and command list
 */

/* extra name and class specifications */
#define XtNaddButtonList        "addButtonList"
#define XtCAddButtonList        "AddButtonList"
#define XtNallButtonList        "allButtonList"
#define XtCAllButtonList        "AllButtonList"
#define XtNartButtonList        "artButtonList"
#define XtCArtButtonList        "ArtButtonList"
#define XtNartSpecButtonList    "artSpecButtonList"
#define XtCArtSpecButtonList    "ArtSpecButtonList"
#define XtNauthenticator        "authenticator"
#define XtCAuthenticator        "Authenticator"
#define XtNauthenticatorCommand "authenticatorCommand"
#define XtCAuthenticatorCommand "AuthenticatorCommand"
#define XtNauthorFullName       "authorFullName"
#define XtCAuthorFullName       "AuthorFullName"
#define XtNbreakLength          "breakLength"
#define XtCBreakLength          "BreakLength"
#define XtNbusyIconName         "busyIconName"
#define XtCBusyIconName         "BusyIconName"
#define XtNbusyIconPixmap       "busyIconPixmap"
#define XtCBusyIconPixmap       "BusyIconPixmap"
#define XtNbuttonsOnTop		"buttonsOnTop"
#define XtCButtonsOnTop		"ButtonsOnTop"
#define XtNcacheActive		"cacheActive"
#define XtCCacheActive		"CacheActive"
#define XtNcacheFile            "cacheFile"
#define XtCCacheFile            "CacheFile"
#define XtNcacheFilesMaxFiles	"cacheFilesMaxFiles"
#define XtCCacheFilesMaxFiles	"CacheFilesMaxFiles"
#define XtNcacheFilesMaxSize	"cacheFilesMaxSize"
#define XtCCacheFilesMaxSize	"CacheFilesMaxSize"
#if SUPPORT_SILLY_CALVIN_ICON
#define XtNcalvin               "calvin"
#define XtCCalvin               "Calvin"
#endif
#define XtNcancelCount          "cancelCount"
#define XtCCancelCount          "CancelCount"
#define XtNcc                   "cc"
#define XtCCC                   "CC"
#define XtNccForward            "ccForward"
#define XtNcmdLineNntpServer    "cmdLineNntpServer"
#define XtCCmdLineNntpServer    "CmdLineNntpServer"
#define XtNconfirm              "confirm"
#define XtCConfirm              "Confirm"
#define XtNdeadLetters          "deadLetters"
#define XtCDeadLetters          "DeadLetters"
#define XtNdefaultLines         "defaultLines"
#define XtCDefaultLines         "DefaultLines"
#define XtCDebug                "Debug"
#define XtNdiscardOld           "discardOld"
#define XtCDiscardOld           "DiscardOld"
#define XtNdisplayLineCount     "displayLineCount"
#define XtCDisplayLineCount     "DisplayLineCount"
#define XtNdisplayLocalTime     "displayLocalTime"
#define XtCDisplayLocalTime     "DisplayLocalTime"
#define XtNdistribution         "distribution"
#define XtCDistribution         "Distribution"
#define XtNdomainName           "domainName"
#define XtCDomainName           "DomainName"
#define XtNdumpCore             "dumpCore"
#define XtNeditorCommand        "editorCommand"
#define XtCEditorCommand        "EditorCommand"
#define XtNexecutableSignatures "executableSignatures"
#define XtCExecutableSignatures "ExecutableSignatures"
#define XtNfullNewsrc           "fullNewsrc"
#define XtCFullNewsrc           "FullNewsrc"
#define XtNhiddenHost		"hiddenHost"
#define XtCHiddenHost		"HiddenHost"
#define XtNiconGeometry         "iconGeometry"
#define XtCIconGeometry         "IconGeometry"
#define XtNignoreNewsgroups     "ignoreNewsgroups"
#define XtCIgnoreNewsgroups     "IgnoreNewsgroups"
#define XtNincludeCommand       "includeCommand"
#define XtCIncludeCommand       "IncludeCommand"
#define XtNincludeHeader        "includeHeader"
#define XtCIncludeHeader        "IncludeHeader"
#define XtNincludePrefix        "includePrefix"
#define XtCIncludePrefix        "IncludePrefix"
#define XtNincludeSep           "includeSep"
#define XtCIncludeSep           "IncludeSep"
#define XtNinfo                 "info"
#define XtCInfo                 "Info"
#define XtNkillFileName		"killFileName"
#define XtCKillFileName		"KillFileName"
#define XtNkillFiles            "killFiles"
#define XtCKillFiles            "KillFiles"
#define XtNkillTimeout		"killTimeout"
#define XtCKillTimeout		"KillTimeout"
#define XtNleaveHeaders         "leaveHeaders"
#define XtCLeaveHeaders         "LeaveHeaders"
#define XtNlineLength           "lineLength"
#define XtCLineLength           "LineLength"
#define XtNlocalSignatures      "localSignatures"
#define XtCLocalSignatures      "LocalSignatures"
#define XtNlockFile             "lockFile"
#define XtCLockFile             "LockFile"
#define XtNmailer               "mailer"
#define XtCMailer               "Mailer"
#define XtNmaxLines             "maxLines"
#define XtCMaxLines             "MaxLines"
#define XtNmhPath               "mhPath"
#define XtCMhPath               "MhPath"
#define XtNminLines             "minLines"
#define XtCMinLines             "MinLines"
#define XtNnewsrcFile           "newsrcFile"
#define XtCNewsrcFile           "NewsrcFile"
#define XtNngButtonList         "ngButtonList"
#define XtCNgButtonList         "NgButtonList"
#define XtNnntpPort             "nntpPort"
#define XtCNntpPort             "NntpPort"
#define XtNnntpServer           "nntpServer"
#define XtCNntpServer           "NntpServer"
#define XtNonlyShow             "onlyShow"
#define XtCOnlyShow             "OnlyShow"
#define XtNorganization         "organization"
#define XtCOrganization         "Organization"
#define XtNpageArticles         "pageArticles"
#define XtCPageArticles         "PageArticles"
#define XtNpointerBackground    "pointerBackground"
#define XtCPointerBackground    "PointerBackground"
#define XtNpointerForeground    "pointerForeground"
#define XtCPointerForeground    "PointerForeground"
#define XtNprefetchMax          "prefetchMax"
#define XtCPrefetchMax          "PrefetchMax"
#define XtNprefetchMinSpeed     "prefetchMinSpeed"
#define XtCPrefetchMinSpeed     "PrefetchMinSpeed"
#define XtNprintCommand         "printCommand"
#define XtCPrintCommand         "PrintCommand"
#define XtNreplyTo              "replyTo"
#define XtCReplyTo              "ReplyTo"
#define XtNrescanOnEnter        "rescanOnEnter"
#define XtCRescanOnEnter        "RescanOnEnter"
#define XtNrescanTime           "rescanTime"
#define XtCRescanTime           "RescanTime"
#define XtNresetSave            "resetSave"
#define XtCResetSave            "ResetSave"
#define XtNsaveDir              "saveDir"
#define XtCSaveDir              "SaveDir"
#define XtNsaveMode             "saveMode"
#define XtCSaveMode             "SaveMode"
#define XtNsaveNewsrcFile       "saveNewsrcFile"
#define XtCSaveNewsrcFile       "SaveNewsrcFile"
#define XtNsavePostings         "savePostings"
#define XtCSavePostings         "SavePostings"
#define XtNsaveSentMail		"saveSentMail"
#define XtNsaveSentPostings	"saveSentPostings"
#define XtCSaveSent		"SaveSent"
#define XtNsaveString           "saveString"
#define XtCSaveString           "SaveString"
#define XtNsignatureFile        "signatureFile"
#define XtCSignatureFile        "SignatureFile"
#define XtNsignatureNotify      "signatureNotify"
#define XtCSignatureNotify      "SignatureNotify"
#define XtNsortedSubjects       "sortedSubjects"
#define XtCSortedSubjects       "SortedSubjects"
#define XtNstayInArticleMode    "stayInArticleMode"
#define XtCStayInArticleMode    "StayInArticleMode"
#define XtNstripHeaders         "stripHeaders"
#define XtCStripHeaders         "StripHeaders"
#define XtNsubjectRead          "subjectRead"
#define XtCSubjectRead          "SubjectRead"
#define XtNsubjectScrollBack    "subjectScrollBack"
#define XtCSubjectScrollBack    "SubjectScrollBack"
#define XtNtmpDir               "tmpDir"
#define XtCTmpDir               "TmpDir"
#define XtNtopLines             "topLines"
#define XtCTopLines             "TopLines"
#define XtNtypeAhead            "typeAhead"
#define XtCTypeAhead            "TypeAhead"
#define XtNunreadIconName       "unreadIconName"
#define XtCUnreadIconName       "UnreadIconName"
#define XtNunreadIconPixmap     "unreadIconPixmap"
#define XtCUnreadIconPixmap     "UnreadIconPixmap"
#define XtNupdateNewsrc         "updateNewsrc"
#define XtCUpdateNewsrc         "UpdateNewsrc"
#define XtNvalidNewsgroups      "validNewsgroups"
#define XtCValidNewsgroups      "ValidNewsgroups"
#define XtNverboseKill          "verboseKill"
#define XtCVerboseKill          "VerboseKill"
#define XtNversion              "version"
#define XtCVersion              "Version"
#define XtNwatchUnread          "watchUnread"
#define XtCWatchUnread          "WatchUnread"

/*
  I'm putting this list of resources separately becauase they're
  actually sub-resources and are handled specially.
  */
#define XtNwarnings		"warnings"
#define XtNposting		"posting"
#define XtCPosting		"Posting"
#define XtNfollowup		"followup"
#define XtCFollowup		"Followup"
#define XtNfollowupTo		"followupTo"
#define XtNcrossPost		"crossPost"


static Boolean defaultFalse = False;
static Boolean defaultTrue  = True;


/*
  This is a gross, disgusting hack to allow me to use the Xt Intrinsic
  resource-handling routines (in particular, XtGetSubresources) in
  order to extract subresources, without having to use the Xrm
  routines directly.
  */

typedef struct widgetNameList {
  String name;
  struct widgetNameList **sub_names;
  int sub_count;
  XtResource *resources;
  int resources_count;
} widgetNameList;

static void DoSubResources _ARGUMENTS((Widget, widgetNameList *));

static XtResource warningsFollowupResources[] = {
  {XtNfollowupTo, XtNfollowupTo, XtRBoolean, sizeof(Boolean),
   XtOffset(app_res, warnings.followup.followupTo), XtRBoolean,
   &app_resources.warnings.followup.followupTo},
  {XtNcrossPost, XtNcrossPost, XtRInt, sizeof(int),
   XtOffset(app_res, warnings.followup.crossPost), XtRInt,
   (XtPointer) &app_resources.warnings.followup.crossPost},
};

static widgetNameList warningsFollowupNames = {
  XtNfollowup,
  0, 0,
  warningsFollowupResources,
  XtNumber(warningsFollowupResources)
};

static XtResource warningsPostingResources[] = {
  {XtNfollowupTo, XtNfollowupTo, XtRInt, sizeof(int),
   XtOffset(app_res, warnings.posting.followupTo), XtRInt,
   (XtPointer) &app_resources.warnings.posting.followupTo},
  {XtNcrossPost, XtNcrossPost, XtRInt, sizeof(int),
   XtOffset(app_res, warnings.posting.crossPost), XtRInt,
   (XtPointer) &app_resources.warnings.posting.crossPost},
};
  
static widgetNameList warningsPostingNames = {
  XtNposting,
  0, 0,
  warningsPostingResources,
  XtNumber(warningsPostingResources)
};

static widgetNameList *warningsSubNames[] = {
  &warningsFollowupNames, &warningsPostingNames
};

static XtResource warningsResources[] = {
  {XtCFollowup, XtCFollowup, XtRBoolean, sizeof(Boolean),
   XtOffset(app_res, warnings.followup.followupTo), XtRBoolean,
   &defaultTrue},
  {XtCFollowup, XtCFollowup, XtRInt, sizeof(int),
   XtOffset(app_res, warnings.followup.crossPost), XtRImmediate,
   (XtPointer) 2},
  {XtCPosting, XtCPosting, XtRInt, sizeof(Boolean),
   XtOffset(app_res, warnings.posting.followupTo), XtRImmediate,
   (XtPointer) FOLLOWUPTO_CONFIRM},
  {XtCPosting, XtCPosting, XtRBoolean, sizeof(Boolean),
   XtOffset(app_res, warnings.posting.crossPost), XtRImmediate,
   (XtPointer) CROSSPOST_CONFIRM},
};

static widgetNameList subNames = {
  XtNwarnings,
  warningsSubNames, XtNumber(warningsSubNames),
  warningsResources, XtNumber(warningsResources)
};


app_resourceRec app_resources;
static char title[LABEL_SIZE];

/*
 * resources 'xrn' needs to get, rather than ones that the individual
 * widgets will handle
 */
static XtResource resources[] = {
    {XtNaddButtonList, XtCAddButtonList, XtRString, sizeof(char *),
     XtOffset(app_res,addButtonList), XtRString, (XtPointer) NULL},
    {XtNallButtonList, XtCAllButtonList, XtRString, sizeof(char *),
     XtOffset(app_res,allButtonList), XtRString, (XtPointer) NULL},
    {XtNartButtonList, XtCArtButtonList, XtRString, sizeof(char *),
     XtOffset(app_res,artButtonList), XtRString, (XtPointer) NULL},
    {XtNartSpecButtonList, XtCArtSpecButtonList, XtRString, sizeof(char *),
     XtOffset(app_res,artSpecButtonList), XtRString, (XtPointer) NULL},
    {XtNauthenticator, XtCAuthenticator, XtRString, sizeof(char *),
     XtOffset(app_res,authenticator), XtRString, (XtPointer) NULL},
    {XtNauthenticatorCommand, XtCAuthenticatorCommand, XtRString, sizeof(char *),
     XtOffset(app_res,authenticatorCommand), XtRString, (XtPointer) NULL},
    {XtNauthorFullName, XtCAuthorFullName, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,authorFullName), XtRBoolean, (XtPointer) &defaultTrue},
    {XtNbreakLength, XtCBreakLength, XtRInt, sizeof(int),
     XtOffset(app_res,breakLength), XtRString, (XtPointer) BREAKLENGTH},
    {XtNbusyIconName,  XtCBusyIconName,  XtRString,  sizeof(char *),
     XtOffset(app_res,busyIconName), XtRString, (XtPointer) NULL},
    {XtNbusyIconPixmap,  XtCBusyIconPixmap,  XtRBitmap,  sizeof(char *),
     XtOffset(app_res,busyIconPixmap), XtRPixmap, (XtPointer) NULL},
    {XtNbuttonsOnTop, XtCButtonsOnTop, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,buttonsOnTop), XtRBoolean, (XtPointer) &defaultFalse},
    {XtNcacheActive, XtCCacheActive, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,cacheActive), XtRBoolean, (XtPointer) &defaultFalse},
    {XtNcacheFile, XtCCacheFile, XtRString, sizeof(char *),
     XtOffset(app_res,cacheFile), XtRString, (XtPointer) CACHEFILE},
    {XtNcacheFilesMaxFiles, XtCCacheFilesMaxFiles, XtRInt, sizeof(int),
     XtOffset(app_res,cacheFilesMaxFiles), XtRImmediate, (XtPointer) 50},
    {XtNcacheFilesMaxSize, XtCCacheFilesMaxSize, XtRInt, sizeof(int),
     XtOffset(app_res,cacheFilesMaxSize), XtRImmediate, (XtPointer) 0},
#if SUPPORT_SILLY_CALVIN_ICON
    {XtNcalvin, XtCCalvin, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,calvin), XtRBoolean, (XtPointer) &defaultFalse},
#endif
    {XtNcancelCount, XtCCancelCount, XtRInt, sizeof(int),
     XtOffset(app_res,cancelCount), XtRString, (XtPointer) CANCELCOUNT},
    {XtNcc, XtCCC, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,cc), XtRBoolean, (XtPointer) &defaultFalse},
    {XtNccForward, XtCCC, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,ccForward), XtRBoolean, (XtPointer) &defaultFalse},
    {XtNcmdLineNntpServer, XtCCmdLineNntpServer, XtRString, sizeof(char *),
     XtOffset(app_res,cmdLineNntpServer), XtRString, (XtPointer) NULL},
    {XtNconfirm, XtCConfirm, XtRString, sizeof(char *),
     XtOffset(app_res,confirm), XtRString, (XtPointer) NULL},
    {XtNdeadLetters, XtCDeadLetters, XtRString, sizeof(char *),
     XtOffset(app_res,deadLetters), XtRString, (XtPointer) DEADLETTER},
    {XtNdefaultLines, XtCDefaultLines, XtRInt, sizeof(int),
     XtOffset(app_res,defaultLines), XtRString, (XtPointer) MINLINES},
    {XtNdiscardOld, XtCDiscardOld, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,discardOld), XtRBoolean, (XtPointer) &defaultFalse},
    {XtNdisplayLineCount, XtCDisplayLineCount, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,displayLineCount), XtRBoolean, (XtPointer) &defaultTrue},
    {XtNdisplayLocalTime, XtCDisplayLocalTime, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,displayLocalTime), XtRBoolean, (XtPointer) &defaultFalse},
    {XtNdistribution, XtCDistribution, XtRString, sizeof(char *),
     XtOffset(app_res,distribution), XtRString, (XtPointer) NULL},
    {XtNdomainName, XtCDomainName, XtRString, sizeof(char *),
     XtOffset(app_res,domainName), XtRString, (XtPointer) NULL},
    {XtNdumpCore, XtCDebug, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,dumpCore), XtRBoolean, (XtPointer)
#ifdef DUMPCORE
     &defaultTrue
#else
     &defaultFalse
#endif
    },
    {XtNeditorCommand, XtCEditorCommand, XtRString, sizeof(char *),
     XtOffset(app_res,editorCommand), XtRString, (XtPointer) NULL},
    {XtNexecutableSignatures, XtCExecutableSignatures, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,executableSignatures), XtRBoolean, (XtPointer) &defaultFalse},
    {XtNfullNewsrc, XtCFullNewsrc, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,fullNewsrc), XtRBoolean, (XtPointer) &defaultTrue},
    {XtNgeometry, XtCGeometry, XtRString,  sizeof(char *),
     XtOffset(app_res,geometry), XtRString, (XtPointer) NULL},
    {XtNhiddenHost, XtCHiddenHost, XtRString, sizeof(String),
     XtOffset(app_res,hiddenHost), XtRString, (XtPointer) NULL},
    {XtNiconGeometry,  XtCIconGeometry,  XtRString,  sizeof(char *),
     XtOffset(app_res,iconGeometry), XtRString, (XtPointer) NULL},
    {XtNiconName,  XtCIconName,  XtRString,  sizeof(char *),
     XtOffset(app_res,iconName), XtRString,
#ifdef SHORT_ICONNAME
     (XtPointer) "xrn"
#else
     (XtPointer) title
#endif
    },
    {XtNiconPixmap,  XtCIconPixmap,  XtRBitmap,  sizeof(char *),
     XtOffset(app_res,iconPixmap), XtRPixmap, (XtPointer) NULL},
    {XtNignoreNewsgroups, XtCIgnoreNewsgroups, XtRString, sizeof(char *),
     XtOffset(app_res,ignoreNewsgroups), XtRString, (XtPointer) NULL},
    {XtNincludeCommand, XtCIncludeCommand, XtRString, sizeof(char *),
     XtOffset(app_res,includeCommand), XtRString, (XtPointer) NULL},
    {XtNincludeHeader, XtCIncludeHeader, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,includeHeader), XtRBoolean, (XtPointer) &defaultFalse},
    {XtNincludePrefix, XtCIncludePrefix, XtRString, sizeof(char *),
     XtOffset(app_res,includePrefix), XtRString, (XtPointer) INCLUDEPREFIX},
    {XtNincludeSep, XtCIncludeSep, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,includeSep), XtRBoolean, (XtPointer) &defaultTrue},
    {XtNinfo, XtCInfo, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,info), XtRBoolean, (XtPointer) &defaultTrue},
    {XtNkillFileName, XtCKillFileName, XtRString, sizeof(String),
     XtOffset(app_res,killFileName), XtRImmediate, (XtPointer) "KILL"},
    {XtNkillFiles, XtCKillFiles, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,killFiles), XtRBoolean, (XtPointer) &defaultTrue},
    {XtNkillTimeout, XtCKillTimeout, XtRInt, sizeof(int),
     XtOffset(app_res,killTimeout), XtRImmediate, (XtPointer) 0},
    {XtNleaveHeaders, XtCLeaveHeaders, XtRString, sizeof(char *),
     XtOffset(app_res,leaveHeaders), XtRString, (XtPointer) NULL},
    {XtNlineLength, XtCLineLength, XtRInt, sizeof(int),
     XtOffset(app_res,lineLength), XtRString, (XtPointer) LINELENGTH},
    {XtNlocalSignatures, XtCLocalSignatures, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,localSignatures), XtRBoolean, (XtPointer) &defaultFalse},
    {XtNlockFile, XtCLockFile, XtRString, sizeof(char *),
     XtOffset(app_res,lockFile), XtRString, (XtPointer) "~/.xrnlock"},
    {XtNmailer, XtCMailer, XtRString, sizeof(char *),
     XtOffset(app_res,mailer), XtRString, (XtPointer) SENDMAIL},
    {XtNmaxLines, XtCMaxLines, XtRInt, sizeof(int),
     XtOffset(app_res,maxLines), XtRString, (XtPointer) MAXLINES},
    {XtNmhPath, XtCMhPath, XtRString, sizeof(String),
     XtOffset(app_res,mhPath), XtRString, (XtPointer) NULL},
    {XtNminLines, XtCMinLines, XtRInt, sizeof(int),
     XtOffset(app_res,minLines), XtRString, (XtPointer) MINLINES},
    {XtNnewsrcFile, XtCNewsrcFile, XtRString, sizeof(char *),
     XtOffset(app_res,newsrcFile), XtRString, (XtPointer) NEWSRCFILE},
    {XtNngButtonList, XtCNgButtonList, XtRString, sizeof(char *),
     XtOffset(app_res,ngButtonList), XtRString, (XtPointer) NULL},
    {XtNnntpPort, XtCNntpPort, XtRString, sizeof(char *),
     XtOffset(app_res,nntpPort), XtRString, (XtPointer) NULL},
    {XtNnntpServer, XtCNntpServer, XtRString, sizeof(char *),
     XtOffset(app_res,nntpServer), XtRString, (XtPointer) NULL},
    {XtNonlyShow, XtCOnlyShow, XtRInt, sizeof(int),
     XtOffset(app_res,onlyShow), XtRString, (XtPointer) ONLYSHOW},
    {XtNorganization, XtCOrganization, XtRString, sizeof(char *),
     XtOffset(app_res,organization), XtRString, (XtPointer) NULL},
    {XtNpageArticles, XtCPageArticles, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,pageArticles), XtRBoolean, (XtPointer) &defaultTrue},
    {XtNpointerBackground, XtCPointerBackground, XtRPixel, sizeof(Pixel),
     XtOffset(app_res,pointer_background), XtRString, XtDefaultBackground},
    {XtNpointerForeground, XtCPointerForeground, XtRPixel, sizeof(Pixel),
     XtOffset(app_res,pointer_foreground), XtRString, XtDefaultForeground},
    {XtNprefetchMax, XtCPrefetchMax, XtRInt, sizeof(int),
     XtOffset(app_res,prefetchMax), XtRString, (XtPointer) XRN_PREFETCH_MAX},
    {XtNprefetchMinSpeed, XtCPrefetchMinSpeed, XtRInt, sizeof(int),
     XtOffset(app_res,prefetchMinSpeed), XtRString, (XtPointer) "3"},
    {XtNprintCommand, XtCPrintCommand, XtRString, sizeof(char *),
     XtOffset(app_res,printCommand), XtRString, (XtPointer) PRINTCOMMAND},
    {XtNreplyTo, XtCReplyTo, XtRString, sizeof(char *),
     XtOffset(app_res,replyTo), XtRString, (XtPointer) NULL},
    {XtNrescanOnEnter, XtCRescanOnEnter, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,rescanOnEnter), XtRBoolean, (XtPointer) &defaultFalse},
    {XtNrescanTime, XtCRescanTime, XtRInt, sizeof(int),
     XtOffset(app_res,rescanTime), XtRString, (XtPointer) RESCAN_TIME},
    {XtNresetSave, XtCResetSave, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,resetSave), XtRBoolean, (XtPointer) &defaultTrue},
    {XtNsaveDir,  XtCSaveDir,  XtRString, sizeof(char *),
     XtOffset(app_res,saveDir), XtRString, (XtPointer) SAVEDIR},
    {XtNsaveMode, XtCSaveMode, XtRString, sizeof(char *),
     XtOffset(app_res,strSaveMode), XtRString, (XtPointer) SAVEMODE},
    {XtNsaveNewsrcFile, XtCSaveNewsrcFile, XtRString, sizeof(char *),
     XtOffset(app_res,saveNewsrcFile), XtRString, (XtPointer) SAVENEWSRCFILE},
    {XtNsavePostings, XtCSavePostings, XtRString, sizeof(char *),
     XtOffset(app_res,savePostings), XtRString, (XtPointer) SAVEPOSTINGS},
    {XtNsaveSentMail, XtCSaveSent, XtRString, sizeof(char *),
     XtOffset(app_res,saveSentMail), XtRString, (XtPointer) 0},
    {XtNsaveSentPostings, XtCSaveSent, XtRString, sizeof(char *),
     XtOffset(app_res,saveSentPostings), XtRString, (XtPointer) 0},
    {XtNsaveString, XtCSaveString, XtRString, sizeof(char *),
     XtOffset(app_res,saveString), XtRString, (XtPointer) NULL},
    {XtNsignatureFile, XtCSignatureFile, XtRString, sizeof(char *),
     XtOffset(app_res,signatureFile), XtRString, (XtPointer) SIGNATUREFILE},
    {XtNsignatureNotify, XtCSignatureNotify, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,signatureNotify), XtRBoolean, (XtPointer) &defaultFalse},
    {XtNsortedSubjects, XtCSortedSubjects, XtRString, sizeof(String),
     XtOffset(app_res,sortedSubjects), XtRImmediate, (XtPointer) 0},
    {XtNstayInArticleMode, XtCStayInArticleMode, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,stayInArticleMode), XtRBoolean, (XtPointer) &defaultFalse},
    {XtNstripHeaders, XtCStripHeaders, XtRString, sizeof(char *),
     XtOffset(app_res,stripHeaders), XtRString, (XtPointer) NULL},
    {XtNsubjectRead, XtCSubjectRead, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,subjectRead), XtRBoolean, (XtPointer) &defaultFalse},
    {XtNsubjectScrollBack, XtCSubjectScrollBack, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,subjectScrollBack), XtRBoolean, (XtPointer) &defaultTrue},
    {XtNtitle,  XtCTitle,  XtRString,  sizeof(char *),
     XtOffset(app_res,title), XtRString, (XtPointer) title},
    {XtNtmpDir, XtCTmpDir, XtRString, sizeof(char *),
     XtOffset(app_res,tmpDir), XtRString, (XtPointer) NULL},
    {XtNtopLines, XtCTopLines, XtRInt, sizeof(int),
     XtOffset(app_res,topLines), XtRString, (XtPointer) TOPLINES},
    {XtNtypeAhead, XtCTypeAhead, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,typeAhead), XtRBoolean, (XtPointer) &defaultTrue},
    {XtNunreadIconName,  XtCUnreadIconName,  XtRString,  sizeof(char *),
     XtOffset(app_res,unreadIconName), XtRString, (XtPointer) NULL},
    {XtNunreadIconPixmap,  XtCUnreadIconPixmap,  XtRBitmap,  sizeof(char *),
     XtOffset(app_res,unreadIconPixmap), XtRPixmap, (XtPointer) NULL},
    {XtNupdateNewsrc, XtCUpdateNewsrc, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,updateNewsrc), XtRBoolean, (XtPointer) &defaultFalse},
    {XtNvalidNewsgroups, XtCValidNewsgroups, XtRString, sizeof(char *),
     XtOffset(app_res,validNewsgroups), XtRString, (XtPointer) NULL},
    {XtNverboseKill, XtCVerboseKill, XtRString, sizeof(String),
     XtOffset(app_res,verboseKill), XtRString, (XtPointer) "jms"},
    {XtNversion, XtCVersion, XtRString, sizeof(char *),
     XtOffset(app_res,version), XtRString, (XtPointer) NULL},
    {XtNwatchUnread, XtCWatchUnread, XtRString, sizeof(char *),
     XtOffset(app_res,watchList), XtRString, (XtPointer) NULL},
};


/*
 * allowed command line options
 */
static XrmOptionDescRec optionList[] = {
    {"-addButtonList",  XtNaddButtonList,  XrmoptionSepArg,  (XtPointer) NULL},
    {"-allButtonList",  XtNallButtonList,  XrmoptionSepArg,  (XtPointer) NULL},
    {"-artButtonList",  XtNartButtonList,  XrmoptionSepArg,  (XtPointer) NULL},
    {"-artSpecButtonList",  XtNartSpecButtonList,  XrmoptionSepArg,  (XtPointer) NULL},
    {"-authenticator", XtNauthenticator, XrmoptionSepArg, (XtPointer) NULL},
    {"-authenticatorCommand", XtNauthenticatorCommand, XrmoptionSepArg, (XtPointer) NULL},
    {"-authorFullName",	XtNauthorFullName, XrmoptionNoArg,   (XtPointer) "off"},
    {"+authorFullName",	XtNauthorFullName, XrmoptionNoArg,   (XtPointer) "on"},
    {"-breakLength",    XtNbreakLength,    XrmoptionSepArg,  (XtPointer) NULL},
    {"-busyIconName",   XtNbusyIconName,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-busyIconPixmap", XtNbusyIconPixmap, XrmoptionSepArg,  (XtPointer) NULL},
    {"-cacheFile",      XtNcacheFile,      XrmoptionSepArg,  (XtPointer) NULL},
#if SUPPORT_SILLY_CALVIN_ICON
    {"-calvin",         XtNcalvin,         XrmoptionNoArg,   (XtPointer) "on"},
#endif
    {"-cancelCount",    XtNcancelCount,    XrmoptionSepArg,  (XtPointer) NULL},
    {"-cc",		XtNcc,	   	   XrmoptionNoArg,   (XtPointer) "off"},
    {"+cc",		XtNcc,	   	   XrmoptionNoArg,   (XtPointer) "on"},
    {"-ccForward",	XtNccForward,  	   XrmoptionNoArg,   (XtPointer) "off"},
    {"+ccForward",	XtNccForward,  	   XrmoptionNoArg,   (XtPointer) "on"},
    {"-confirm",        XtNconfirm,        XrmoptionSepArg,  (XtPointer) NULL},
    {"-deadLetters",    XtNdeadLetters,    XrmoptionSepArg,  (XtPointer) NULL},
    {"-defaultLines",   XtNdefaultLines,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-discardOld", XtNdiscardOld, XrmoptionNoArg, (XtPointer) "off"},
    {"+discardOld", XtNdiscardOld, XrmoptionNoArg, (XtPointer) "on"},
    {"-displayLineCount",XtNdisplayLineCount,XrmoptionNoArg, (XtPointer) "off"},
    {"+displayLineCount",XtNdisplayLineCount,XrmoptionNoArg, (XtPointer) "on"},
    {"-displayLocalTime",XtNdisplayLocalTime,XrmoptionNoArg, (XtPointer) "off"},
    {"+displayLocalTime",XtNdisplayLocalTime,XrmoptionNoArg, (XtPointer) "on"},
    {"-distribution",   XtNdistribution,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-dumpCore",	XtNdumpCore,	   XrmoptionNoArg,   (XtPointer) "off"},
    {"+dumpCore",	XtNdumpCore,	   XrmoptionNoArg,   (XtPointer) "on"},
    {"-editorCommand",  XtNeditorCommand,  XrmoptionSepArg,  (XtPointer) NULL},
    {"-executableSignatures", XtNexecutableSignatures, XrmoptionNoArg,  (XtPointer) "off"},
    {"+executableSignatures", XtNexecutableSignatures, XrmoptionNoArg,  (XtPointer) "on"},
    {"-fullNewsrc", XtNfullNewsrc, XrmoptionNoArg, (XtPointer) "off"},
    {"+fullNewsrc", XtNfullNewsrc, XrmoptionNoArg, (XtPointer) "on"},
    {"-geometry",       XtNgeometry,       XrmoptionSepArg,  (XtPointer) NULL},
    {"-iconGeometry",   XtNiconGeometry,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-iconName",       XtNiconName,       XrmoptionSepArg,  (XtPointer) NULL},
    {"-iconPixmap",     XtNiconPixmap,     XrmoptionSepArg,  (XtPointer) NULL},
    {"-ignoreNewsgroups", XtNignoreNewsgroups, XrmoptionSepArg,  (XtPointer) NULL},
    {"-includeCommand", XtNincludeCommand, XrmoptionSepArg,  (XtPointer) NULL},
    {"-includeHeader",  XtNincludeHeader,  XrmoptionNoArg,   (XtPointer) "off"},
    {"+includeHeader",  XtNincludeHeader,  XrmoptionNoArg,   (XtPointer) "on"},
    {"-includePrefix",  XtNincludePrefix,  XrmoptionSepArg,  (XtPointer) NULL},
    {"-includeSep",     XtNincludeSep,     XrmoptionNoArg,   (XtPointer) "off"},
    {"+includeSep",     XtNincludeSep,     XrmoptionNoArg,   (XtPointer) "on"},
    {"-info",    	XtNinfo,           XrmoptionNoArg,   (XtPointer) "off"},
    {"+info",    	XtNinfo,           XrmoptionNoArg,   (XtPointer) "on"},
    {"-killFiles",      XtNkillFiles,      XrmoptionNoArg,   (XtPointer) "off"},
    {"+killFiles",      XtNkillFiles,      XrmoptionNoArg,   (XtPointer) "on"},
    {"-leaveHeaders",   XtNleaveHeaders,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-lineLength",     XtNlineLength,     XrmoptionSepArg,  (XtPointer) NULL},
    {"-localSignatures", XtNlocalSignatures, XrmoptionNoArg,  (XtPointer) "off"},
    {"+localSignatures", XtNlocalSignatures, XrmoptionNoArg,  (XtPointer) "on"},
    {"-lockFile", XtNlockFile, XrmoptionSepArg,  (XtPointer) NULL},
    {"-mailer",         XtNmailer,         XrmoptionSepArg,  (XtPointer) NULL},
    {"-maxLines",       XtNmaxLines,       XrmoptionSepArg,  (XtPointer) NULL},
    {"-minLines",       XtNminLines,       XrmoptionSepArg,  (XtPointer) NULL},
    {"-newsrcFile",     XtNnewsrcFile,     XrmoptionSepArg,  (XtPointer) NULL},
    {"-ngButtonList",   XtNngButtonList,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-nntpServer",     XtNcmdLineNntpServer,     XrmoptionSepArg,  (XtPointer) NULL},
    {"-onlyShow", XtNonlyShow, XrmoptionSepArg,  (XtPointer) NULL},
    {"-organization",   XtNorganization,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-pageArticles",	XtNpageArticles,   XrmoptionNoArg,   (XtPointer) "off"},
    {"+pageArticles",	XtNpageArticles,   XrmoptionNoArg,   (XtPointer) "on"},
    {"-pointerBackground", XtNpointerBackground, XrmoptionSepArg, (XtPointer) NULL},
    {"-pointerForeground", XtNpointerForeground, XrmoptionSepArg, (XtPointer) NULL},
    {"-prefetchMax",    XtNprefetchMax,    XrmoptionSepArg,  (XtPointer) NULL},
    {"-prefetchMinSpeed", XtNprefetchMinSpeed, XrmoptionSepArg,  (XtPointer) NULL},
    {"-printCommand",   XtNprintCommand,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-replyTo",        XtNreplyTo,        XrmoptionSepArg,  (XtPointer) NULL},
    {"-rescanOnEnter",  XtNrescanOnEnter,  XrmoptionNoArg,   (XtPointer) "off"},
    {"+rescanOnEnter",  XtNrescanOnEnter,  XrmoptionNoArg,   (XtPointer) "on"},
    {"-rescanTime",	XtNrescanTime,	   XrmoptionSepArg,  (XtPointer) NULL},
    {"-resetSave", XtNresetSave, XrmoptionNoArg,  (XtPointer) "off"},
    {"+resetSave", XtNresetSave, XrmoptionNoArg,  (XtPointer) "on"},
    {"-saveDir",        XtNsaveDir,        XrmoptionSepArg,  (XtPointer) NULL},
    {"-saveMode",       XtNsaveMode,       XrmoptionSepArg,  (XtPointer) NULL},
    {"-saveNewsrcFile", XtNsaveNewsrcFile, XrmoptionSepArg,  (XtPointer) NULL},
    {"-savePostings",   XtNsavePostings,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-saveString", XtNsaveString, XrmoptionSepArg,  (XtPointer) NULL},
    {"-signatureFile",  XtNsignatureFile,  XrmoptionSepArg,  (XtPointer) NULL},
    {"-signatureNotify", XtNsignatureNotify, XrmoptionNoArg,  (XtPointer) "off"},
    {"+signatureNotify", XtNsignatureNotify, XrmoptionNoArg,  (XtPointer) "on"},
    {"-stayInArticleMode", XtNstayInArticleMode, XrmoptionNoArg, (XtPointer) "off"},
    {"+stayInArticleMode", XtNstayInArticleMode, XrmoptionNoArg, (XtPointer) "on"},
    {"-sortedSubjects", XtNsortedSubjects, XrmoptionNoArg,   (XtPointer) "off"},
    {"+sortedSubjects", XtNsortedSubjects, XrmoptionNoArg,   (XtPointer) "on"},
    {"-stripHeaders",   XtNstripHeaders,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-subjectRead",    XtNsubjectRead,    XrmoptionNoArg,   (XtPointer) "off"},
    {"+subjectRead",    XtNsubjectRead,    XrmoptionNoArg,   (XtPointer) "on"},
    {"-subjectScrollBack", XtNsubjectScrollBack, XrmoptionNoArg, (XtPointer) "off"},
    {"+subjectScrollBack", XtNsubjectScrollBack, XrmoptionNoArg, (XtPointer) "on"},
    {"-tmpDir",         XtNtmpDir,         XrmoptionSepArg,  (XtPointer) NULL},
    {"-topLines",       XtNtopLines,       XrmoptionSepArg,  (XtPointer) NULL},
    {"-typeAhead",    	XtNtypeAhead,      XrmoptionNoArg,   (XtPointer) "off"},
    {"+typeAhead",    	XtNtypeAhead,      XrmoptionNoArg,   (XtPointer) "on"},
    {"-unreadIconName", XtNunreadIconName, XrmoptionSepArg,  (XtPointer) NULL},
    {"-unreadIconPixmap",XtNunreadIconPixmap,XrmoptionSepArg,(XtPointer) NULL},
    {"-updateNewsrc",   XtNupdateNewsrc,   XrmoptionNoArg,   (XtPointer) "off"},
    {"+updateNewsrc",   XtNupdateNewsrc,   XrmoptionNoArg,   (XtPointer) "on"},
    {"-verboseKill",	XtNverboseKill,	   XrmoptionSepArg,  (XtPointer) NULL},
    {"-watchUnread",    XtNwatchUnread,    XrmoptionSepArg,  (XtPointer) NULL},
};    

static String fallback_resources[] = {
    "*Information.geometry:                  600x150",
    "*Information.pane.text*scrollVertical:  always",
    "*Information.pane.text*wrap:            word",
    "*Information.pane.box.skipAdjust:       True",
    0
};


/*
 * print out the usage message
 *
 */
static void usage _ARGUMENTS((int, char **));

static void usage(ac, av)
    int ac;
    char **av;  /* program name */
{
    int i;

    (void) printf("Unknown options:");
    for (i = 1; i <= ac - 1; i++) {
	if (index(av[i], ':') == NIL(char)) {
	    printf(" %s", av[i]);
	}
    }
    puts("\n");
    fputs("usage: ", stdout);
    puts(av[0]);
    puts(" [options] [-display host:display]");
    puts("\t-addButtonList\t\tList of Add mode buttons");
    puts("\t-allButtonList\t\tList of All mode buttons");
    puts("\t-artButtonList\t\tList of Article mode buttons (top box)");
    puts("\t-artSpecButtonList\tList of Article mode buttons (bottom box)");
    puts("\t+/-articleScrollBack\tScroll back to current article after performing\n\t\t\t\toperations on others in article mode");
    puts("\t+/-authorFullName\tUse author's fullname in article list");
    puts("\t-breakLength\t\tLength of line at which line wrapping begins");
    puts("\t-busyIconName\t\tIcon name used when busy");
    puts("\t-busyIconPixmap\t\tIcon pixmap used when busy");
    puts("\t-cacheFile file\tXRN variable/active cache filename");
    puts("\t-cancelCount number\tNumber of articles to search before popping up\n\t\t\t\tthe cancel button");
    puts("\t+/-cc\t\t\tInclude 'Cc: user' in replies");
    puts("\t+/-ccForward\t\tInclude 'Cc: user' in forwarded messages");    
    puts("\t-confirm\t\tTurn on/off confirmation boxes");
    puts("\t-deadLetters file\tFile to store failed postings/messages");
    puts("\t-defaultLines number\tDefault number of lines above cursor");
    puts("\t+/-discardOld\t\tDiscard unshown articles when onlyShow is in effect");
    puts("\t+/-displayLineCount\tDisplay line count in the subject index");
    puts("\t+/-displayLocalTime\t\tDisplay local time in the Date: field");
    puts("\t-distribution\t\tDefault distribution for messages");
#ifdef DUMPCORE
    puts("\t+/-dumpCore\t\tDump core on error exit");
#endif
    puts("\t-editorCommand\t\tEditor to use (defaults to the toolkit editor)");
    puts("\t+/-executableSignatures\tExecute signature files that are executable\n\t\t\t\tto get signature text");
    puts("\t+/-fullNewsrc\t\tNewsrc file should contain all known newsgroups");
    puts("\t-geometry WxH+X+Y\tSize and position of window");
    puts("\t-hiddenHost host\tHost name to put in \"From\" lines\n\t\t\t\tof composed messages");
    puts("\t-iconGeometry +X+Y\tPosition of icon");
    puts("\t-iconName\t\tIcon name used when unread articles");
    puts("\t-iconPixmap\t\tIcon used when unread articles");
    puts("\t-ignoreNewsgroups list\tRegexps of valid newsgroups to ignore");
    puts("\t-includeCommand\t\tCommand to use for article insertions\n\t\t\t\t(defaults to the toolkit editor)");
    puts("\t+/-includeHeader\tInclude original article's header");
    puts("\t-includePrefix\t\tPrefix for included lines");
    puts("\t+/-includeSep\t\tPut prefix in front of included lines");
    puts("\t+/-info\t\t\tPut all information in the message pane");
    puts("\t+/-killFiles\t\tTurn on/off the use of kill files");
    puts("\t-leaveHeaders list\tHeaders to leave");
    puts("\t-lineLength\t\tLength of lines for article postings");
    puts("\t+/-localSignatures\tSearch for signature files as for local\n\t\t\t\tkill files");
    puts("\t+/-lockFile\t\tname of the XRN lock file");
    puts("\t-mailer\t\t\tMailer to use");
    puts("\t-maxLines number\tMaximum number of lines above cursor (if\n\t\t\t\tnegative, minimum number below cursor)");
    puts("\t-minLines number\tMinimum number of lines above cursor");
    puts("\t-newsrcFile file\t.newsrc filename");
    puts("\t-ngButtonList\t\tList of Newsgroup mode buttons");
    puts("\t-nntpServer name\tNNTP server");
    puts("\t-onlyShow num\t\tOnly show the last num articles in a newsgroup");
    puts("\t-organization\t\tName of your organization");
    puts("\t+/-pageArticles\t\tSpacebar scrolls the current article");
    puts("\t-pointerBackground color background color of mouse cursor");
    puts("\t-pointerForeground color foreground color of mouse cursor");
    puts("\t-prefetchMax\t\tMaximum number of articles to prefetch");
    puts("\t-printCommand\t\tCommand to use to print out an article");
    puts("\t-replyTo\t\tValue used for the Reply-To field");
    puts("\t-rescanTime\t\tIdle time before checking for new articles");
    puts("\t+/-resetSave\t\tReset the save dialog string upon entering\n\t\t\t\teach newsgroup");
    puts("\t-saveDir directory\tDirectory for saving files");
    puts("\t-saveMode mode\t\tMethod of saving articles");
    puts("\t-saveNewsrcFile file\tSaved .newsrc filename");
    puts("\t-savePostings file\tFile to save postings/messages");
    puts("\t-saveString\t\tString to use in the save dialog");
    puts("\t-signatureFile file\tSignature file for posting");
    puts("\t+/-signatureNotify\tNotify user which signature file is being used");
    puts("\t+/-sortedSubjects\tSubject sorting order");
    puts("\t+/-stayInArticleMode\tSwitch to the next newsgroup whenever possible\n\t\t\t\tinstead of exiting article mode");
    puts("\t-stripHeaders list\tHeaders to strip");
    puts("\t+/-subjectRead\t\tChange default from next unread to subject next");
    puts("\t-tmpDir\t\t\tTemporary article directory");
    puts("\t-topLines number\tNumber of lines used by the top window");
    puts("\t+/-typeAhead\t\tEnable typeahead");
    puts("\t+/-unreadIconName\tIcon name used when unread articles");
    puts("\t+/-unreadIconPixmap\tIcon pixmap used when unread articles");
    puts("\t+/-updateNewsrc\t\tUpdate the .newsrc file each time a group\n\t\t\t\tis exited");
    puts("\t-verboseKill actions\tNotify user of specified actions when\n\t\t\t\tprocessing KILL files");
    puts("\t-watchUnread\t\tList of news groups to monitor");

    exit(0);
}

static void DoSubResources(w, list)
     Widget w;
     widgetNameList *list;
{
  Widget subw;

  XtGetSubresources(w, &app_resources,
		    list->name, list->name,
		    list->resources, list->resources_count,
		    0, 0);

  if (list->sub_names) {
    int i;

    subw = XtCreateWidget(list->name, shellWidgetClass, w, 0, 0);
    for (i = 0; i < list->sub_count; i++)
      DoSubResources(subw, list->sub_names[i]);
    XtDestroyWidget(subw);
  }
}


/*
 * initialize the toolkit, parse the command line, and handle the Xdefaults
 *
 *   returns: top level widget
 *
 */
Widget Initialize(argc, argv)
    int argc;
    char **argv;
{
#ifndef XDBBUG
    extern XrmDatabase XtDatabase _ARGUMENTS((Display *));
#endif /* XDBBUG */
    Widget widget;
    static Arg shell_args[] = {
	{XtNinput, (XtArgVal)True},
	{XtNiconName, (XtArgVal) NULL},
	{XtNtitle, (XtArgVal) NULL},
    };
    char *tmp_ptr = 0;

#if XtSpecificationRelease > 5
    widget = XtOpenApplication(&TopContext, XRN_APP_CLASS,
			       optionList, XtNumber(optionList),
			       &argc, argv, fallback_resources,
			       sessionShellWidgetClass, NULL, (Cardinal)0);
#else /* earlier than X11R6 */
    widget = XtAppInitialize(&TopContext, XRN_APP_CLASS,
			     optionList, XtNumber(optionList),
			     &argc, argv, fallback_resources, 0, 0);

#endif /* X11R6 or greater */

    if (argc > 1) {
	usage(argc, argv);
    }

#if defined(__DATE__) && defined(WANT_DATE)
    (void) sprintf(title, "xrn - version %s (compiled on %s)",
		   XRN_VERSION, __DATE__);
#else
    (void) sprintf(title, "xrn - version %s",
		   XRN_VERSION);
#endif

    /* get the resources needed by xrn itself */
    XtGetApplicationResources(widget, (XtPointer) &app_resources,
			      resources, XtNumber(resources), 0, 0);
    DoSubResources(widget, &subNames);

    /* 
     * check and set the lock file - must be after the application resources
     * are processed
     */
    checkLock();

    /* set up the titles */
    shell_args[1].value = (XtArgVal) app_resources.iconName;
    shell_args[2].value = (XtArgVal) app_resources.title;

    XtSetValues(widget, shell_args, XtNumber(shell_args));

    /* article saving mode */

    app_resources.saveMode = 0;
    if (utSubstring(app_resources.strSaveMode, "mailbox") == 1) {
	app_resources.saveMode |= MAILBOX_SAVE;
    } else if (utSubstring(app_resources.strSaveMode, "formfeed") == 1) {
	app_resources.saveMode |= FORMFEED_SAVE;
    } else {
	app_resources.saveMode |= NORMAL_SAVE;
    }
    if (utSubstring(app_resources.strSaveMode, "noheaders") == 1) {
	app_resources.saveMode |= NOHEADERS_SAVE;
    } else {
	app_resources.saveMode |= HEADERS_SAVE;
    }

    if (utSubstring(app_resources.strSaveMode, "subdirs") == 1) {
	app_resources.saveMode |= SUBDIRS_SAVE;
    } else if (utSubstring(app_resources.strSaveMode, "onedir") == 1) {
	app_resources.saveMode |= ONEDIR_SAVE;
    } else {
	app_resources.saveMode |= SAVE_DIR_DEFAULT;
    }

    /* header stripping mode */

    /* STRIP_HEADERS with a NIL table will leave all headers (nothing to strip) */
    app_resources.headerTree = avl_init_table(strcmp);
    if (! app_resources.headerTree) {
	 ehErrorExitXRN(ERROR_OUT_OF_MEM_MSG);
    }
    app_resources.headerMode = STRIP_HEADERS;

    /*
     * A leaveHeaders value of "all" cancels leaveHeaders, and a
     * stripHeaders value of "none" cancels stripHeaders.
     */

    if (app_resources.leaveHeaders) {
	utDowncase(app_resources.leaveHeaders);
        if (! strcmp(app_resources.leaveHeaders, "all"))
	    app_resources.leaveHeaders = NIL(char);
    }
    if (app_resources.stripHeaders) {
	utDowncase(app_resources.stripHeaders);
        if (! strcmp(app_resources.stripHeaders, "none"))
	    app_resources.stripHeaders = NIL(char);
    }

    if ((app_resources.leaveHeaders != NIL(char)) &&
	(app_resources.stripHeaders != NIL(char))) {
	ehErrorExitXRN( ERROR_STRIP_LEAVE_HEADERS_MSG );
     }  

    if (app_resources.leaveHeaders != NIL(char)) {
	char *ptr, *token;
	
	app_resources.headerMode = LEAVE_HEADERS;
	ptr = app_resources.leaveHeaders;
	while ((token = strtok(ptr, ", \t\n")) != NIL(char)) {
	    utDowncase(token);
	    if (avl_insert(app_resources.headerTree, token, (char *) 1) < 0) {
		 ehErrorExitXRN( ERROR_OUT_OF_MEM_MSG );
	    }
	    ptr = NIL(char);
	}
	
    } else if (app_resources.stripHeaders != NIL(char)) {
	char *ptr, *token;
	
	app_resources.headerMode = STRIP_HEADERS;
	ptr = app_resources.stripHeaders;
	while ((token = strtok(ptr, ", \t\n")) != NIL(char)) {
	    utDowncase(token);
	    if (avl_insert(app_resources.headerTree, token, (char *) 1) < 0) {
		 ehErrorExitXRN(ERROR_OUT_OF_MEM_MSG);
	    }
	    ptr = NIL(char);
	}
    }

    /* confirm boxes */

    app_resources.confirmMode = 0;

    if (app_resources.confirm != NIL(char)) {
	char *ptr, *token;

	ptr = app_resources.confirm;
	while ((token = strtok(ptr, ", \t\n")) != NIL(char)) {
	    if (strcmp(token, "ngQuit") == 0) {
		app_resources.confirmMode |= NG_QUIT;
	    } else if (strcmp(token, "ngExit") == 0) {
		app_resources.confirmMode |= NG_EXIT;
	    } else if (strcmp(token, "ngCatchUp") == 0) {
	      app_resources.confirmMode |= NG_CATCHUP;
	    } else if (strcmp(token, "ngGetList") == 0) {
	      app_resources.confirmMode |= NG_GETLIST;
	    } else if (strcmp(token, "artCatchUp") == 0) {
		app_resources.confirmMode |= ART_CATCHUP;
	    } else if (strcmp(token, "artFedUp") == 0) {
		app_resources.confirmMode |= ART_FEDUP;
	    } else if (strcmp(token, "ngUnsub") == 0) {
		app_resources.confirmMode |= NG_UNSUBSCRIBE;
	    } else if (strcmp(token, "artUnsub") == 0) {
		app_resources.confirmMode |= ART_UNSUBSCRIBE;
	    } else {
		mesgPane(XRN_SERIOUS, 0, UNKNOWN_CONFIRM_BUTTON_MSG, token);
	    }
	    ptr = NIL(char);
	}
    }

    /* temporary directory */

    if (! app_resources.tmpDir) {
	char *ptr = getenv("TMPDIR");

	if (ptr) {
	    app_resources.tmpDir = XtNewString(ptr);
	} else {
	    app_resources.tmpDir = XtNewString(TEMPORARY_DIRECTORY);
	}
	tmp_ptr = app_resources.tmpDir;
    }

    app_resources.tmpDir = utTildeExpand(app_resources.tmpDir);
    app_resources.tmpDir = XtNewString(app_resources.tmpDir);

    if (tmp_ptr)
	XtFree(tmp_ptr);

    if (app_resources.editorCommand && !*app_resources.editorCommand)
      app_resources.editorCommand = 0;

    /* line breaking */
    if (app_resources.editorCommand != NIL(char)) {
	app_resources.breakLength = 0;
	app_resources.lineLength = 0;
    }

    /*
      Authentication
      */
    if (getenv("NNTPAUTH"))
      app_resources.authenticator = getenv("NNTPAUTH");

    art_sort_parse_sortlist(app_resources.sortedSubjects);

    return widget;
}
