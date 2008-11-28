
#if !defined(lint) && !defined(SABER)
static char XRNrcsid[] = "$Header: /d/src/cvsroot/xrn/resources.c,v 1.8 1994-10-16 16:06:04 jik Exp $";
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
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <ctype.h>
#include "avl.h"
#include "news.h"
#include "xthelper.h"
#include "mesg.h"
#include "xrn.h"
#include "resources.h"
#include "error_hnds.h"
#include "internals.h"
#include "mesg_strings.h"

/*
 * resources and command list
 */

/* extra name and class specifications */
#define XtCgeometry       "Geometry"
#ifdef VMS
#define XtNgeometry	  "geometry"
#endif
#define XtNversion        "version"
#define XtCversion        "Version"
#define XtNiconGeometry   "iconGeometry"
#define XtCiconGeometry   "IconGeometry"
#define XtNpointerForeground "pointerForeground"
#define XtCPointerForeground "PointerForeground"
#define XtNpointerBackground "pointerBackground"
#define XtCPointerBackground "PointerBackground"
#define XtNnntpServer     "nntpServer"
#define XtCnntpServer     "NntpServer"
#define XtNnewsrcFile     "newsrcFile"
#define XtCnewsrcFile     "NewsrcFile"
#define XtNsaveNewsrcFile "saveNewsrcFile"
#define XtCsaveNewsrcFile "SaveNewsrcFile"
#define XtNsaveDir        "saveDir"
#define XtCsaveDir        "SaveDir"
#define XtNsignatureFile  "signatureFile"
#define XtCsignatureFile  "SignatureFile"
#define XtNsignatureNotify "signatureNotify"
#define XtCsignatureNotify "SignatureNotify"
#define XtNexecutableSignatures "executableSignatures"
#define XtCexecutableSignatures "ExecutableSignatures"
#define XtNlocalSignatures "localSignatures"
#define XtClocalSignatures "LocalSignatures"
#define XtNtopLines       "topLines"
#define XtCtopLines       "TopLines"
#define XtNsaveMode       "saveMode"
#define XtCsaveMode       "SaveMode"
#define XtNleaveHeaders   "leaveHeaders"
#define XtCleaveHeaders   "LeaveHeaders"
#define XtNstripHeaders   "stripHeaders"
#define XtCstripHeaders   "StripHeaders"
#define XtNignoreNewsgroups "ignoreNewsgroups"
#define XtCignoreNewsgroups "IgnoreNewsgroups"
#define XtNdeadLetters    "deadLetters"
#define XtCdeadLetters    "DeadLetters"
#define XtNsavePostings   "savePostings"
#define XtCsavePostings   "SavePostings"
#define XtNminLines       "minLines"
#define XtCminLines       "MinLines"
#define XtNmaxLines       "maxLines"
#define XtCmaxLines       "MaxLines"
#define XtNdefaultLines   "defaultLines"
#define XtCdefaultLines   "DefaultLines"
#define XtNcancelCount    "cancelCount"
#define XtCcancelCount    "CancelCount"
#define XtNmailer         "mailer"
#define XtCmailer         "Mailer"
#define XtNeditorCommand  "editorCommand"
#define XtCeditorCommand  "EditorCommand"
#ifdef WATCH
#define XtNwatchUnread  "watchUnread"
#define XtCwatchUnread  "WatchUnread"
#endif
#define XtNincludeCommand "includeCommand"
#define XtCincludeCommand "IncludeCommand"
#define XtNincludeHeader  "includeHeader"
#define XtCincludeHeader  "IncludeHeader"
#define XtNextraMailHeaders "extraMailHeaders"
#define XtCextraMailHeaders "ExtraMailHeaders"
#define XtNincludePrefix  "includePrefix"
#define XtCincludePrefix  "IncludePrefix"
#define XtNincludeSep     "includeSep"
#define XtCincludeSep     "IncludeSep"
#define XtNupdateNewsrc   "updateNewsrc"
#define XtCupdateNewsrc   "UpdateNewsrc"
#define XtNsortedSubjects "sortedSubjects"
#define XtCsortedSubjects "SortedSubjects"
#define XtNtmpDir         "tmpDir"
#define XtCtmpDir         "TmpDir"
#define XtNsubjectRead    "subjectRead"
#define XtCsubjectRead    "SubjectRead"
#define XtNinfo	  	  "info"
#define XtCInfo	  	  "Info"
#define XtNtypeAhead      "typeAhead"
#define XtCtypeAhead      "TypeAhead"
#define XtNconfirm        "confirm"
#define XtCconfirm        "Confirm"
#define XtNkillFiles      "killFiles"
#define XtCkillFiles      "KillFiles"
#if SUPPORT_SILLY_CALVIN_ICON
#define XtNcalvin         "calvin"
#define XtCcalvin         "Calvin"
#endif
#define XtNlineLength     "lineLength"
#define XtClineLength     "LineLength"
#define XtNbreakLength    "breakLength"
#define XtCbreakLength    "BreakLength"
#define XtNrescanTime	  "rescanTime"
#define XtCRescanTime	  "RescanTime"
#define XtNorganization	  "organization"
#define XtCorganization	  "Organization"
#define XtNdistribution   "distribution"
#define XtCdistribution   "Distribution"
#define XtNreplyTo	  "replyTo"
#define XtCreplyTo	  "ReplyTo"
#ifdef XRN_PREFETCH
#define XtNprefetchMax    "prefetchMax"
#define XtCprefetchMax    "PrefetchMax"
#endif
#ifdef HILITE_SUBJECT
#define XtNhighlightSubjects	"highlightSubjects"
#define XtChighlightSubjects	"HighlightSubjects"
#endif

