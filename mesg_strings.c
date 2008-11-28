#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: mesg_strings.c,v 1.65 1996-06-27 18:34:22 jik Exp $";
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

#include "config.h"

/*
 * mesg_strings.c: global message string array
 *
 * This file must be in a specific format so that it can be converted
 * correctly into mesg_strings.h.  For details about the format, see
 * mesg_str.awk.
 */


/* Default to English. */

#ifndef XRN_LANG_english
#ifndef XRN_LANG_german
#define XRN_LANG_english
#endif
#endif

/*
 * Global messsages and strings like "yes", "no", "OK?", etc.  These
 * are #define's instead of strings in the mesg_strings array because
 * they are used in static arrays that can't contain references to
 * other arrays.
 */

#ifdef XRN_LANG_english

#define YES_STRING   "yes"
#define NO_STRING    "no"
#define ABORT_STRING "abort"
#define DOIT_STRING  "doit"
#define SAVE_STRING  "save"

#define ADD_STRING        "add"
#define FORWARD_STRING    "forward"
#define BACK_STRING       "back"
#define LAST_GROUP_STRING "last group"
#define FIRST_STRING      "first"
#define LAST_STRING       "last"
#define CURSOR_POS_STRING "cursor position"
#define SUB_STRING        "subscribe"
#define GOTO_NG_STRING    "go to newsgroup"
#define CLICK_TO_CONT_STRING "Click to continue"

/* Strings to compose newsgroup line for ngMode */
/* The format is NEWSGROUPS_INDEX_MSG.  The strings in it are:
 * 1) UNREAD_MSG or empty string 
 * 2) NEWS_IN_MSG or empty string 
 * 3) newsgroup
 * 4) articles unread
 * 5) NOT_ONE_MSG if *one* unread article is available, else " "
 *
 * example:"Unread news in comp.sys.ibm                            30 articles + 20 old"                  
 * Note: maximum line length is normally 200
 */
/* The number of characters into a newsgroup index line after which
   the newsgroup name actually starts. */
#define NEWS_GROUP_OFFSET 15
/*
  The number of characters on a newsgroup index line, other than the
  newsgroup name, and not including the newline or the null at the
  end.
  */
#define NEWS_GROUP_LINE_CHARS 41
/* end strings for ngMode */

/* strings for buildQuestion, bottomline in ngMode */
/* max len after sprintf is LABEL_SIZE (128)      */
#define QUEST_ART_NOUNREAD_NONEXT_STRING  "Art. %ld in %s"
#define QUEST_ART_NOUNREAD_NEXT_STRING    "Art. %ld in %s (Next: %s, %ld article%s)"
#define QUEST_ART_UNREAD_NONEXT_STRING    "Art. %ld in %s (%ld left)"
#define QUEST_ART_UNREAD_NEXT_STRING      "Art. %ld in %s (%ld left) (Next: %s, %ld article%s)"
/*                                               1)    2)   3)              4)  5)
 * 1) article number
 * 2) actual newsgroup 
 * 3) # unread articles
 * 4) next newsgroup
 * 5) # unread articles next newsgroup
 *
 * example "Art. 4093 in comp.sys.ibm (30 left) (Next: comp.sys.ibm.hardware, 20 articles)
 */
/* end strings for buildQuestions */

/* Subject markers */

#define READ_MARKER	'+'
#define UNREAD_MARKER	'u'
#define SAVED_MARKER	'S'
#define PRINTED_MARKER	'P'

#endif /* XRN_LANG_english */


#ifdef XRN_LANG_german

#define YES_STRING   "Ja"
#define NO_STRING    "Nein"
#define ABORT_STRING "Abbrechen"
#define DOIT_STRING  "Ausf\374hren"
#define SAVE_STRING  "Sichern"

#define ADD_STRING        "hinzuf\374gen"
#define FORWARD_STRING    "nach unten"
#define BACK_STRING       "nach oben"
#define LAST_GROUP_STRING "letzte Gruppe"
#define FIRST_STRING      "Anfang"
#define LAST_STRING       "Ende"
#define CURSOR_POS_STRING "Aktuelle Position"
#define SUB_STRING        "Abonnieren"
#define GOTO_NG_STRING    "Gehe zu Gruppe"
#define CLICK_TO_CONT_STRING "Fortsetzen"

/* Strings to compose newsgroup line for ngMode */
/* The format is NEWSGROUPS_INDEX_MSG.  The strings in it are:
 * 1) UNREAD_MSG oder leerer string 
 * 2) NEWS_IN_MSG oder leere string 
 * 3) Newsgruppe
 * 4) Artikel ungelesen
 * 5) NOT_ONE_MSG wenn *ein* ungelesener Artikel, sonst " "
 *
 * Beispiel:"Ungelesene Nachrichten in comp.sys.ibm.misc                        30 Artikel  + 20 alt"                 
 * Note: maximale Zeilenlaenge ist normalerweise 200
 */
/* The number of characters into a newsgroup index line after which
   the newsgroup name actually starts. */
#define NEWS_GROUP_OFFSET 26
/*
  The number of characters on a newsgroup index line, other than the
  newsgroup name, and not including the newline or the null at the
  end.
  */
#define NEWS_GROUP_LINE_CHARS 45
/* end strings for ngMode */

/* strings for buildQuestion, bottomline in ngMode */
/* max len after sprintf is LABEL_SIZE (128)      */
#define QUEST_ART_NOUNREAD_NONEXT_STRING  "Art. %ld in %s"
#define QUEST_ART_NOUNREAD_NEXT_STRING    "Art. %ld in %s (N\344chste: %s, %d Artikel%s)"
#define QUEST_ART_UNREAD_NONEXT_STRING    "Art. %ld in %s (%ld verbleiben)"
#define QUEST_ART_UNREAD_NEXT_STRING      "Art. %ld in %s (%ld verbleiben) (N\344chste: %s, %d Artikel%s)"
/*                                              1)    2)   3)                          4)  5)
 * 1) article number
 * 2) actual newsgroup 
 * 3) # unread articles
 * 4) next newsgroup
 * 5) # unread articles next newsgroup
 *
 * example "Art. 4093 in comp.sys.ibm (30 left) (Next: comp.sys.ibm.hardware, 20 articles)
 */
/* end strings for buildQuestions */

/* Subject markers */

#define READ_MARKER	'+'
#define UNREAD_MARKER	'n'
#define SAVED_MARKER	'S'
#define PRINTED_MARKER	'D'

#endif /* XNR_LANG_german */

/*
 * Messages for use in mesgPane calls below.  Many of these messages
 * are used multiple times, which is why they are all here are
 * constants.  Also, putting them all here makes it easy to make sure
 * they're all consistent.  Finally, putting them here saves space in
 * the executable since there's only one copy of each string.
 */

#ifdef XRN_LANG_english

