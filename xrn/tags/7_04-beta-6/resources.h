#ifndef RESOURCES_H
#define RESOURCES_H

/*
 * $Id: resources.h,v 1.21 1995-09-06 17:05:47 jik Exp $
 */

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
 * resources.h: resources for xrn
 */

#ifndef _XtIntrinsic_h
#include <X11/Intrinsic.h>
#endif

#ifndef AVL_H
#include "avl.h"
#endif

#define NAME_SIZE 1024

typedef struct {
    char* version;
    avl_tree *headerTree;
    int headerMode;
    char *geometry;
    char *iconGeometry;
    char *iconName;
    Pixmap iconPixmap;
    char *busyIconName;
    Pixmap busyIconPixmap;
    char *unreadIconName;
    Pixmap unreadIconPixmap;
    char *title;
    Pixel pointer_foreground;
    Pixel pointer_background;
    char *saveDir;
    char *expandedSaveDir;
    char *newsrcFile;
    char *saveNewsrcFile;
    char *cacheFile;
    char *signatureFile;
    Boolean signatureNotify, executableSignatures, localSignatures;
    char *nntpServer, *cmdLineNntpServer;
    int topLines;
    int saveMode;
    char *leaveHeaders;
    char *stripHeaders;
    char *savePostings;
    char *deadLetters;
    int minLines;
    int maxLines;
    int defaultLines;
    int cancelCount;
    char *mailer;
    Boolean subjectRead;
    Boolean info;
    char *tmpDir;
    char *confirm;
    int confirmMode;
    Boolean killFiles;
#if SUPPORT_SILLY_CALVIN_ICON
    Boolean calvin;
#endif
    char *editorCommand;
    char *includeCommand;
    char *strSaveMode;
    char *organization;
    char *distribution;
    char *replyTo;
    Boolean includeHeader;
    char* includePrefix;
    char *watchList;
    Boolean includeSep;
    Boolean updateNewsrc;
    int lineLength;
    int breakLength;
    int rescanTime;
    Boolean pageArticles;
    Boolean sortedSubjects;
    Boolean typeAhead;
    int prefetchMax;
    char *addButtonList;
    char *ngButtonList;
    char *allButtonList;
    char *artButtonList;
    char *artSpecButtonList;
    char *printCommand;
    Boolean dumpCore;
    String verboseKill;
    Boolean cc;
    Boolean ccForward;
    Boolean authorFullName;
#ifdef REALLY_USE_LOCALTIME
    Boolean displayLocalTime;
#endif
    Boolean displayLineCount;
    Boolean resetSave;
    char *saveString;
    char *lockFile;
    char *mhPath;
    int onlyShow;
    char *ignoreNewsgroups;
    char *domainName;
    char *authenticatorCommand;
    char *authenticator;
    Boolean rescanOnEnter, stayInArticleMode, subjectScrollBack, discardOld;
    int prefetchMinSpeed;
    Boolean fullNewsrc;
} app_resourceRec, *app_res;

extern app_resourceRec app_resources;

extern Widget Initialize _ARGUMENTS((int,char **));

/* article save options */

#define MAILBOX_SAVE   0x01
#define NORMAL_SAVE    0x02
#define HEADERS_SAVE   0x10
#define NOHEADERS_SAVE 0x20

#define ONEDIR_SAVE    0x100
#define SUBDIRS_SAVE   0x200

/* confirm box options */

#define NG_EXIT 0x01
#define NG_QUIT 0x02
#define NG_CATCHUP 0x04
#define NG_UNSUBSCRIBE 0x08
#define ART_CATCHUP 0x10
#define ART_PART_CATCHUP 0x20
#define ART_UNSUBSCRIBE 0x40
#define ART_FEDUP 0x80


/* header options */

#define STRIP_HEADERS  0
#define LEAVE_HEADERS  1

#endif /* RESOURCES_H */