#define XtNaddButtonList  "addButtonList"
#define XtCAddButtonList  "AddButtonList"
#define XtNngButtonList   "ngButtonList"
#define XtCNgButtonList   "NgButtonList"
#define XtNallButtonList  "allButtonList"
#define XtCAllButtonList  "AllButtonList"
#define XtNartButtonList  "artButtonList"
#define XtCArtButtonList  "ArtButtonList"
#define XtNartSpecButtonList  "artSpecButtonList"
#define XtCArtSpecButtonList  "ArtSpecButtonList"

#define XtNaddBindings	  "addBindings"
#define XtCAddBindings	  "AddBindings"
#define XtNngBindings	  "ngBindings"
#define XtCNgBindings	  "NgBindings"
#define XtNartBindings	  "artBindings"
#define XtCArtBindings	  "ArtBindings"
#define XtNallBindings	  "allBindings"
#define XtCAllBindings	  "AllBindings"

#define XtNpageArticles	  "pageArticles"
#define XtCPageArticles	  "PageArticles"
#define XtNprintCommand	  "printCommand"
#define XtCPrintCommand	  "PrintCommand"

#define XtNdumpCore	"dumpCore"
#define XtCDebug	"Debug"

#define XtNcc		"cc"
#define XtNccForward	"ccForward"
#define XtCCC		"CC"


#define XtNverboseKill    "verboseKill"
#define XtCVerboseKill    "VerboseKill"

#define XtNbusyIconPixmap   "busyIconPixmap"
#define XtCBusyIconPixmap   "BusyIconPixmap"

#define XtNbusyIconName   "busyIconName"
#define XtCBusyIconName   "BusyIconName"

#define XtNunreadIconPixmap   "unreadIconPixmap"
#define XtCUnreadIconPixmap   "UnreadIconPixmap"

#define XtNunreadIconName     "unreadIconName"
#define XtCUnreadIconName     "UnreadIconName"

#ifdef MOTIF
#define XtNuseGadgets		"useGadgets"
#define XtCUseGadgets		"UseGadgets"
#endif

#define XtNauthorFullName	"authorFullName"
#define XtCAuthorFullName	"AuthorFullName"

#ifdef REALLY_USE_LOCALTIME
#define XtNdisplayLocalTime	"displayLocalTime"
#define XtCDisplayLocalTime	"DisplayLocalTime"
#endif

#define XtNdisplayLineCount	"displayLineCount"
#define XtCDisplayLineCount	"DisplayLineCount"

#define XtNsaveString	"saveString"
#define XtCSaveString	"SaveString"
#define XtNresetSave	"resetSave"
#define XtCResetSave	"ResetSave"

#define XtNlockFile	"lockFile"
#define XtCLockFile	"LockFile"

#define XtNindexLineLength	"indexLineLength"
#define XtCIndexLineLength	"IndexLineLength"

#define XtNmhPath         "mhPath"
#define XtCMhPath         "MhPath"

#define XtNonlyShow	"onlyShow"
#define XtCOnlyShow	"OnlyShow"

static Boolean defaultFalse = False;
static Boolean defaultTrue  = True;


app_resourceRec app_resources;
static char title[LABEL_SIZE];

/*
 * resources 'xrn' needs to get, rather than ones that the individual
 * widgets will handle
 */
static XtResource resources[] = {
   {XtNgeometry, XtCgeometry, XtRString,  sizeof(char *),
      XtOffset(app_res,geometry), XtRString, (XtPointer) NULL},
   
   {XtNiconGeometry,  XtCiconGeometry,  XtRString,  sizeof(char *),
      XtOffset(app_res,iconGeometry), XtRString, (XtPointer) NULL},
   
   {XtNiconPixmap,  XtCIconPixmap,  XtRBitmap,  sizeof(char *),
      XtOffset(app_res,iconPixmap), XtRPixmap, (XtPointer) NULL},
   {XtNiconName,  XtCIconName,  XtRString,  sizeof(char *),
      XtOffset(app_res,iconName), XtRString,
#ifdef SHORT_ICONNAME
	(XtPointer) "xrn"},
#else
	(XtPointer) title},