char *message_strings[] = {
/* < BAD_BUTTON_NAME > */
    "XRN error: bad button name `%s'.", /* button name */
/* < NO_SUCH_NG_DELETED > */
    "Newsgroup `%s' does not exist.  It may have been deleted.  ", /* newsgroup name */
/* < UNKNOWN_FUNC_RESPONSE > */
    "Internal XRN error: unknown response %d from %s in %s.", /* return value, called function, calling function */
/* < DISPLAYING_LAST_UNREAD > */
    "No unread articles in `%s'.  Displaying last available article.", /* newsgroup name */
/* < PROBABLY_KILLED > */
    "No unread articles in `%s'.  They were probably killed.  ", /* newsgroup name */
/* < NO_ARTICLES > */
    "No articles in `%s'.", /* newsgroup name */
/* < PROBABLY_EXPIRED > */
    "No articles in `%s'.  They were probably expired or cancelled.  ", /* newsgroup name */
/* < NO_NG_SPECIFIED > */
    "No newsgroup name specified.",
/* < NO_SUCH_NG > */
    "Newsgroup `%s' does not exist.", /* newsgroup name */
/* < NO_PREV_NG > */
    "No previous newsgroup.",
/* < NO_GROUPS_SELECTED > */
    "No newsgroups were selected.",
/* < NG_NOT_MOVED > */
    "New position for newsgroups can't be in block of selected newsgroups.  Newsgroups have not been moved.",
/* < SKIPPING_TO_NEXT_NG > */
    "Skipping to next newsgroup.",
/* < BAD_ART_NUM > */
    "Invalid article number `%s'.", /* article number string */
/* < NO_ART_NUM > */
    "No article number specified.",
/* < ART_NOT_AVAIL > */
    "Article number %d not available.", /* article number */
/* < ARTS_NOT_AVAIL > */
    "Article numbers %d-%d not available.", /* first, last article number */
/* < NO_PREV_REGEXP > */
    "No previous regular expression.",
/* < NO_PREV_ART > */
    "No previous article.",
/* < MSG_ABORTED > */
    "Aborted %s.", /* "article" or "message" */
/* < NO_FILE_NOT_SAVED > */
    "Cannot determine save file name.  Article/message not saved.",
/* < NO_SUBJECT > */
    "The Subject field is missing in your message!  ",
/* < EMPTY_SUBJECT > */
    "The Subject field in your message is empty!  ",
/* < NO_NEWSGROUPS > */
    "The Newsgroups field is missing in your message!  ",
/* < MULTI > */
    "There are multiple %s fields in your message!  Please delete all but one of them and resend.", /* field name */
/* < DEFAULT_ADDED > */
    "A default value has been added.  Please edit it as necessary and resend.",
/* < EMPTY_ADDED > */
    "An empty one has been added.  ",
/* < FILL_IN_RESEND > */
    "Please fill it in and resend.",
/* < NO_POSTABLE_NG > */
    "No postable newsgroups in `Newsgroups' line.  Please fix and resend or save and abort.",
/* < SAVING_DEAD > */
    "Saving in `%s'.", /* file name */
/* < COULDNT_POST > */
    "Could not post article.  ",
/* < POST_NOTALLOWED > */
    "Posting not allowed from this machine.  ",
/* < COULDNT_SEND > */
    "Could not send mail message.  ",
/* < MAILED_TO_MODERATOR > */
    "One or more moderated newsgroups in `Newsgroups' line of message.  Article will be mailed to moderator by server.",
/* < ARTICLE_POSTED > */
    "Article posted.",
/* < MAIL_MESSAGE_SENT > */
    "Mail message sent.",
/* < CANT_INCLUDE_CMD > */
    "Cannot execute includeCommand (`popen' failed).",
/* < CANT_OPEN_ART > */
    "Cannot open article file `%s': %s.", /* file name, error string */
/* < CANT_OPEN_FILE > */
    "Cannot open file `%s': %s.", /* error string */
/* < NO_FILE_SPECIFIED > */
    "No file specified.",
/* < CANT_OPEN_TEMP > */
    "Cannot open temporary file `%s': %s.", /* file name, error string */
/* < CANT_STAT_TEMP > */
    "Cannot stat temporary file `%s': %s.", /* file name, error string */
/* < NO_CHANGE > */
    "No change in temporary file `%s'.", /* file name */
/* < ZERO_SIZE > */
    "Temporary file `%s' has zero size.", /* file name */
/* < NO_MSG_TEMPLATE > */
    "Internal XRN error: no message template in call to Call_Editor.",
/* < CANT_EDITOR_CMD > */
    "Cannot execute editor command `%s': %s.", /* command, error string */
/* < ONE_COMPOSITION_ONLY > */
    "Only one composition allowed at a time.",
/* < EXECUTING_SIGNATURE > */
    "Executing signature command `%s'.", /* command */
/* < CANT_EXECUTE_SIGNATURE > */
    "Cannot execute signature file `%s'.  Reading instead.", /* signature file name */
/* < READING_SIGNATURE > */
    "Reading signature file `%s'.", /* signature file name */
/* < CANT_READ_SIGNATURE > */
    "Cannot read signature file `%s': %s.", /* signature file name, error string */
/* < SIGNATURE_TOO_BIG > */
    "Signature file `%s' is too large; ignoring it.", /* signature file name */
/* < CANCEL_ABORTED > */
    "Article not cancelled.",
/* < CANCELLED_ART > */
    "Article has been cancelled.",
/* < UNKNOWN_REGEXP_ERROR > */
    "Unknown error in regular expression `%s'.", /* regexp string */
/* < KNOWN_REGEXP_ERROR > */
    "Error in regular expression `%s': %s.", /* regexp, error string */
/* < ART_NUMBERING_PROBLEM > */
    "Article numbering problem.  Marking all articles in newsgroup `%s' unread.", /* newsgroup name */
/* < CANT_OPEN_KILL > */
    "Cannot open kill file `%s': %s.", /* file name, error string */
/* < CANT_OPEN_INCLUDED_KILL > */
    "Cannot open kill file `%s' (included from `%s': %s.", /* file name, parent file name, error string */
/* < MALFORMED_KILL_ENTRY > */
    "Error in KILL file entry `%s' in KILL file `%s': %s.", /* entry, file, reason for error */
/* < ERROR_INCLUDE_MISSING > */
    "No newsgroup or file name specified in include directive",
/* < ERROR_INCLUDE_NOT_SEPARATED > */
   " Include operand not separated",
/* < KILL_ERROR_UNKNOWN_OPTION > */
    "Error in KILL file entry `%s' in KILL file `%s': Unknown option `%s'.", /* entry, file, unknown option */
/* < UNKNOWN_KILL_REGEXP_ERROR > */
    "Unknown regular expression error in KILL file entry `%s' in KILL file `%s'.", /* entry */
/* < KNOWN_KILL_REGEXP_ERROR > */
    "Regular expression error in KILL file entry `%s' in KILL file `%s': %s.", /* entry, error string */
/* < KILL_TOO_LONG > */
    "Discarding too-long entry starting with `%s' in KILL file `%s'.", /* start of entry, file */
/* < NOT_IN_NEWSRC > */
    "Newsgroup `%s' is not in your .newsrc file.", /* newsgroup name */
/* < BOGUS_NG_REMOVING > */
    "Newsgroup `%s' does not exist.  Removing it from your .newsrc file.", /* newsgroup name */
/* < MISSING_NG_LISTING > */
    "Newsgroup `%s' not found in cache.  Retrieving newsgroup list to find it.", /* newsgroup name */
/* < MAYBE_LIST > */
    "Newsgroup `%s' not found in cache.\nRetrieve newsgroup list to find it?", /* newsgroup name */
/* < DUP_NEWSRC_ENTRY > */
    "Duplicate .newsrc entry for newsgroup `%s'.  Using the first one.", /* newsgroup name */
/* < BAD_NEWSRC_LINE > */
    "Unable to parse line %d in .newsrc file.  Ignoring it.", /* line number */
/* < CANT_OPEN_NEWSRC_COPYING > */
    "Cannot open .newsrc file `%s' for copying: %s.", /* file name, error string */
/* < CANT_EXPAND > */
    "Cannot expand file name `%s'.", /* file name */
/* < EMPTY_NEWSRC_SAVE_NAME > */
    ".newsrc save file name is the empty string.",
/* < CANT_OPEN_NEWSRC_SAVE > */
    "Cannot open .newsrc save file `%s' for writing: %s.", /* file name, error string */
/* < NEWSRC_SAVE_FILE_WRITE_ERR > */
    "Error writing to .newsrc save file `%s': %s.", /* file name, error string */
/* < CANT_READ_NEWSRC > */
    "Cannot read .newsrc file `%s': %s.", /* file name, error string */
/* < CREATING_NEWSRC > */
    "Creating .newsrc file `%s' for you.", /* file name */
/* < CANT_CREATE_NEWSRC > */
    "Cannot create .newsrc file `%s': %s.", /* file name, error string */
/* < CANT_STAT_NEWSRC > */
    "Cannot stat .newsrc file `%s': %s.", /* file name, error string */
/* < ZERO_LENGTH_NEWSRC > */
    ".newsrc file `%s' is empty.  Aborting.", /* file name */
/* < CANT_OPEN_NEWSRC > */
    "Cannot open .newsrc file `%s' for reading: %s.", /* file name, error string */
/* < CANT_PARSE_NEWSRC > */
    "Cannot parse .newsrc file `%s' -- error on line %d.", /* file name, error line */
/* < CANT_OPEN_NEWSRC_TEMP > */
    "Cannot open .newsrc temporary file `%s' for writing: %s.", /* file name, error string */
/* < CANT_OPEN_NEWSRC_WRITING > */
    "Cannot open .newsrc file `%s' for writing: %s.", /* file name, error string */
/* < ERROR_UNLINKING_NEWSRC > */
    "Error unlinking .newsrc file `%s': %s.", /* file name, error string */
/* < ERROR_RENAMING > */
    "Error renaming temporary file `%s' to file `%s': %s.", /* temporary file name, file name, error string */
/* < NO_MAIL_DIR > */
    "No Mail directory `%s'.", /* directory name */
/* < NO_SUCH_MAIL_DIR > */
    "No such folder `%s'; Create it?", /* directory name */
/* < CANT_STAT_MAIL_DIR > */
    "Cannot stat directory `%s': %s.", /* directory name, error string */
/* < MAIL_DIR_NOT_DIR > */
    "Cannot create folder -- mail path `%s' is not a directory.", /* directory name */
/* < FOLDER_NOT_DIR > */
    "Path `%s' is not a folder.", /* folder name */
/* < NO_SUCH_RMAIL > */
    "No such RMAIL file `%s'; Create it?", /* file name */
/* < CANT_OPEN_RMAIL > */
    "Cannot open RMAIL file `%s' for writing: %s.", /* file name, error string */
/* < CANT_WRITE_RMAIL > */
    "Cannot write to RMAIL file `%s': %s.", /* file name, error string */
/* < UNKNOWN_CONFIRM_BUTTON > */
    "XRN error: unknown confirm button `%s'.", /* button name */
/* < CANT_EXECUTE_CMD_POPEN > */
    "Cannot execute command `%s' (`popen' failed).", /* command string */
/* < CANT_EXPAND_DIR > */
    "Cannot expand directory `%s'.", /* directory name */
/* < CANT_CREATE_SAVE_DIR > */
    "Cannot create save directory `%s': %s.", /* drectory name, error string */
/* < CANT_FIGURE_FILE_NAME > */
    "Cannot figure out file name `%s'.", /* file name */
/* < CANT_CREAT_APPEND_SAVE_FILE > */
    "Cannot %s file `%s': %s.", /* "create" or "append to", file name, error string */
/* < ERROR_WRITING_FILE > */
/* < ERROR_WRITING_SAVE_FILE > */
    "Error writing to file `%s': %s.", /* file name, error string */
/* < CONNECTING > */
    "Connecting to NNTP server `%s'...", /* server name */
/* < GETTING_LIST > */
    "Getting list of newsgroups...",
/* < GETTING_NEWGROUPS > */
    "Getting list of new newsgroups...",
/* < FAILED_CONNECT > */
    "Failed to connect to NNTP server `%s'.", /* server name */
/* < LOST_CONNECT_ATTEMPT_RE > */
    "Lost connection to the NNTP server.  Attempting to reconnect.",
/* < RECONNECTED > */
    "Reconnected to the NNTP server.",
/* < CANT_TEMP_NAME > */
    "Cannot create temporary file name for article.",
/* < CANT_CREATE_TEMP > */
    "Cannot open temporary file `%s' for writing: %s.", /* file name, error string */
/* < BOGUS_ACTIVE_ENTRY > */
    "Skipping bogus active file entry `%s'.", /* entry */
/* < BOGUS_ACTIVE_CACHE > */
    "Skipping bogus active cache entry `%s'.", /* entry */
/* < XHDR_ERROR > */
    "XHDR command to the NNTP server failed.  Either the NNTP server does not\n\tsupport XHDR (in which case XRN will not work), or an internal\n\tXRN error occurred.",
/* < NNTP_ERROR > */
    "NNTP serious error: `%s'.", /* error string */
/* < MALFORMED_XHDR_RESPONSE > */
    "NNTP server sent malformed XHDR response.  XHDR command was `%s', response was `%s'.", /* command, response */
/* < NO_APP_DEFAULTS > */
    "The current XRN Application Defaults file is not installed.  As a result, some XRN functionality may be missing.  If XRN was installed by someone else at your site, contact the installer about this error.  If you are the installer, see the COMMON-PROBLMS file in the XRN source directory to find out how to fix this problem.  ",
/* < VERSIONS > */
    "Installed Application Defaults file version is `%s'.  XRN executable version is `%s'.", /* app-defaults version, executable version */
/* < NO_DOMAIN > */
    "Could not determine your host's domain.\nRerun XRN with the DOMAIN environment variable or the domainName X resource set\nin order to post or send mail.",
/* < NO_SERVER > */
    "Could not determine the news server.\nRerun XRN with the NNTPSERVER environment variable, the nntpServer X resource or\nthe -nntpServer command line argument.",
/* < UNKNOWN_LIST_REGEXP_ERROR > */
    "Unknown regular expression error in %s list entry `%s'; entry ignored.", /* list name, entry */
/* < KNOWN_LIST_REGEXP_ERROR > */
    "Regular expression error in %s list entry `%s': %s; entry ignored.", /* list name, entry, error string */
/* < OPEARATION_APPLY_CURSOR > */
   "Operations apply to current selection or cursor position",
/* < NO_MORE_UNREAD_ART > */
   "No more unread articles in the subscribed to newsgroups",
/* < SEL_GROUPS_ADDSUB > */
   "Select groups to `add', `quit' unsubscribes remaining groups",
/* < ARE_YOU_SURE > */
   "Are you sure?",
/* SUB, UNSUB and IGNORED strings must have the same len */
/* < SUBED >  */ 
   "subscribed  ",
/* < UNSUBED > */
   "unsubscribed",
/* < IGNORED > */
   "ignored     ",
/* < OK_CATCHUP > */
   "OK to Catchup?",
/* < OK_CATCHUP_CUR > */ 
   "OK to catch up to current position?",
/* < OK_GETLIST > */
    "OK to fetch newsgroup list from the server?",
/* < OK_TO_UNSUB > */
   "OK to unsubscribe?",
/* < OK > */
    "OK",
/* < EDIT > */
    "edit",
/* < SEARCH_ABORTED > */
   "search has been aborted",
/* < ERROR_SUBJ_SEARCH > */ 
   "Subject search: %s", /* regular expression */
/* < ERROR_SUBJ_EXH > */ 
   "Subject has been exhausted",
/* < ERROR_SUBJ_ABORT > */
   "search aborted",
/* < KILL_DONE > */
   "Returning to first unread article.",
/* < UNKNOWN_KILL_TYPE > */
    "Unknown kill type \"%s\" in \"%s\" kill request.", /* field type argument, field name */
/* < ERROR_CANT_UPDATE_NEWSRC > */
   "Cannot update the newsrc file",
/* < ARTICLE_NUMBER > */ 
   "Article Number:",
/* < ERROR_SUBJ_EXPR > */
   "Search for expression %s: no match was found", /* regular expression */
/* < ERROR_SEARCH > */ 
   "Search for expression %s", /* regular expression */
/* < REGULAR_EXPR > */
   "Regular Expression:",
/* < BEHIND_WHAT_GROUP > */
   "After which newsgroup?",
/* < ARTICLE_QUEUED > */
   "Article sucessfully queued",
/* < GROUP_SUB_TO > */
   "Group to subscribe to:",
/* < GROUP_TO_GO > */ 
   "Group to go to:",
/* < VIEW_ALLNG_SUB > */
   "View all available groups, with option to subscribe",
/* < AUTOMATIC_RESCAN > */
   "automatic rescan in progress...",
/* < RESCANNING_BACKGROUND > */
    "Rescanning in the background...",
/* < ERROR_UNSUP_TRANS > */
   "unsupported transition: %d to %d", /* transition from, to */
/* < POST_FOLLOWUP > */
   "article",
/* < FOLLOWUP_REPLY > */ 
   "article and mail message",
/* < DEFAULT_MAIL > */
   "mail message",
/* < SAVE_IN > */
   "Saving in %s",  /* file */
/* < ERROR_SEND_MAIL > */
   "Error sending mail:",
/* < ASK_FILE > */
   "File Name?",
/* < ASK_POST_ARTICLE > */
   "Post the article?",
/* < ASK_POST_SEND > */
   "Post and/or send the message?",
/* < ASK_RE_EDIT_ARTCILE > */
   "Re-edit the article?",
/* < ASK_RE_EDIT > */
   "Re-edit the message?",
/* < ERROR_EXEC_FAILED > */
   "XRN Error: execl of `%s' failed\n", /* prog */
/* < ASK_POSTER_FANDR > */
   "`Followup-To' line in message says to reply to poster.\nIgnore it and post as well, or just send E-mail?",
/* < ASK_POSTER_REPLY > */
   "`Followup-To' line in message says to reply to poster.\nPost followup or mail reply?",
/* < POST_AND_SEND > */
   "post and send mail",
/* < SEND_MAIL > */
   "send mail",
/* < POST > */ 
   "post",
/* < FOLLOWUP_MULTIPLE_NGS > */
    "The default `Newsgroups' line of your followup contains multiple newsgroups.  Please be sure to remove inappropriate newsgroups before sending your message, and/or to put a more appropriate list of groups in your `Followup-To' line.",
/* < FOLLOWUP_FOLLOWUPTO > */
    "Note that the article to which you are responding contains a `Followup-To' line, so the default `Newsgroups' line of your followup has been set from that line rather than from the `Newsgroups' line of the original article.",
/* < CROSSPOST_PROHIBIT > */
    "The `Newsgroups' line of your message contains %d newsgroups.  The maximum\nnumber of groups to which you are allowed to post a message is %d.  Please\nreduce the number of groups to which you are posting and then send your\nmessage again.",
/* < CROSSPOST_CONFIRM > */
    "The `Newsgroups' line of your message contains %d newsgroups.  Please\nconsider reducing the number of groups to which you are posting.",
/* < FOLLOWUP_FOLLOWUPTO_CONFIRM > */
    "The `Newsgroups' line of your message contains %d newsgroups, and the\n`Followup-To' line contains %d newsgroups.  Please consider reducing the\nnumber of groups in your `Newsgroups' and/or `Followup-To' line.",
/* < FOLLOWUP_CONFIRM > */
    "The `Newsgroups' line of your message contains %d newsgroups.  Please consider\neither reducing the number of newsgroups to which you are posting or adding a\n`Followup-To' line containing a smaller number of groups.",
/* < ERROR_STRIPFIELD_NL > */
   "ouch!  can't find newline in stripField\n",
/* < FOLLOWUP_REPLY_TO_TITLE > */
   "Followup and reply to article %ld in `%s'", /* article number, newsgroup */
/* < FOLLOWUP_TO_TITLE > */
   "Followup to article %ld in `%s'", /* article number, newsgroup */
/* < REPLY_TO_TITLE > */
   "Reply to article %ld in `%s'", /* article number, newsgroup */
/* < FORWARD_TO_TITLE > */
   "Forward article %ld in `%s' to a user", /* article number, newsgroup */
/* < POST_ARTICLE > */
   "Post article",
/* < POST_ARTICLE_TO > */
   "Post article to `%s'", /* newsgroup */
/* < POST_MAIL_ARTICLE > */
    "Post and mail article",
/* < POST_MAIL_ARTICLE_TO > */
    "Post article to `%s' and mail it",
/* < MAIL > */
    "Send a mail message",
/* < USER_CANT_CANCEL > */
   "Not entitled to cancel the article",
/*
 ### may be the following messages shouldn't translate ###
 */
/* < REPLY_YOU_WRITE > */
   "In article %s, you write:\n", /* messageid */
/* < FORWARDED_ARTIKEL > */
   "\n------ Forwarded Article %s\n------ From %s\n\n", /* messageid , author */
/* < FORWARDED_ARTICLE_END > */
   "\n------ End of Forwarded Article\n",
/* < FOLLOWUP_AUTHOR_WRITE > */
"In article %s, %s writes:\n", /* messageid , author */
/* #### end may be not translate #### */
/* < NEWSGROUPS_INDEX > */
    "%6s %7s %*s %4d article%1.1s +%5d old",
/* < UNREAD > */
    "Unread",
/* < NEWS_IN > */
    "news in",
/* < NOT_ONE > */
    "s", /* see NEWSGROUPS_INDEX in STRING section */
/* < DONE > */
    "done",
/* < ABORTED > */
    "aborted",
/* < FAILED > */
    "failed",
/* < CREATE > */
    "create",
/* < APPEND > */
    "append to",
/* < ERR_XRN_RUN > */
   "An XRN of yours is running on %s as process %d.\nIf it is no longer running, remove the file \"%s\".\n", /* host, pid, lockfile */
/* < ERROR_CANT_READ_NEWSRC > */
   "Can not read the .newsrc file",
/* < PROCESS_KILL_FOR > */
   "Processing KILL file for newsgroup `%s'...", /* newsgroup */
/* < ERROR_REGEX_NOSLASH > */
   "no slash terminating the regular expression",
/* < ERROR_REGEX_NOSLASH_START > */
    "no slash preceding the regular expression",
/* < ERROR_REGEX_NOCOLON > */
   "no colon after the regular expression",
/* < ERROR_REGEX_UNKNOWN_COMMAND > */
   "unknown command (valid commands are `j', `m', and `s')",
/* < KILL_LINE > */
    "Processing entry `%s' in KILL file `%s'.",
/* < KILL_KILLED > */
   "killed - %s",         /* subject */
/* < KILL_UNREAD > */
   "marked unread - %s",  /* subject */
/* < KILL_SAVED > */
   "saved - %s",         /* subject */
/* < COUNT_KILLED > */
   "killed %d article%s in %s", /* count, "" or NOT_ONE_STRING , newsgroup */
/* < COUNT_UNREAD > */
   "marked %d article%s unread in %s", /* count, "" or NOT_ONE_STRING , newsgroup */
/* < COUNT_SAVED > */
   "saved %d article%s in %s", /* count, "" or NOT_ONE_STRING , newsgroup */
/* < ERROR_CORNERED > */
   "XRN error in `cornered': expecting nglist to be valid\n",
/* < ERROR_OUT_OF_MEM > */
   "out of memory",
/* < PREFETCHING > */
    "Fetching `%s'...",
/* < SERVER_POSTING_ERROR > */
    "Error from NNTP server: %s", /* error message */
/* < ASK_SAVEBOX > */
   "FileName, +FolderName, or @FolderName?",
/* < SAVE_PIPE_TO > */
   "Piping article %ld into command `%s'...",  /* articlenumber, command */
/* < ERROR_SAVE_PIPE > */
   "`%s' exited with status %d", /* command, status */
/* < SAVE_MH_REFILE > */
   "MH refile to folder %s %s", /* folder, status */
/* < SAVE_RMAIL_REFILE > */
   "RMAIL refile to folder %s %s", /* folder, status */
/* < SAVE_OK > */
   "Saving article %ld in file `%s'...", /* articlenumber, filename */
/* < SAVE_APPEND_OK > */
   "Appending article %ld to file `%s'...", /* articlenumber, filename */
/* < SAVE_ARTICLE > */
   "Article: %ld of %s\n", /* articlenumber, newsgroup */
/* < ERROR_INFINITE_LOOP > */
   "XRN panic: moveBeginning / moveEnd in infinite loop",
/* < ERROR_FINDARTICLE > */
   "Valid article number not found in findArticle",
/* < ERROR_STRIP_LEAVE_HEADERS > */
   "Only one of `stripHeaders', `leaveHeaders' resources allowed",
/* < ERROR_REQUEST_FAILED > */
   "        Request was: `%s'\n        Failing response was: `%s'", /* command, message */
/* < ASK_UPDATE_NEWSRC > */
    ".newsrc file updated by another program, continue?",
/* < PENDING_COMPOSITION > */
    "You cannot exit when a composition is pending!",
/* < NNTP_PASSWORD > */
    "Enter NNTP password:",
/* < UNKNOWN_SORT_TYPE > */
    "Unknown subject sort type: %s",
/* < TOO_MANY_SORT_TYPES > */
    "Too many subject sort types specified.",
/* < UNPARSEABLE_DATE > */
    "Unparseable date (article %ld in %s): %s",
/* < THREADING_FOR > */
    "Threading newsgroup `%s'...", /* newsgroup */
};