#endif
   {XtNunreadIconPixmap,  XtCUnreadIconPixmap,  XtRBitmap,  sizeof(char *),
      XtOffset(app_res,unreadIconPixmap), XtRPixmap, (XtPointer) NULL},
   {XtNunreadIconName,  XtCUnreadIconName,  XtRString,  sizeof(char *),
      XtOffset(app_res,unreadIconName), XtRString, (XtPointer) NULL},
   
   {XtNbusyIconPixmap,  XtCBusyIconPixmap,  XtRBitmap,  sizeof(char *),
      XtOffset(app_res,busyIconPixmap), XtRPixmap, (XtPointer) NULL},
   {XtNbusyIconName,  XtCBusyIconName,  XtRString,  sizeof(char *),
      XtOffset(app_res,busyIconName), XtRString, (XtPointer) NULL},
   
   {XtNtitle,  XtCTitle,  XtRString,  sizeof(char *),
      XtOffset(app_res,title), XtRString, (XtPointer) title},
   
   {XtNpointerBackground, XtCPointerBackground, XtRPixel, sizeof(Pixel),
      XtOffset(app_res,pointer_background), XtRString, XtDefaultBackground},

   {XtNpointerForeground, XtCPointerForeground, XtRPixel, sizeof(Pixel),
      XtOffset(app_res,pointer_foreground), XtRString, XtDefaultForeground},

   {XtNnntpServer, XtCnntpServer, XtRString, sizeof(char *),
      XtOffset(app_res,nntpServer), XtRString, (XtPointer) NULL},
   
   {XtNnewsrcFile, XtCnewsrcFile, XtRString, sizeof(char *),
      XtOffset(app_res,newsrcFile), XtRString, (XtPointer) NEWSRCFILE},
   
   {XtNsaveNewsrcFile, XtCsaveNewsrcFile, XtRString, sizeof(char *),
      XtOffset(app_res,saveNewsrcFile), XtRString, (XtPointer) SAVENEWSRCFILE},
   
   {XtNsaveDir,  XtCsaveDir,  XtRString, sizeof(char *),
      XtOffset(app_res,saveDir), XtRString, (XtPointer) SAVEDIR},
   
   {XtNsignatureFile, XtCsignatureFile, XtRString, sizeof(char *),
      XtOffset(app_res,signatureFile), XtRString, (XtPointer) SIGNATUREFILE},
   
   {XtNsignatureNotify, XtCsignatureNotify, XtRBoolean, sizeof(Boolean),
      XtOffset(app_res,signatureNotify), XtRBoolean, (XtPointer) &defaultFalse},
   
   {XtNexecutableSignatures, XtCexecutableSignatures, XtRBoolean, sizeof(Boolean),
      XtOffset(app_res,executableSignatures), XtRBoolean, (XtPointer) &defaultFalse},
   
   {XtNlocalSignatures, XtClocalSignatures, XtRBoolean, sizeof(Boolean),
      XtOffset(app_res,localSignatures), XtRBoolean, (XtPointer) &defaultFalse},
   
   {XtNtopLines, XtCtopLines, XtRInt, sizeof(int),
      XtOffset(app_res,topLines), XtRString, (XtPointer) TOPLINES},
   
   {XtNsaveMode, XtCsaveMode, XtRString, sizeof(char *),
      XtOffset(app_res,strSaveMode), XtRString, (XtPointer) SAVEMODE},
   
   {XtNleaveHeaders, XtCleaveHeaders, XtRString, sizeof(char *),
      XtOffset(app_res,leaveHeaders), XtRString, (XtPointer) NULL},
   
   {XtNstripHeaders, XtCstripHeaders, XtRString, sizeof(char *),
      XtOffset(app_res,stripHeaders), XtRString, (XtPointer) NULL},
   
   {XtNignoreNewsgroups, XtCignoreNewsgroups, XtRString, sizeof(char *),
      XtOffset(app_res,ignoreNewsgroups), XtRString, (XtPointer) NULL},
   
   {XtNdeadLetters, XtCdeadLetters, XtRString, sizeof(char *),
      XtOffset(app_res,deadLetters), XtRString, (XtPointer) DEADLETTER},

   {XtNsavePostings, XtCsavePostings, XtRString, sizeof(char *),
      XtOffset(app_res,savePostings), XtRString, (XtPointer) SAVEPOSTINGS},
   
   {XtNminLines, XtCminLines, XtRInt, sizeof(int),
      XtOffset(app_res,minLines), XtRString, (XtPointer) MINLINES},

   {XtNmaxLines, XtCmaxLines, XtRInt, sizeof(int),
      XtOffset(app_res,maxLines), XtRString, (XtPointer) MAXLINES},
   
   {XtNdefaultLines, XtCdefaultLines, XtRInt, sizeof(int),
      XtOffset(app_res,defaultLines), XtRString, (XtPointer) MINLINES},

   {XtNcancelCount, XtCcancelCount, XtRInt, sizeof(int),
      XtOffset(app_res,cancelCount), XtRString, (XtPointer) CANCELCOUNT},

   {XtNmailer, XtCmailer, XtRString, sizeof(char *),
      XtOffset(app_res,mailer), XtRString, (XtPointer) SENDMAIL},
   
   {XtNeditorCommand, XtCeditorCommand, XtRString, sizeof(char *),
      XtOffset(app_res,editorCommand), XtRString, (XtPointer) NULL},

   {XtNincludeCommand, XtCincludeCommand, XtRString, sizeof(char *),
      XtOffset(app_res,includeCommand), XtRString, (XtPointer) NULL},

   {XtNincludeHeader, XtCincludeHeader, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,includeHeader), XtRBoolean, (XtPointer) &defaultFalse},

   {XtNextraMailHeaders, XtCextraMailHeaders, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,extraMailHeaders), XtRBoolean, (XtPointer) &defaultFalse},

   {XtNversion, XtCversion, XtRString, sizeof(char *),
     XtOffset(app_res,version), XtRString, (XtPointer) NULL},

   {XtNincludePrefix, XtCincludePrefix, XtRString, sizeof(char *),
      XtOffset(app_res,includePrefix), XtRString, (XtPointer) INCLUDEPREFIX},

   {XtNincludeSep, XtCincludeSep, XtRBoolean, sizeof(Boolean),
      XtOffset(app_res,includeSep), XtRBoolean, (XtPointer) &defaultTrue},

   {XtNupdateNewsrc, XtCupdateNewsrc, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,updateNewsrc), XtRBoolean, (XtPointer) &defaultFalse},

   {XtNsortedSubjects, XtCsortedSubjects, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,sortedSubjects), XtRBoolean, (XtPointer) &defaultFalse},

   {XtNtmpDir, XtCtmpDir, XtRString, sizeof(char *),
      XtOffset(app_res,tmpDir), XtRString, (XtPointer) NULL},

   {XtNsubjectRead, XtCsubjectRead, XtRBoolean, sizeof(Boolean),
      XtOffset(app_res,subjectRead), XtRBoolean, (XtPointer) &defaultFalse},
   
   {XtNinfo, XtCInfo, XtRBoolean, sizeof(Boolean),
      XtOffset(app_res,info), XtRBoolean, (XtPointer) &defaultTrue},
   
   {XtNtypeAhead, XtCtypeAhead, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,typeAhead), XtRBoolean, (XtPointer) &defaultTrue},

   {XtNconfirm, XtCconfirm, XtRString, sizeof(char *),
      XtOffset(app_res,confirm), XtRString, (XtPointer) NULL},

   {XtNkillFiles, XtCkillFiles, XtRBoolean, sizeof(Boolean),
      XtOffset(app_res,killFiles), XtRBoolean, (XtPointer) &defaultTrue},

#if SUPPORT_SILLY_CALVIN_ICON
   {XtNcalvin, XtCcalvin, XtRBoolean, sizeof(Boolean),
      XtOffset(app_res,calvin), XtRBoolean, (XtPointer) &defaultFalse},
#endif

   {XtNlineLength, XtClineLength, XtRInt, sizeof(int),
      XtOffset(app_res,lineLength), XtRString, (XtPointer) LINELENGTH},
   
   {XtNbreakLength, XtCbreakLength, XtRInt, sizeof(int),
      XtOffset(app_res,breakLength), XtRString, (XtPointer) BREAKLENGTH},

   {XtNrescanTime, XtCRescanTime, XtRInt, sizeof(int),
      XtOffset(app_res,rescanTime), XtRString, (XtPointer) RESCAN_TIME},

   {XtNorganization, XtCorganization, XtRString, sizeof(char *),
      XtOffset(app_res,organization), XtRString, (XtPointer) NULL},
   
   {XtNdistribution, XtCdistribution, XtRString, sizeof(char *),
      XtOffset(app_res,distribution), XtRString, (XtPointer) NULL},
   
   {XtNreplyTo, XtCreplyTo, XtRString, sizeof(char *),
      XtOffset(app_res,replyTo), XtRString, (XtPointer) NULL},
   
#ifdef XRN_PREFETCH
   {XtNprefetchMax, XtCprefetchMax, XtRInt, sizeof(int),
       XtOffset(app_res,prefetchMax), XtRString, (XtPointer) XRN_PREFETCH_MAX},
#endif

#ifdef HILITE_SUBJECT
   {XtNhighlightSubjects, XtChighlightSubjects, XtRBoolean, sizeof(Boolean),
       XtOffset(app_res,highlightSubjects), XtRBoolean, (XtPointer) &defaultFalse},
#endif

   {XtNaddButtonList, XtCAddButtonList, XtRString, sizeof(char *),
      XtOffset(app_res,addButtonList), XtRString, (XtPointer) NULL},

   {XtNngButtonList, XtCNgButtonList, XtRString, sizeof(char *),
      XtOffset(app_res,ngButtonList), XtRString, (XtPointer) NULL},

   {XtNallButtonList, XtCAllButtonList, XtRString, sizeof(char *),
      XtOffset(app_res,allButtonList), XtRString, (XtPointer) NULL},

   {XtNartButtonList, XtCArtButtonList, XtRString, sizeof(char *),
      XtOffset(app_res,artButtonList), XtRString, (XtPointer) NULL},

   {XtNartSpecButtonList, XtCArtSpecButtonList, XtRString, sizeof(char *),
      XtOffset(app_res,artSpecButtonList), XtRString, (XtPointer) NULL},

   {XtNaddBindings, XtCAddBindings, XtRString, sizeof(char *),
      XtOffset(app_res,addBindings), XtRString, (XtPointer) NULL},

   {XtNngBindings, XtCNgBindings, XtRString, sizeof(char *),
      XtOffset(app_res,ngBindings), XtRString, (XtPointer) NULL},

   {XtNartBindings, XtCArtBindings, XtRString, sizeof(char *),
      XtOffset(app_res,artBindings), XtRString, (XtPointer) NULL},

   {XtNallBindings, XtCAllBindings, XtRString, sizeof(char *),
      XtOffset(app_res,allBindings), XtRString, (XtPointer) NULL},

   {XtNpageArticles, XtCPageArticles, XtRBoolean, sizeof(Boolean),
      XtOffset(app_res,pageArticles), XtRBoolean, (XtPointer) &defaultTrue},

   {XtNprintCommand, XtCPrintCommand, XtRString, sizeof(char *),
      XtOffset(app_res,printCommand), XtRString, (XtPointer) PRINTCOMMAND},

   {XtNdumpCore, XtCDebug, XtRBoolean, sizeof(Boolean),
      XtOffset(app_res,dumpCore), XtRBoolean, (XtPointer)
#ifdef DUMPCORE
							&defaultTrue},
#else
							&defaultFalse},
#endif
   {XtNcc, XtCCC, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,cc), XtRBoolean, (XtPointer) &defaultFalse},
   {XtNccForward, XtCCC, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,ccForward), XtRBoolean, (XtPointer) &defaultFalse},
   {XtNverboseKill, XtCVerboseKill, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,verboseKill), XtRBoolean, (XtPointer) &defaultTrue},
#ifdef MOTIF
   {XtNuseGadgets, XtCUseGadgets, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,useGadgets), XtRBoolean, (XtPointer) &defaultFalse},
#endif
   {XtNauthorFullName, XtCAuthorFullName, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,authorFullName), XtRBoolean, (XtPointer) &defaultTrue},
#ifdef REALLY_USE_LOCALTIME
   {XtNdisplayLocalTime, XtCDisplayLocalTime, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,displayLocalTime), XtRBoolean, (XtPointer) &defaultTrue},