#endif /* XRN_LANG_english */

#ifdef XRN_LANG_german

/*
 * mesg_strings.c: global message string array
 *
 * --------------
 * section GERMAN
 * --------------
 * The GERMAN section was created and translated by
 *            K.Marquardt (K.Marquardt@zhv.basf-ag.de)  Nov/23/94
 * revised by T.Foks (foks@do2.maus.ruhr.de) May/23/96
 *
 * german version (iso8859-1), use LANGUAGE= german in Imakefile/Makefile
 *
 * values of the iso8859-1 characters:
 *
 * "a = \344, "o = \366, "u = \374
 * "A = \304, "O = \326, "U = \334
 * sz = \337
 */

char *message_strings[] = {
/* < BAD_BUTTON_NAME > */
    "XRN Fehler: Falscher Knopf Name `%s'.", /* button Name */
/* < NO_SUCH_NG_DELETED > */
    "Newsgruppe `%s' existiert nicht, m\366glicherweise wurde sie gel\366scht.", /* Newsgruppe Name */
/* < UNKNOWN_FUNC_RESPONSE > */
    "Interner XRN Fehler: unbekannte R\374ckmeldung %d von %s in %s.", /* return value, called function, calling function */
/* < DISPLAYING_LAST_UNREAD > */
    "Keine ungelesenen Artikel in `%s'. Zeige letzten vorhandenen Artikel an.", /* Newsgruppe Name */
/* < PROBABLY_KILLED > */
    "Keine ungelesenen Artikel in `%s', m\366glicherweise wurden sie gel\366scht.", /* Newsgruppe Name */
/* < NO_ARTICLES > */
    "Keine Artikel in `%s'.", /* Newsgruppe Name */
/* < PROBABLY_EXPIRED > */
    "Keine Artikel in `%s', m\366glicherweise wurden sie zur\374ckgezogen.", /* Newsgruppe Name */
/* < NO_NG_SPECIFIED > */
    "Keine Newsgruppe angegeben.",
/* < NO_SUCH_NG > */
    "Newsgruppe `%s' existiert nicht.", /* Newsgruppe Name */
/* < NO_PREV_NG > */
    "Keine vorhergehende Newsgruppe.",
/* < NO_GROUPS_SELECTED > */
    "Keine Newsgruppen ausgew\344hlt.",
/* < NG_NOT_MOVED > */
    "Die neue Position kann nicht innerhalb der angew\344hlten Newsgruppen liegen.  Es wurden keine Newsgruppen verschoben.",
/* < SKIPPING_TO_NEXT_NG > */
    "Verzweige zur n\344chsten Newsgruppe.",
/* < BAD_ART_NUM > */
    "Falsche Artikelnummer `%s'.", /* Artikel.number string */
/* < NO_ART_NUM > */
    "Keine Artikelnummer angegeben.",
/* < ART_NOT_AVAIL > */
    "Artikelnummer %d nicht vorhanden.", /* Artikel.number */
/* < ARTS_NOT_AVAIL > */
    "Die Artikel %d-%d sind nicht verf\374gbar.", /* first, last article number */
/* < NO_PREV_REGEXP > */
    "Kein vorheriges Suchmuster.",
/* < NO_PREV_ART > */
    "Kein vorheriger Artikel.",
/* < MSG_ABORTED > */
    "Abbruch: %s.", /* "article" or "message" */
/* < NO_FILE_NOT_SAVED > */
    "Kann den Dateinamen nicht ermitteln, Artikel/Nachricht nicht gespeichert.",
/* < NO_SUBJECT > */
    "Ihre Nachricht enth\344lt kein Thema!",
/* < EMPTY_SUBJECT > */
    "Die `Subject` Zeile Ihrer Nachricht ist leer!",
/* < NO_NEWSGROUPS > */
    "Ihre Nachricht enth\344lt keine `Newsgroups` Zeile!",
/* < MULTI > */
    "Sie haben mehrere %s Zeilen in Ihrer Nachricht!  Bitte alle, bis auf eine, entfernen und erneut abschicken.", /* field name */
/* < DEFAULT_ADDED > */
    "Es wurde ein Standardwert eingef\374gt, bitte passen Sie ihn entsprechend an.",
/* < EMPTY_ADDED > */
    "Es wurde ein leeres Feld eingef\374gt.",
/* < FILL_IN_RESEND > */
    "Bitte Ausf\374llen und erneut abschicken.",
/* < NO_POSTABLE_NG > */
    "Sie k\366nnen in keine der angegebenen Newsgruppen ver\366ffentlichen.  Bitte `Newsgroups` Zeile ab\344ndern und erneut abschicken.",
/* < SAVING_DEAD > */
    "Speichere in `%s'.", /* file Name */
/* < COULDNT_POST > */
    "Kann Artikel nicht ver\366ffentlichen.",
/* < POST_NOTALLOWED > */
    "Von diesem Rechner k\366nnen keine Artikel ver\366ffentlicht werden.",
/* < COULDNT_SEND > */
    "Konnte Nachricht nicht senden.",
/* < MAILED_TO_MODERATOR > */
    "Es wurden eine oder mehrere moderierte Newsgruppen angegeben. Der Artikel wird vom Server zum Moderator geschickt.",
/* < ARTICLE_POSTED > */
    "Artikel ver\366ffentlicht.",
/* < MAIL_MESSAGE_SENT > */
    "Nachricht verschickt.",
/* < CANT_INCLUDE_CMD > */
    "Kann includeCommand nicht ausf\374hren, `popen' fehlgeschlagen.",
/* < CANT_OPEN_ART > */
    "Kann Artikeldatei `%s' nicht \366ffnen: %s.", /* file name, Fehler string */
/* < CANT_OPEN_FILE > */
    "Kann Datei `%s' nicht \366ffnen: %s.", /* Fehler string */
/* < NO_FILE_SPECIFIED > */
    "Keine Datei angegeben.",
/* < CANT_OPEN_TEMP > */
    "Kann Datei `%s' nicht \366ffnen: %s.", /* file name, Fehler string */
/* < CANT_STAT_TEMP > */
    "Kann den Status der Datei `%s' nicht ermitteln: %s.", /* file name, Fehler string */
/* < NO_CHANGE > */
    "Keine Ver\344nderung der Datei `%s'.", /* file Name */
/* < ZERO_SIZE > */
    "Datei `%s' ist leer.", /* file Name */
/* < NO_MSG_TEMPLATE > */
    "Interner XRN Fehler: Keine Nachrichtenvorlage beim Aufruf des Editors.",
/* < CANT_EDITOR_CMD > */
    "Kann Editorbefehl `%s'nicht ausf\374hren: %s.", /* command, Fehler string */
/* < ONE_COMPOSITION_ONLY > */
    "Verfassen nur eines Artikels gleichzeitig m\366glich.",
/* < EXECUTING_SIGNATURE > */
    "F\374hre Signaturbefehl `%s' aus.", /* command */
/* < CANT_EXECUTE_SIGNATURE > */
    "Kann Signaturdatei `%s' nicht ausf\374hren, lese sie ein.", /* signature file Name */
/* < READING_SIGNATURE > */
    "Lese Signaturdatei `%s' ein.", /* signature file Name */
/* < CANT_READ_SIGNATURE > */
    "Kann Signaturdatei nicht `%s' lesen: %s.", /* signature file name, Fehler string */
/* < SIGNATURE_TOO_BIG > */
    "Signaturdatei `%s' ist zu gro\337; wird ignoriert.", /* signature file name */
/* < CANCEL_ABORTED > */
    "Artikel nicht zur\374ckgezogen.",
/* < CANCELLED_ART > */
    "Artikel zur\374ckgezogen.",
/* < UNKNOWN_REGEXP_ERROR > */
    "Unbekannter Fehler im Suchmuster `%s'.", /* regexp string */
/* < KNOWN_REGEXP_ERROR > */
    "Fehler im Suchmuster `%s': %s.", /* regexp, Fehler string */
/* < ART_NUMBERING_PROBLEM > */
    "Probleme mit den Artikelnummern, markiere alle Artikel in `%s' als nicht gelesen.", /* Newsgruppe Name */
/* < CANT_OPEN_KILL > */
    "Kann Killfile (`%s') nicht \366ffnen: %s.", /* file name, error string */
/* < CANT_OPEN_INCLUDED_KILL > */
    "Kann Killfile (`%s') nicht \366ffnen (included from `%s'): %s.", /* file name, parent file name, error string */
/* < MALFORMED_KILL_ENTRY > */
    "Error in KILL file entry `%s' in KILL file `%s': %s.", /* entry, file, reason for error */
/* < ERROR_INCLUDE_MISSING > */
    "No newsgroup or file name specified in include directive",
/* < ERROR_INCLUDE_NOT_SEPARATED > */
   " Include operand not separated",
/* < KILL_ERROR_UNKNOWN_OPTION > */
    "Error in KILL file entry `%s' in KILL file `%s': Unknown option `%s'.", /* entry, file, unknown option */
/* < UNKNOWN_KILL_REGEXP_ERROR > */
    "Unknown regular expression error in KILL file entry `%s' in KILL file `%s'.", /* entry */
/* < KNOWN_KILL_REGEXP_ERROR > */
    "Regular expression error in KILL file entry `%s' in KILL file `%s': %s.", /* entry, error string */
/* < KILL_TOO_LONG > */
    "Discarding too-long entry starting with `%s' in KILL file `%s'.", /* start of entry, file */
/* < NOT_IN_NEWSRC > */
    "Newsgruppe `%s' ist nicht in der Datei .newsrc.", /* Newsgruppe Name */
/* < BOGUS_NG_REMOVING > */
    "Newsgruppe `%s' existiert nicht, entferne sie aus der Datei .newsrc.", /* Newsgruppe Name */
/* < MISSING_NG_LISTING > */
    "Newsgruppe `%s' nicht im Cache gefunden. Hole Newsgruppen-Liste um sie zu finden.", /* newsgroup name */
/* < MAYBE_LIST > */
    "Newsgruppe `%s' nicht im Cache gefunden.\nNewsgruppen-Liste holen, um sie zu finden?", /* newsgroup name */
/* < DUP_NEWSRC_ENTRY > */
    "Doppelter Eintrag in der Datei .newsrc f\374r `%s', verwende den ersten.", /* newsgroup name */
/* < BAD_NEWSRC_LINE > */
    "Kann Zeile %d in der Datei .newsrc nicht interpretieren, \374bergehe sie.", /* line number */
/* < CANT_OPEN_NEWSRC_COPYING > */
    "Kann Datei .newsrc `%s' nicht kopieren: %s.", /* file name, Fehler string */
/* < CANT_EXPAND > */
    "Kann Dateinamen `%s' nicht erweitern.", /* file name */
/* < EMPTY_NEWSRC_SAVE_NAME > */
    "Dateiname zum Abspeichern der Datei .newsrc ist leer.",
/* < CANT_OPEN_NEWSRC_SAVE > */
    "Kann .newsrc-Datei `%s' nicht schreiben: %s.", /* file name, Fehler string */
/* < NEWSRC_SAVE_FILE_WRITE_ERR > */
    "Fehler beim Schreiben in .newsrc-Datei `%s': %s.", /* file name, Fehler string */
/* < CANT_READ_NEWSRC > */
    "Kann .newsrc-Datei `%s' nicht lesen: %s.", /* file Name, error string */
/* < CREATING_NEWSRC > */
    "Erzeuge .newsrc-Datei `%s'.", /* file Name */
/* < CANT_CREATE_NEWSRC > */
    "Kann .newsrc-Datei `%s' nicht erstellen: %s.", /* file name, Fehler string */
/* < CANT_STAT_NEWSRC > */
    "Kann Status der .newsrc-Datei `%s' nicht ermitteln: %s.", /* file name, Fehler string */
/* < ZERO_LENGTH_NEWSRC > */
    ".newsrc-Datei `%s' ist leer, gebe auf.", /* file Name */
/* < CANT_OPEN_NEWSRC > */
    "Kann .newsrc-Datei `%s' nicht lesen: %s.", /* file name, Fehler string */
/* < CANT_PARSE_NEWSRC > */
    "Kann .newsrc-Datei `%s' nicht interpretieren -- Fehler in Zeile %d.", /* file name, error line */
/* < CANT_OPEN_NEWSRC_TEMP > */
    "Kann .newsrc-Datei `%s' nicht schreiben: %s.", /* file name, Fehler string */
/* < CANT_OPEN_NEWSRC_WRITING > */
    "Kann .newsrc-Datei `%s' nicht schreiben: %s.", /* file name, Fehler string */ 
/* < ERROR_UNLINKING_NEWSRC > */
    "Fehler beim Entfernen des Links der .newsrc-Datei `%s': %s.", /* file name, Fehler string */
/* < ERROR_RENAMING > */
    "Fehler beim Umbenennen der Datei `%s' in `%s': %s.", /* temporary file name, file name, Fehler string */
/* < NO_MAIL_DIR > */
    "Kein Postverzeichnis `%s'.", /* directory Name */
/* < NO_SUCH_MAIL_DIR > */
    "Kein Postverzeichnis `%s';  Verzeichnis anlegen?", /* directory name */
/* < CANT_STAT_MAIL_DIR > */
    "Kann Status der Verzeichnisses `%s' nicht ermitteln: %s.", /* directory name, error string */
/* < MAIL_DIR_NOT_DIR > */
    "Kann Verzeichnis nicht anlegen -- Postpfad `%s' ist kein Verzeichnis.", /* directory Name */
/* < FOLDER_NOT_DIR > */
    "Pfad `%s' ist kein Verzeichnis.", /* folder Name */
/* < NO_SUCH_RMAIL > */
    "Keine RMAIL-Datei `%s';  Datei anlegen?", /* file name */
/* < CANT_OPEN_RMAIL > */
    "Kann RMAIL-Datei `%s' nicht \366ffnen: %s.", /* file name, Fehler string */
/* < CANT_WRITE_RMAIL > */
    "Kann nicht in die RMAIL-Datei `%s' schreiben: %s.", /* file name, Fehler string */
/* < UNKNOWN_CONFIRM_BUTTON > */
    "XRN Fehler: unbekannter Best\344tigungsknopf `%s'.", /* button Name */
/* < CANT_EXECUTE_CMD_POPEN > */
    "Konnte Befehl `%s' nicht ausf\374hren, `popen' fehlgeschlagen.", /* command string */
/* < CANT_EXPAND_DIR > */
    "Kann Verzeichnisname `%s' nicht erweitern.", /* directory name */
/* < CANT_CREATE_SAVE_DIR > */
    "Kann Verzeichnis `%s' nicht anlegen: %s.", /* drectory name, Fehler string */
/* < CANT_FIGURE_FILE_NAME > */
    "Kann Dateinamen `%s' nicht ermitteln.", /* file name */  
/* < CANT_CREAT_APPEND_SAVE_FILE > */
    "%s Datei `%s' nicht ausf\374hrbar: %s.", /* "create" or "append to", file name, Fehler string */
/* < ERROR_WRITING_FILE > */
/* < ERROR_WRITING_SAVE_FILE > */
    "Fehler beim Schreiben in Datei `%s': %s.", /* file name, Fehler string */
/* < CONNECTING > */
    "Verbindungsaufbau zum NNTP Server `%s'...", /* server name */
/* < GETTING_LIST > */
    "Lese die Liste der Newsgruppen...",
/* < GETTING_NEWGROUPS > */
    "Lese die Liste der neuen Newsgruppen...",
/* < FAILED_CONNECT > */
    "Verbindungsaufbau zum NNTP Server `%s' fehlgeschlagen .", /* server name */
/* < LOST_CONNECT_ATTEMPT_RE > */
    "Verbindung zum NNTP Server abgebrochen, versuche neuen Verbindungsaufbau.",
/* < RECONNECTED > */
    "Verbindung zum NNTP Server wieder hergestellt.",
/* < CANT_TEMP_NAME > */
    "Kann keinen tempor\344ren Dateinamen f\374r Artikel erzeugen.",
/* < CANT_CREATE_TEMP > */
    "Kann kann tempor\344re Datei `%s' nicht schreiben: %s.", /* file name, Fehler string */
/* < BOGUS_ACTIVE_ENTRY > */
    "\334bergehe fehlerhaften Eintrag `%s' der active-Datei.", /* entry */
/* < BOGUS_ACTIVE_CACHE > */
    "\334bergehe fehlerhaften Eintrag `%s' im Cache.", /* entry */
/* < XHDR_ERROR > */
    "XHDR Befehl beim NNTP Server fehlgeschlagen.\nEntweder unterst\374tzt der NNTP Server den XHDR Befehl nicht oder es ist ein interner Fehler in XRN aufgetreten.",
/* < NNTP_ERROR > */
    "NNTP Fehler: `%s'.", /* Fehler string */
/* < MALFORMED_XHDR_RESPONSE > */
    "NNTP Server sendete nicht erwartete XHDR Antwort.\nXHDR Befehl: `%s', Antwort `%s'.", /* command, response */
/* < NO_APP_DEFAULTS > */
    "Die XRN Application-Defaults-Datei ist nicht vorhanden. Es ist m\366glich das einige XRN Funktionen nicht gehen. Falls XRN von jemand anderem installiert wurde, teilen Sie ihm bitte diesen Fehler mit. Wenn Sie XRN installiert haben, lesen Sie bitte die Datei COMMON-PROBLEMS im XRN Quellcode Verzeichnis, dort steht wie Sie das Problem beheben k\366nnen.",
/* < VERSIONS > */
    "Application Defaults Version ist `%s',  XRN Version ist `%s'.", /* app-defaults version, executable version */
/* < NO_DOMAIN > */
    "Kann die Domain des Rechners nicht ermitteln, verwende die Environmentvariable DOMAIN oder die Resource domainName zum Versenden von Artikeln und Post.",
/* < NO_SERVER > */
    "Kann den NNTP Server nicht ermitteln, verwende die Environmentvariable NNTPSERVER, die Resource nntpServer oder die Option -nntpServer.",
/* < UNKNOWN_LIST_REGEXP_ERROR > */
    "Unbekannter Suchmusterfehler in %s im Listeneintrag `%s'; ignoriere Eintrag.", /* list name, entry */
/* < KNOWN_LIST_REGEXP_ERROR > */
    "Suchmusterfehler in %s im Listeneintrag `%s': %s; ignoriere Eintrag.", /* list name, entry, error string */
/* < OPEARATION_APPLY_CURSOR > */
   "Operationen sind abh\344nging von der aktuellen Auswahl und der Position des Zeigers",
/* < NO_MORE_UNREAD_ART > */
   "Keine weiteren ungelesenen Artikel in den abonnierten Newsgruppen.",
/* < SEL_GROUPS_ADDSUB > */
   "Zum Abonnieren Newsgruppen ausw\344hlen, verbleibende werden als `nicht abonniert' gekennzeichnet.",
/* < ARE_YOU_SURE > */
   "Sind Sie sicher?",
/* SUBED, UNSUBED and IGNORED strings must have the same len */
/* < SUBED >  */ 
   "abonniert      ",
/* < UNSUBED > */
   "nicht abonniert",
/* < IGNORED > */
   "ignoriert      ",
/* < OK_CATCHUP > */
   "Als gelesen markieren?",
/* < OK_CATCHUP_CUR > */ 
   "Bis zur aktuellen Position als gelesen markieren?",
/* < OK_GETLIST > */
    "Newsgruppen Liste vom Server holen?",
/* < OK_TO_UNSUB > */
   "Abonnement aufheben?",
/* < OK > */
    "OK",
/* < EDIT > */
    "Editieren",
/* < SEARCH_ABORTED > */
   "Suche wurde abgebrochen.",
/* < ERROR_SUBJ_SEARCH > */ 
   "Thema suchen: %s", /* regular expression */
/* < ERROR_SUBJ_EXH > */ 
   "Thema nicht vorhanden.",
/* < ERROR_SUBJ_ABORT > */
   "Suche abgebrochen.",
/* < KILL_DONE > */
   "Gehe zum ersten ungelesenen Artikel.",
/* < UNKNOWN_KILL_TYPE > */
    "Unknown kill type \"%s\" in \"%s\" kill request.", /* field type argument, field name */
/* < ERROR_CANT_UPDATE_NEWSRC > */
   "Kann die Datei .newsrc nicht aktualisieren.",
/* < ARTICLE_NUMBER > */ 
   "Artikel Nummer:",
/* < ERROR_SUBJ_EXPR > */
   "Suche nach %s: Keine Eintr\344ge gefunden.", /* regular expression */
/* < ERROR_SEARCH > */ 
   "Suche nach %s", /* regular expression */
/* < REGULAR_EXPR > */
   "Suchmuster:",
/* < BEHIND_WHAT_GROUP > */
   "Nach welcher Newsgrupppe?",
/* < ARTICLE_QUEUED > */
   "Artikel nacheinander versand.",
/* < GROUP_SUB_TO > */
   "Zu abonnierende Gruppe:",
/* < GROUP_TO_GO > */ 
   "Gehen zur Gruppe:",
/* < VIEW_ALLNG_SUB > */
   "Anzeigen aller Gruppen mit der M\366glichkeit zum Abonnieren.",
/* < AUTOMATIC_RESCAN > */
   "Automatische Abfrage des Servers wird ausgef\374hrt...",
/* < RESCANNING_BACKGROUND > */
    "Abfrage des Servers im Hintergrund...",
/* < ERROR_UNSUP_TRANS > */
   "Nicht unterst\344tzte \334bersetzung: %d nach %d", /* transition from, to */
/* < POST_FOLLOWUP > */
   "Artikel",
/* < FOLLOWUP_REPLY > */ 
   "Artikel und Nachricht",
/* < DEFAULT_MAIL > */
   "Nachricht",
/* < SAVE_IN > */
   "Sichere in %s",  /* file */
/* < ERROR_SEND_MAIL > */
   "Fehler beim Versenden einer Nachricht:",
/* < ASK_FILE > */
   "Dateiname?",
/* < ASK_POST_ARTICLE > */
   "Artikel ver\366ffentlichen?",
/* < ASK_POST_SEND > */
   "Nachricht ver\366ffentlichen und/oder versenden?",
/* < ASK_RE_EDIT_ARTCILE > */
   "Artikel nochmals bearbeiten?",
/* < ASK_RE_EDIT > */
   "Nachricht nochmals bearbeiten?",
/* < ERROR_EXEC_FAILED > */
   "XRN Fehler: execl von `%s' fehlgeschlagen\n", /* prog */
/* < ASK_POSTER_FANDR > */
   "`Followup-To' Zeile der Nachricht lautet `an Verfasser'.\nEintrag ignorieren und Artikel auch ver\366ffentlichen oder nur Nachricht an Verfasser senden?",
/* < ASK_POSTER_REPLY > */
   "`Followup-To' Zeile der Nachricht lautet `an Verfasser'.\nArtikel ver\366ffentlichen oder Nachricht an Verfasser senden?",
/* < POST_AND_SEND > */
   "Bezug und Nachricht",
/* < SEND_MAIL > */
   "Nachricht",
/* < POST > */ 
   "Artikel",
/* < FOLLOWUP_MULTIPLE_NGS > */
    "Die Standard 'Newsgroups'-Zeile Ihres Bezuges enth\344lt mehrere Newsgruppen. Bitte stellen Sie sicher, da\337 unzutreffende Newsgruppen entfernt sind bevor Sie den Artikel versenden und/oder f\374gen Sie zutreffendere Newsgruppen in die 'Followup-To'-Zeile ein.",
/* < FOLLOWUP_FOLLOWUPTO > */
    "Beachten Sie bitte, da\337 der Artikel, auf den Sie antworten, eine 'Followup-To'-Zeile enth\344lt. Deshalb wurde die Standard 'Newsgroups'-Zeile Ihres Bezuges auf den Inhalt dieser Zeile gesetzt, statt auf die 'Newsgroups'-Zeile des originalen Artikels.",
/* < CROSSPOST_PROHIBIT > */
    "Die `Newsgroups'-Zeile Ihres Artikels enth\344lt %d Newsgruppen. Die maximale\nAnzahl von Newsgruppen an die Sie versenden d\374rfen betr\344gt %d. Bitte\nreduzieren Sie die Anzahl der Newsgruppen an die Sie versenden m\366chten und versenden\nSie dann Ihren Artikel erneut.",
/* < CROSSPOST_CONFIRM > */
    "Die `Newsgroups'-Zeile Ihres Artikels enth\344lt %d Newsgruppen. Bitte\nerw\344gen Sie eine Reduzierung der Newsgruppen an die Sie versenden m\366chten.",
/* < FOLLOWUP_FOLLOWUPTO_CONFIRM > */
    "Die 'Newsgroups'-Zeile Ihres Artikels enth\344lt %d Newsgruppen und die\n`Followup-To'-Zeile enth\344lt %d Newsgruppen. Bitte erw\344gen Sie eine Reduzierung der\nNewsgruppen in Ihrer `Newsgroups'- und/oder `Followup-To'-Zeile.",
/* < FOLLOWUP_CONFIRM > */
    "Die 'Newsgroups'-Zeile Ihres Artikels enth\344lt %d Newsgruppen. Bitte erw\344gen Sie\nentweder eine Reduzierung der Anzahl der Newsgruppen an die Sie versenden m\366chten oder ein Hinzuf\374gen einer\n`Followup-To'-Zeile, die eine geringere Anzahl an Newsgruppen enth\344lt.",
/* < ERROR_STRIPFIELD_NL > */
   "Kein Zeilenvorschub in stripField gefunden.\n",
/* < FOLLOWUP_REPLY_TO_TITLE > */
   "Bezug und Nachricht zu Artikel %ld in %s", /* article number, newsgroup */
/* < FOLLOWUP_TO_TITLE > */
   "Bezug zu Artikel %ld in %s", /* article number, newsgroup */
/* < REPLY_TO_TITLE > */
   "Nachricht zu Artikel %ld in %s", /* article number, newsgroup */
/* < FORWARD_TO_TITLE > */
   "Weiterreichen des Artikel %ld in `%s' an einen anderen Benutzer", /* article number, newsgroup */
/* < POST_ARTICLE > */
   "Artikel ver\366ffentlichen.",
/* < POST_ARTICLE_TO > */
   "Artikel in `%s' ver\366ffentlichen.", /* newsgroup */
/* < POST_MAIL_ARTICLE > */
   "Artikel ver\366ffentlichen und versenden.",
/* < POST_MAIL_ARTICLE_TO > */
   "Ver\366ffentliche Artikel an `%s' und versende ihn.",
/* < MAIL > */
   "Versenden einer E-Mail.",
/* < USER_CANT_CANCEL > */
   "Sie sind nicht berechtigt den Artikel zur\374ckzuziehen.",
/* 
 ### Die folgenden Texte sollten evtl. nicht uebersetzt werden ###
 */
/* < REPLY_YOU_WRITE > */
   "In article %s, you write:\n", /* messageid */
/* < FORWARDED_ARTIKEL > */
   "\n------ Forwarded Article %s\n------ From %s\n\n", /* messageid , author */
/* < FORWARDED_ARTICLE_END > */
   "\n------ End of Forwarded Article\n",
/* < FOLLOWUP_AUTHOR_WRITE > */
    "In article %s, %s writes:\n", /* messageid , author */
/* #### Ende des evtl. nicht uebersetzen #### */
/* < NEWSGROUPS_INDEX > */
    "%10s %7s %*s %4d Artikel%1.1s +%5d alt",
/* < UNREAD > */
    "Ungelesene",
/* < NEWS_IN > */
    "Nachrichten in",
/* < NOT_ONE > */
    " ", /* see NEWSGROUPS_INDEX in STRING section */    
/* < DONE > */
    "erledigt",
/* < ABORTED > */
    "abgebrochen",
/* < FAILED > */
    "fehlgeschlagen",
/* < CREATE > */
    "erzeugen",
/* < APPEND > */
    "anh\344ngen an",
/* < ERR_XRN_RUN > */
   "XRN l\344uft bereits auf %s als Proze\337 %d.\nFalls es nicht mehr l\344uft entfernen sie die Datei `%s'.\n", /* host, pid, lockfile */
/* < ERROR_CANT_READ_NEWSRC > */
   "Kann Datei .newsrc nicht lesen",
/* < PROCESS_KILL_FOR > */
   "Bearbeite KILL-Datei f\374 Newsgruppe `%s'...", /* newsgroup */
/* < ERROR_REGEX_NOSLASH > */
   "Fehlender Schr\344gstrich `/' am Ende des Suchmusters",
/* < ERROR_REGEX_NOSLASH_START > */
    "no slash preceding the regular expression",
/* < ERROR_REGEX_NOCOLON > */
   "Kein Komma nach dem Suchmuster",
/* < ERROR_REGEX_UNKNOWN_COMMAND > */
   "Unbekannter Befehl im Suchmuster (Erlaubt: `j', `m' und `s')",
/* < KILL_LINE > */
    "Processing entry `%s' in KILL file `%s'.",
/* < KILL_KILLED > */
   "gel\366scht - %s",    /* subject */
/* < KILL_UNREAD > */
   "nicht gelesen - %s",  /* subject */
/* < KILL_SAVED > */
   "gesichert - %s",      /* subject */
/* < COUNT_KILLED > */
   "%d gel\366schte Artikel%s in %s", /* count, "" or NOT_ONE_STRING , newsgroup */
/* < COUNT_UNREAD > */
   "%d als ungelesen markierte Artikel%s in %s", /* count, "" or NOT_ONE_STRING , newsgroup */
/* < COUNT_SAVED > */
   "%d gespeicherte Artikel%s in %s", /* count, "" or NOT_ONE_STRING , newsgroup */
/* < ERROR_CORNERED > */
   "XRN Fehler in `cornered': erwarte g\374ltige nglist\n",
/* < ERROR_OUT_OF_MEM > */
   "kein Speicher mehr verf\374gbar",
/* < PREFETCHING > */
    "Einlesen von `%s'...",
/* < SERVER_POSTING_ERROR > */
    "Fehler des NNTP Servers: %s", /* error message */
/* < ASK_SAVEBOX > */
   "Dateiname, +Dateiname oder @Dateiname?",
/* < SAVE_PIPE_TO > */
   "Leite Artikel %ld zu Befehl `%s'...",  /* articlenumber, command */
/* < ERROR_SAVE_PIPE > */
   "Befehl `%s' beendet mit R\374ckmeldung %d.", /* command, status */
/* < SAVE_MH_REFILE > */
   "MH zu Verzeichnis %s %s", /* folder, status */
/* < SAVE_RMAIL_REFILE > */
   "RMAIL zu Verzeichnis %s %s", /* folder, status */
/* < SAVE_OK > */
   "Speichere Artikel %ld in Datei `%s'...", /* articlenumber, filename */
/* < SAVE_APPEND_OK > */
   "H\344nge Artikel %ld an Datei `%s' an...", /* articlenumber, filename */
/* < SAVE_ARTICLE > */
   "Artikel: %ld aus %s\n", /* articlenumber, newsgroup */
/* < ERROR_INFINITE_LOOP > */
   "XRN Fehler: moveBeginning / moveEnd in Endlosschleife",
/* < ERROR_FINDARTICLE > */
   "G\374ltige Artikelnummer nicht gefunden in findArticle",
/* < ERROR_STRIP_LEAVE_HEADERS > */
   "Es darf nur eine der Ressourcen `stripHeaders', `leaveHeaders' angegeben werden.",
/* < ERROR_REQUEST_FAILED > */
   "        Anfrage war: `%s'\n        R\374ckmeldung war: `%s'", /* command, message */
/* < ASK_UPDATE_NEWSRC > */
    "Datei .newsrc wurde von einem anderen Programm ver\344ndert. Trotzdem schreiben?",
/* < PENDING_COMPOSITION > */
    "Verlassen nicht m\366glich, solange noch ein Artikel verfa\374t wird!",
/* < NNTP_PASSWORD > */
    "Enter NNTP password:",
/* < UNKNOWN_SORT_TYPE > */
    "Unknown subject sort type: %s",
/* <TOO_MANY_SORT_TYPES > */
    "Too many subject sort types specified.",
/* < UNPARSEABLE_DATE > */
    "Unparseable date (article %ld in %s): %s",
/* < THREADING_FOR > */
    "Threading newsgroup `%s'...", /* newsgroup */
};

#endif /* XRN_LANG_german */