#endif
   {XtNdisplayLineCount, XtCDisplayLineCount, XtRBoolean, sizeof(Boolean),
     XtOffset(app_res,displayLineCount), XtRBoolean, (XtPointer) &defaultTrue},

   {XtNsaveString, XtCSaveString, XtRString, sizeof(char *),
      XtOffset(app_res,saveString), XtRString, (XtPointer) NULL},
   {XtNresetSave, XtCResetSave, XtRBoolean, sizeof(Boolean),
      XtOffset(app_res,resetSave), XtRBoolean, (XtPointer) &defaultTrue},
   {XtNmhPath, XtCMhPath, XtRString, sizeof(String),
      XtOffset(app_res,mhPath), XtRString, (XtPointer) NULL},

   {XtNlockFile, XtCLockFile, XtRString, sizeof(char *),
      XtOffset(app_res,lockFile), XtRString, (XtPointer) "~/.xrnlock"},

   {XtNonlyShow, XtCOnlyShow, XtRInt, sizeof(int),
      XtOffset(app_res,onlyShow), XtRString, (XtPointer) ONLYSHOW},

   {XtNindexLineLength, XtCIndexLineLength, XtRInt, sizeof(int),
      XtOffset(app_res,indexLineLength), XtRString, (XtPointer) "80"},

#ifdef WATCH
   {XtNwatchUnread, XtCwatchUnread, XtRString, sizeof(char *),
      XtOffset(app_res,watchList), XtRString, (XtPointer) NULL},
#endif
};


/*
 * allowed command line options
 */
static XrmOptionDescRec optionList[] = {
    {"-geometry",       XtNgeometry,       XrmoptionSepArg,  (XtPointer) NULL},
    {"-iconGeometry",   XtNiconGeometry,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-pointerBackground", XtNpointerBackground, XrmoptionSepArg, (XtPointer) NULL},
    {"-pointerForeground", XtNpointerForeground, XrmoptionSepArg, (XtPointer) NULL},
    {"-nntpServer",     XtNnntpServer,     XrmoptionSepArg,  (XtPointer) NULL},
    {"-newsrcFile",     XtNnewsrcFile,     XrmoptionSepArg,  (XtPointer) NULL},
    {"-saveNewsrcFile", XtNsaveNewsrcFile, XrmoptionSepArg,  (XtPointer) NULL},
    {"-saveDir",        XtNsaveDir,        XrmoptionSepArg,  (XtPointer) NULL},
    {"-signatureFile",  XtNsignatureFile,  XrmoptionSepArg,  (XtPointer) NULL},
    {"-signatureNotify", XtNsignatureNotify, XrmoptionNoArg,  (XtPointer) "off"},
    {"+signatureNotify", XtNsignatureNotify, XrmoptionNoArg,  (XtPointer) "on"},
    {"-executableSignatures", XtNexecutableSignatures, XrmoptionNoArg,  (XtPointer) "off"},
    {"+executableSignatures", XtNexecutableSignatures, XrmoptionNoArg,  (XtPointer) "on"},
    {"-localSignatures", XtNlocalSignatures, XrmoptionNoArg,  (XtPointer) "off"},
    {"+localSignatures", XtNlocalSignatures, XrmoptionNoArg,  (XtPointer) "on"},
    {"-topLines",       XtNtopLines,       XrmoptionSepArg,  (XtPointer) NULL},
    {"-saveMode",       XtNsaveMode,       XrmoptionSepArg,  (XtPointer) NULL},
    {"-leaveHeaders",   XtNleaveHeaders,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-stripHeaders",   XtNstripHeaders,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-ignoreNewsgroups", XtNignoreNewsgroups, XrmoptionSepArg,  (XtPointer) NULL},
    {"-deadLetters",    XtNdeadLetters,    XrmoptionSepArg,  (XtPointer) NULL},
    {"-savePostings",   XtNsavePostings,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-minLines",       XtNminLines,       XrmoptionSepArg,  (XtPointer) NULL},
    {"-maxLines",       XtNmaxLines,       XrmoptionSepArg,  (XtPointer) NULL},
    {"-defaultLines",   XtNdefaultLines,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-cancelCount",    XtNcancelCount,    XrmoptionSepArg,  (XtPointer) NULL},
    {"-mailer",         XtNmailer,         XrmoptionSepArg,  (XtPointer) NULL},
    {"-editorCommand",  XtNeditorCommand,  XrmoptionSepArg,  (XtPointer) NULL},
#ifdef WATCH
    {"-watchUnread",    XtNwatchUnread,    XrmoptionSepArg,  (XtPointer) NULL},
#endif
    {"-includeCommand", XtNincludeCommand, XrmoptionSepArg,  (XtPointer) NULL},
    {"-includeHeader",  XtNincludeHeader,  XrmoptionNoArg,   (XtPointer) "off"},
    {"+includeHeader",  XtNincludeHeader,  XrmoptionNoArg,   (XtPointer) "on"},
    {"-extraMailHeaders",XtNextraMailHeaders,XrmoptionNoArg, (XtPointer) "off"},
    {"+extraMailHeaders",XtNextraMailHeaders,XrmoptionNoArg, (XtPointer) "on"},
    {"-includePrefix",  XtNincludePrefix,  XrmoptionSepArg,  (XtPointer) NULL},
    {"-includeSep",     XtNincludeSep,     XrmoptionNoArg,   (XtPointer) "off"},
    {"+includeSep",     XtNincludeSep,     XrmoptionNoArg,   (XtPointer) "on"},
    {"-updateNewsrc",   XtNupdateNewsrc,   XrmoptionNoArg,   (XtPointer) "off"},
    {"+updateNewsrc",   XtNupdateNewsrc,   XrmoptionNoArg,   (XtPointer) "on"},
    {"-sortedSubjects", XtNsortedSubjects, XrmoptionNoArg,   (XtPointer) "off"},
    {"+sortedSubjects", XtNsortedSubjects, XrmoptionNoArg,   (XtPointer) "on"},
    {"-tmpDir",         XtNtmpDir,         XrmoptionSepArg,  (XtPointer) NULL},
    {"-subjectRead",    XtNsubjectRead,    XrmoptionNoArg,   (XtPointer) "off"},
    {"+subjectRead",    XtNsubjectRead,    XrmoptionNoArg,   (XtPointer) "on"},
    {"-info",    	XtNinfo,           XrmoptionNoArg,   (XtPointer) "off"},
    {"+info",    	XtNinfo,           XrmoptionNoArg,   (XtPointer) "on"},
    {"-typeAhead",    	XtNtypeAhead,      XrmoptionNoArg,   (XtPointer) "off"},
    {"+typeAhead",    	XtNtypeAhead,      XrmoptionNoArg,   (XtPointer) "on"},
    {"-confirm",        XtNconfirm,        XrmoptionSepArg,  (XtPointer) NULL},
    {"-killFiles",      XtNkillFiles,      XrmoptionNoArg,   (XtPointer) "off"},
    {"+killFiles",      XtNkillFiles,      XrmoptionNoArg,   (XtPointer) "on"},
#if SUPPORT_SILLY_CALVIN_ICON
    {"-calvin",         XtNcalvin,         XrmoptionNoArg,   (XtPointer) "on"},
#endif
    {"-lineLength",     XtNlineLength,     XrmoptionSepArg,  (XtPointer) NULL},
    {"-breakLength",    XtNbreakLength,    XrmoptionSepArg,  (XtPointer) NULL},
    {"-indexLineLength",XtNindexLineLength,XrmoptionSepArg,  (XtPointer) NULL},
    {"-rescanTime",	XtNrescanTime,	   XrmoptionSepArg,  (XtPointer) NULL},
    {"-organization",   XtNorganization,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-replyTo",        XtNreplyTo,        XrmoptionSepArg,  (XtPointer) NULL},
#ifdef XRN_PREFETCH
    {"-prefetchMax",    XtNprefetchMax,    XrmoptionSepArg,  (XtPointer) NULL},
#endif
#ifdef HILITE_SUBJECT
    {"-highlightSubjects", XtNhighlightSubjects, XrmoptionNoArg, (XtPointer) "off"},
    {"+highlightSubjects", XtNhighlightSubjects, XrmoptionNoArg, (XtPointer) "on"},
#endif
    {"-addButtonList",  XtNaddButtonList,  XrmoptionSepArg,  (XtPointer) NULL},
    {"-ngButtonList",   XtNngButtonList,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-allButtonList",  XtNallButtonList,  XrmoptionSepArg,  (XtPointer) NULL},
    {"-artButtonList",  XtNartButtonList,  XrmoptionSepArg,  (XtPointer) NULL},
    {"-artSpecButtonList",  XtNartSpecButtonList,  XrmoptionSepArg,  (XtPointer) NULL},
    {"-addBindings",    XtNaddBindings,    XrmoptionSepArg,  (XtPointer) NULL},
    {"-ngBindings",     XtNngBindings,     XrmoptionSepArg,  (XtPointer) NULL},
    {"-artBindings",    XtNartBindings,    XrmoptionSepArg,  (XtPointer) NULL},
    {"-allBindings",    XtNallBindings,    XrmoptionSepArg,  (XtPointer) NULL},
    {"-pageArticles",	XtNpageArticles,   XrmoptionNoArg,   (XtPointer) "off"},
    {"+pageArticles",	XtNpageArticles,   XrmoptionNoArg,   (XtPointer) "on"},
    {"-printCommand",   XtNprintCommand,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-distribution",   XtNdistribution,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-dumpCore",	XtNdumpCore,	   XrmoptionNoArg,   (XtPointer) "off"},
    {"+dumpCore",	XtNdumpCore,	   XrmoptionNoArg,   (XtPointer) "on"},
    {"-cc",		XtNcc,	   	   XrmoptionNoArg,   (XtPointer) "off"},
    {"+cc",		XtNcc,	   	   XrmoptionNoArg,   (XtPointer) "on"},
    {"-ccForward",	XtNccForward,  	   XrmoptionNoArg,   (XtPointer) "off"},
    {"+ccForward",	XtNccForward,  	   XrmoptionNoArg,   (XtPointer) "on"},
    {"-iconName",       XtNiconName,       XrmoptionSepArg,  (XtPointer) NULL},
    {"-iconPixmap",     XtNiconPixmap,     XrmoptionSepArg,  (XtPointer) NULL},
    {"-busyIconName",   XtNbusyIconName,   XrmoptionSepArg,  (XtPointer) NULL},
    {"-busyIconPixmap", XtNbusyIconPixmap, XrmoptionSepArg,  (XtPointer) NULL},
    {"-unreadIconName", XtNunreadIconName, XrmoptionSepArg,  (XtPointer) NULL},
    {"-unreadIconPixmap",XtNunreadIconPixmap,XrmoptionSepArg,(XtPointer) NULL},
    {"-verboseKill",	XtNverboseKill,	   XrmoptionNoArg,   (XtPointer) "off"},
    {"+verboseKill",	XtNverboseKill,	   XrmoptionNoArg,   (XtPointer) "on"},
#ifdef MOTIF
    {"-useGadgets",     XtNuseGadgets,     XrmoptionNoArg,   (XtPointer) "off"},
    {"+useGadgets",     XtNuseGadgets,     XrmoptionNoArg,   (XtPointer) "on"},
#endif
    {"-authorFullName",	XtNauthorFullName, XrmoptionNoArg,   (XtPointer) "off"},
    {"+authorFullName",	XtNauthorFullName, XrmoptionNoArg,   (XtPointer) "on"},
#ifdef REALLY_USE_LOCALTIME
    {"-displayLocalTime",XtNdisplayLocalTime,XrmoptionNoArg, (XtPointer) "off"},
    {"+displayLocalTime",XtNdisplayLocalTime,XrmoptionNoArg, (XtPointer) "on"},
#endif
    {"-displayLineCount",XtNdisplayLineCount,XrmoptionNoArg, (XtPointer) "off"},
    {"+displayLineCount",XtNdisplayLineCount,XrmoptionNoArg, (XtPointer) "on"},
    {"-saveString", XtNsaveString, XrmoptionSepArg,  (XtPointer) NULL},
    {"-resetSave", XtNresetSave, XrmoptionNoArg,  (XtPointer) "off"},
    {"+resetSave", XtNresetSave, XrmoptionNoArg,  (XtPointer) "on"},
    {"-lockFile", XtNlockFile, XrmoptionSepArg,  (XtPointer) NULL},
    {"-onlyShow", XtNonlyShow, XrmoptionSepArg,  (XtPointer) NULL},
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
    puts("\t-addBindings\t\tAdd mode bindings");
    puts("\t-addButtonList\t\tList of Add mode buttons");
    puts("\t-allBindings\t\tAll mode bindings");
    puts("\t-allButtonList\t\tList of All mode buttons");
    puts("\t-artBindings\t\tArticle mode bindings");
    puts("\t-artButtonList\t\tList of Article mode buttons (top box)");
    puts("\t-artSpecButtonList\tList of Article mode buttons (bottom box)");
    puts("\t+/-authorFullName\tUse author's fullname in article list");
    puts("\t-breakLength\t\tLength of line at which line wrapping begins");
    puts("\t-busyIconName\t\tIcon name used when busy");
    puts("\t-busyIconPixmap\t\tIcon pixmap used when busy");
    puts("\t-cancelCount number\tNumber of articles to search before popping up\n\t\t\t\tthe cancel button");
    puts("\t+/-cc\t\t\tInclude 'Cc: user' in replies");
    puts("\t+/-ccForward\t\tInclude 'Cc: user' in forwarded messages");    
    puts("\t-confirm\t\tTurn on/off confirmation boxes");
    puts("\t-deadLetters file\tFile to store failed postings/messages");
    puts("\t-defaultLines number\tDefault number of lines above cursor");
    puts("\t+/-displayLineCount\tDisplay line count in the subject index");
#ifdef REALLY_USE_LOCALTIME
    puts("\t+/-displayLocalTime\t\tDisplay local time in the Date: field");
#endif
    puts("\t-distribution\t\tDefault distribution for messages");
#ifdef DUMPCORE
    puts("\t+/-dumpCore\t\tDump core on error exit");
#endif
    puts("\t-editorCommand\t\tEditor to use (defaults to the toolkit editor)");
    puts("\t+/-extraMailHeaders\tGenerate additional news header (X-Newsgroups)");
    puts("\t-geometry WxH+X+Y\tSize and position of window");
#ifdef HILITE_SUBJECT
    puts("\t-highlightSubjects\tHighlight Subject lines in displayed postings");
#endif
    puts("\t-iconGeometry +X+Y\tPosition of icon");
    puts("\t-iconName\t\tIcon name used when unread articles");
    puts("\t-iconPixmap\t\tIcon used when unread articles");
    puts("\t-includeCommand\t\tCommand to use for article insertions\n\t\t\t\t(defaults to the toolkit editor)");
    puts("\t+/-includeHeader\tInclude original article's header");
    puts("\t-includePrefix\t\tPrefix for included lines");
    puts("\t+/-includeSep\t\tPut prefix in front of included lines");
    puts("\t+/-info\t\t\tPut all information in the message pane");
    puts("\t+/-killFiles\t\tTurn on/off the use of kill files");
    puts("\t-leaveHeaders list\tHeaders to leave");
    puts("\t-lineLength\t\tLength of lines for article postings");
    puts("\t+/-lockFile\t\tname of the XRN lock file");
    puts("\t-mailer\t\t\tMailer to use");
    puts("\t-maxLines number\tMaximum number of lines above cursor");
    puts("\t-minLines number\tMinimum number of lines above cursor");
    puts("\t-newsrcFile file\t.newsrc filename");
    puts("\t-ngBindings\t\tNewsgroup mode bindings");
    puts("\t-ngButtonList\t\tList of Newsgroup mode buttons");
    puts("\t-nntpServer name\tNNTP server");
    puts("\t-onlyShow\t\tMark all but the last N articles in each\n\t\t\t\tgroup as read");
    puts("\t-organization\t\tName of your organization");
    puts("\t+/-pageArticles\t\tSpacebar scrolls the current article");
    puts("\t-pointerBackground color background color of mouse cursor");
    puts("\t-pointerForeground color foreground color of mouse cursor");
#ifdef XRN_PREFETCH
    puts("\t-prefetchMax\t\tMaximum number of articles to prefetch");
#endif
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
    puts("\t+/-executableSignatures\tExecute signature files that are executable\n\t\t\t\tto get signature text");
    puts("\t+/-localSignatures\tSearch for signature files as for local\n\t\t\t\tkill files");
    puts("\t+/-sortedSubjects\tSort or do not sort the subjects");
    puts("\t-stripHeaders list\tHeaders to strip");
    puts("\t-ignoreNewsgroups list\tRegexps of newsgroups to ignore");
    puts("\t+/-subjectRead\t\tChange default from next unread to subject next");
    puts("\t-tmpDir\t\t\tTemporary article directory");
    puts("\t-topLines number\tNumber of lines used by the top window");
    puts("\t+/-typeAhead\t\tEnable typeahead");
    puts("\t+/-unreadIconName\tIcon name used when unread articles");
    puts("\t+/-unreadIconPixmap\tIcon pixmap used when unread articles");
    puts("\t+/-updateNewsrc\t\tUpdate the .newsrc file each time a group\n\t\t\t\tis exited");
#ifdef MOTIF
    puts("\t+/-useGadgets\t\tUse Motif gadget buttons instead of widgets");
#endif
    puts("\t+/-verboseKill\t\tList subjects when killing articles");
#ifdef WATCH
    puts("\t-watchUnread\t\tList of news groups to monitor");
#endif
    exit(0);
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
    char *ptr;
    static Arg shell_args[] = {
	{XtNinput, (XtArgVal)True},
	{XtNiconName, (XtArgVal) NULL},
	{XtNtitle, (XtArgVal) NULL},
    };


    /* set up the program name */
#ifndef VMS
    if ((ptr = rindex(argv[0], '/')) == NIL(char)) {
	/* (void) strncpy(app_resources.progName, argv[0],
		       sizeof(app_resources.progName)); */
	app_resources.progName = argv[0];
    } else {
	/* (void) strncpy(app_resources.progName, ++ptr,
		       sizeof(app_resources.progName)); */
	app_resources.progName = ++ptr;
    }
#else
    /* (void) strcpy(app_resources.progName, "xrn"); */
    app_resources.progName = "xrn";
#endif

#ifdef VMS
    {
	char *ptr;
	int i, num_opts;

	num_opts = sizeof(optionList) / sizeof(XrmOptionDescRec);

	for (i = 0; i < num_opts; i++) {
	    for (ptr = optionList[i].option; *ptr; ptr++) {
		if (isupper(*ptr)) {
		    /* XXX Note that this won't work if the VMS
		       constant strings are put into read-only memory.
		       If that's the case, then the "ptr =
		       optionList[i]" above should be changed to "ptr
		       = XtNewString(optionList[i])".  However, I have
		       no way to test this. */
		    *ptr = tolower(*ptr);
		}
	    }
	}
    }
#endif /* VMS */

#if XtSpecificationRelease > 5
    widget = XtOpenApplication(NULL,
#ifdef MOTIF
			       "XRnMotif",
#else
			       "XRn",
#endif /* MOTIF */
			       optionList, XtNumber(optionList),
			       &argc, argv, NULL,
			       sessionShellWidgetClass, NULL, (Cardinal)0);
#else /* earlier than X11R6 */
    widget = XtInitialize(app_resources.progName,
#ifdef MOTIF
			  "XRnMotif",
#else
			  "XRn",
#endif /* MOTIF */
			  optionList, XtNumber(optionList),
			  &argc, argv);
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

#define ariN app_resources.ignoreNewsgroups
#define ariNL app_resources.ignoreNewsgroupsList

    if (ariN) {
	char *str = XtNewString(ariN);
	int i = 1;
	
	if (! (strtok(str, ", \n\t"))) {
	    XtFree(str);
	    goto no_ignored_newsgroups;
	}
	while (strtok((char *) 0, ", \n\t")) {
	    i++;
	}

	ariNL = (char **) XtMalloc(sizeof(char *) * (i + 1));
	if (! ariNL) {
	    ehErrorExitXRN("out of memory");
	}

	XtFree(str);

	for (i = 0, str = strtok(ariN, ", \n\t"); str;
	    str = strtok((char *) 0, ", \n\t")) {
	    ariNL[i++] = str;
#ifdef DEBUG
	    fprintf(stderr, "Ignoring \"%s\" groups.\n", str);
#endif
	}
	ariNL[i++] = 0;
    }
    else {
no_ignored_newsgroups:
	ariNL = (char **) XtMalloc(sizeof(char *));
	ariNL[0] = 0;
    }

#undef ariN
#undef ariNL

    /* header stripping mode */

    /* STRIP_HEADERS with a NIL table will leave all headers (nothing to strip) */
    app_resources.headerTree = avl_init_table(strcmp);
    if (! app_resources.headerTree) {
	 ehErrorExitXRN("out of memory");
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
	ehErrorExitXRN("Only one of 'stripHeaders, leaveHeaders' allowed\n");
     }  

    if (app_resources.leaveHeaders != NIL(char)) {
	char *ptr, *token;
	
	app_resources.headerMode = LEAVE_HEADERS;
	ptr = app_resources.leaveHeaders;
	while ((token = strtok(ptr, ", \t\n")) != NIL(char)) {
	    utDowncase(token);
	    if (avl_insert(app_resources.headerTree, token, (char *) 1) < 0) {
		 ehErrorExitXRN("out of memory");
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
		 ehErrorExitXRN("out of memory");
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
	    } else if (strcmp(token, "artCatchUp") == 0) {
		app_resources.confirmMode |= ART_CATCHUP;
	    } else if (strcmp(token, "artFedUp") == 0) {
		app_resources.confirmMode |= ART_FEDUP;
	    } else if (strcmp(token, "ngUnsub") == 0) {
		app_resources.confirmMode |= NG_UNSUBSCRIBE;
	    } else if (strcmp(token, "artUnsub") == 0) {
		app_resources.confirmMode |= ART_UNSUBSCRIBE;
	    } else {
		mesgPane(XRN_SERIOUS, UNKNOWN_CONFIRM_BUTTON_MSG, token);
	    }
	    ptr = NIL(char);
	}
    }

    /* temporary directory */

    if (app_resources.tmpDir == NIL(char)) {
	char *ptr = getenv("TMPDIR");

	if (ptr == NIL(char)) {
	    /* 
	     * XXX added to deal with a possible compiler problem on
	     * the IBM RT running AOS using the hc2.1s compiler
	     * (reported by Jay Ford <jnford@jay.weeg.uiowa.edu>).
	     */
	    char *tmp_ptr = TEMPORARY_DIRECTORY;
	    app_resources.tmpDir = XtNewString(tmp_ptr);
	} else {
	    app_resources.tmpDir = XtNewString(ptr);
	}
    }

    app_resources.tmpDir = XtNewString(utTildeExpand(app_resources.tmpDir));

    /* line breaking */
    if (app_resources.editorCommand != NIL(char)) {
	app_resources.breakLength = 0;
	app_resources.lineLength = 0;
    }

    return widget;
}
