#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: mesg_strings.c,v 1.88 1998-03-23 13:21:31 jik Exp $";
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
#ifndef XRN_LANG_french
#define XRN_LANG_english
#endif
#endif
#endif

/*
 * Global messsages and strings like "yes", "no", "OK?", etc.  These
 * are #define's instead of strings in the mesg_strings array because
 * they are used in static arrays that can't contain references to
 * other arrays.
 */

#ifdef XRN_LANG_english

#define YES_STRING   "Yes"
#define NO_STRING    "No"
#define ABORT_STRING "Abort"
#define DOIT_STRING  "Do it"
#define SAVE_STRING  "Save"

#define ADD_STRING        "Add"
#define FORWARD_STRING    "Forward"
#define BACK_STRING       "Back"
#define LAST_GROUP_STRING "Last group"
#define FIRST_STRING      "First"
#define LAST_STRING       "Last"
#define CURSOR_POS_STRING "Cursor position"
#define SUB_STRING        "Subscribe"
#define GOTO_NG_STRING    "Go to newsgroup"
#define CLICK_TO_CONT_STRING "Click to continue"

/* Strings to compose newsgroup line for ngMode */
/* The format is NEWSGROUPS_INDEX_MSG.  The strings in it are:
 * 1) UNREAD_MSG or empty string 
 * 2) NEWS_IN_MSG or empty string 
 * 3) newsgroup
 * 4) articles unread
 * 5) NOT_ONE_MSG if *one* unread article is available, else " "
 *
 * example:"Unread news in comp.sys.ibm                            30 articles +   20 old"                  
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


#ifdef XRN_LANG_french

#define YES_STRING   "Oui"
#define NO_STRING    "Non"
#define ABORT_STRING "Annuler"
#define DOIT_STRING  "Continuer"
#define SAVE_STRING  "Sauver"

#define ADD_STRING        "Ajouter"
#define FORWARD_STRING    "Avancer"
#define BACK_STRING       "Reculer"
#define LAST_GROUP_STRING "Dernier groupe"
#define FIRST_STRING      "Premier"
#define LAST_STRING       "Dernier"
#define CURSOR_POS_STRING "Position courante"
#define SUB_STRING        "S'abonner"
#define GOTO_NG_STRING    "Aller au newsgroup"
#define CLICK_TO_CONT_STRING "Continuer"

/* Strings to compose newsgroup line for ngMode */
/* The format is NEWSGROUPS_INDEX_MSG.  The strings in it are:
 * 1) UNREAD_MSG ou une chaÅÓne vide
 * 2) NEWS_IN_MSG ou une chaÅÓne vide
 * 3) groupe de news
 * 4) articles non lus
 * 5) NOT_ONE_MSG si *un* article non lu est disponible, sinon " "
 *
 * exemple:"(non lus) Messages dans comp.sys.ibm       30 articles + 20 anciens"
 * Note: la longueur de ligne maximale est normalement 200
 */
/* The number of characters into a newsgroup index line after which
   the newsgroup name actually starts. */
#define NEWS_GROUP_OFFSET 24
/*
  The number of characters on a newsgroup index line, other than the
  newsgroup name, and not including the newline or the null at the
  end.
  */
#define NEWS_GROUP_LINE_CHARS 53
/* end strings for ngMode */

/* strings for buildQuestion, bottomline in ngMode */
/* max len after sprintf is LABEL_SIZE (128)      */
#define QUEST_ART_NOUNREAD_NONEXT_STRING  "Art. %ld de %s"
#define QUEST_ART_NOUNREAD_NEXT_STRING    "Art. %ld de %s (Suivant: %s, %ld article%s)"
#define QUEST_ART_UNREAD_NONEXT_STRING    "Art. %ld de %s (%ld restant)"
#define QUEST_ART_UNREAD_NEXT_STRING      "Art. %ld de %s (%ld restant) (Suivant: %s, %ld article%s)"
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
#define UNREAD_MARKER	'n'
#define SAVED_MARKER	'S'
#define PRINTED_MARKER	'I'

#endif /* XRN_LANG_french */


#ifdef XRN_LANG_german

#define YES_STRING   "Ja"
#define NO_STRING    "Nein"
#define ABORT_STRING "Abbrechen"
#define DOIT_STRING  "Ausf\374hren"
#define SAVE_STRING  "Sichern"

#define ADD_STRING        "Hinzuf\374gen"
#define FORWARD_STRING    "Weiter"
#define BACK_STRING       "Zur\374ck"
#define LAST_GROUP_STRING "Letzte Gruppe"
#define FIRST_STRING      "Anfang"
#define LAST_STRING       "Ende"
#define CURSOR_POS_STRING "Aktuelle Position"
#define SUB_STRING        "Abonnieren"
#define GOTO_NG_STRING    "Gehe zu Gruppe"
#define CLICK_TO_CONT_STRING "Fortsetzen"

/* Strings to compose newsgroup line for ngMode */
/* The format is NEWSGROUPS_INDEX_MSG.  The strings in it are:
 * 1) UNREAD_MSG oder leerer string
 * 2) NEWS_IN_MSG oder leerer string
 * 3) Newsgruppe
 * 4) Artikel ungelesen
 * 5) NOT_ONE_MSG wenn *ein* ungelesener Artikel, sonst " "
 *
 * Beispiel:"Ungelesene Nachrichten in comp.sys.ibm.misc                        30 Artikel  + 20 alt"                 
 * Note: maximale Zeilenlaenge ist normalerweise 200
 */
/* The number of characters into a newsgroup index line after which
   the newsgroup name actually starts. */
#define NEWS_GROUP_OFFSET 16
/*
  The number of characters on a newsgroup index line, other than the
  newsgroup name, and not including the newline or the null at the
  end.
  */
#define NEWS_GROUP_LINE_CHARS 42
/* end strings for ngMode */

/* strings for buildQuestion, bottomline in ngMode */
/* max len after sprintf is LABEL_SIZE (128)      */
#define QUEST_ART_NOUNREAD_NONEXT_STRING  "Art. %ld in %s"
#define QUEST_ART_NOUNREAD_NEXT_STRING    "Art. %ld in %s (N\344chste: %s, %d Artikel)%s"
#define QUEST_ART_UNREAD_NONEXT_STRING    "Art. %ld in %s (%ld \374brig)"
#define QUEST_ART_UNREAD_NEXT_STRING      "Art. %ld in %s (%ld \374brig) (N\344chste: %s, %d Artikel)%s"
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
 * Messages for use in mesgPane calls.  Many of these messages are
 * used multiple times, which is why they are constants.  Also,
 * putting them all here makes it easy to make sure they're all
 * consistent.  Finally, putting them here saves space in the
 * executable since there's only one copy of each string.
 */

#ifdef XRN_LANG_english

char *message_strings[] = {
/* < BAD_BUTTON_NAME > */
    "XRN error: bad button name `%s'.", /* button name */
/* < NO_SUCH_NG_DELETED > */
    "Newsgroup `%s' does not exist.\n\tIt may have been deleted.", /* newsgroup name */
/* < UNKNOWN_FUNC_RESPONSE > */
    "Internal XRN error: unknown response %d from %s in %s.", /* return value, called function, calling function */
/* < DISPLAYING_LAST_UNREAD > */
    "No unread articles in `%s'.\n\tDisplaying last available article.", /* newsgroup name */
/* < PROBABLY_KILLED > */
    "No unread articles in `%s'.\n\tThey were probably killed.", /* newsgroup name */
/* < NO_ARTICLES > */
    "No articles in `%s'.", /* newsgroup name */
/* < PROBABLY_EXPIRED > */
    "No articles in `%s'.\n\tThey were probably expired or cancelled.", /* newsgroup name */
/* < NO_NG_SPECIFIED > */
    "No newsgroup name specified.",
/* < NO_SUCH_NG > */
    "Newsgroup `%s' does not exist.", /* newsgroup name */
/* < NO_PREV_NG > */
    "No previous newsgroup.",
/* < NO_GROUPS_SELECTED > */
    "No newsgroups were selected.",
/* < NG_NOT_MOVED > */
    "New position for newsgroups can't be\n\tin block of selected newsgroups.\n\tNewsgroups have not been moved.",
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
    "Cannot determine save file name.\n\tArticle/message not saved.",
/* < NO_SUBJECT > */
    "The Subject field is missing in your message!\n\t",
/* < EMPTY_SUBJECT > */
    "The Subject field in your message is empty!\n\t",
/* < NO_NEWSGROUPS > */
    "The Newsgroups field is missing in your message!\n\t",
/* < MULTI > */
    "There are multiple %s fields in your message!\n\tPlease delete all but one of them\n\tand resend.", /* field name */
/* < DEFAULT_ADDED > */
    "A default value has been added.\n\tPlease edit it as necessary\n\tand resend.",
/* < EMPTY_ADDED > */
    "An empty one has been added.\n\t",
/* < FILL_IN_RESEND > */
    "Please fill it in and resend.",
/* < NO_POSTABLE_NG > */
    "No postable newsgroups in `Newsgroups' line.\n\tPlease fix it and resend or save and abort.",
/* < SAVING_DEAD > */
    "Saving in `%s'.", /* file name */
/* < COULDNT_POST > */
    "Could not post article.",
/* < POST_NOTALLOWED > */
    "Posting not allowed from this machine.",
/* < COULDNT_SEND > */
    "Could not send mail message.",
/* < MAILED_TO_MODERATOR > */
    "One or more moderated newsgroups in `Newsgroups' line of message.\n\tArticle will be mailed to moderator by server.",
/* < ARTICLE_POSTED > */
    "Article posted.",
/* < MAIL_MESSAGE_SENT > */
    "Mail message sent.",
/* < CANT_INCLUDE_CMD > */
    "Cannot execute includeCommand (`popen' failed).",
/* < CANT_OPEN_ART > */
    "Cannot open article file `%s':\n\t%s.", /* file name, error string */
/* < CANT_OPEN_FILE > */
    "Cannot open file `%s':\n\t%s.", /* error string */
/* < NO_FILE_SPECIFIED > */
    "No file specified.",
/* < CANT_OPEN_TEMP > */
    "Cannot open temporary file `%s':\n\t%s.", /* file name, error string */
/* < CANT_STAT_TEMP > */
    "Cannot stat temporary file `%s':\n\t%s.", /* file name, error string */
/* < NO_CHANGE > */
    "No change in temporary file `%s'.", /* file name */
/* < ZERO_SIZE > */
    "Temporary file `%s'\n\thas zero size.", /* file name */
/* < NO_MSG_TEMPLATE > */
    "Internal XRN error: no message template in call to Call_Editor.",
/* < CANT_EDITOR_CMD > */
    "Cannot execute editor command `%s':\n\t%s.", /* command, error string */
/* < ONE_COMPOSITION_ONLY > */
    "Only one composition allowed at a time.",
/* < EXECUTING_SIGNATURE > */
    "Executing signature command `%s'.", /* command */
/* < CANT_EXECUTE_SIGNATURE > */
    "Cannot execute signature file `%s'.\n\tReading instead.", /* signature file name */
/* < READING_SIGNATURE > */
    "Reading signature file `%s'.", /* signature file name */
/* < CANT_READ_SIGNATURE > */
    "Cannot read signature file `%s':\n\t%s.", /* signature file name, error string */
/* < SIGNATURE_TOO_BIG > */
    "Signature file `%s'\n\tis too large; ignoring it.", /* signature file name */
/* < CANCEL_ABORTED > */
    "Article not cancelled.",
/* < CANCELLED_ART > */
    "Article has been cancelled.",
/* < CANCEL_TO_MODERATOR > */
    "Message being canceled appears\n\tin one or more moderated newsgroups.\n\tCancel request will be\n\tmailed to moderator by server.",
/* < UNKNOWN_REGEXP_ERROR > */
    "Unknown error in regular expression `%s'.", /* regexp string */
/* < KNOWN_REGEXP_ERROR > */
    "Error in regular expression `%s':\n\t%s.", /* regexp, error string */
/* < ART_NUMBERING_PROBLEM > */
    "Article numbering problem.\n\tMarking all articles in newsgroup `%s' unread.", /* newsgroup name */
/* < CANT_OPEN_KILL > */
    "Cannot open kill file `%s':\n\t%s.", /* file name, error string */
/* < CANT_OPEN_INCLUDED_KILL > */
    "Cannot open kill file `%s'\n\t(included from `%s'):\n\t%s.", /* file name, parent file name, error string */
/* < MALFORMED_KILL_ENTRY > */
    "Error in KILL file entry `%s'\n\tin KILL file `%s':\n\t%s.", /* entry, file, reason for error */
/* < ERROR_INCLUDE_MISSING > */
    "No newsgroup or file name specified in include directive",
/* < ERROR_INCLUDE_NOT_SEPARATED > */
   " Include operand not separated",
/* < KILL_ERROR_UNKNOWN_OPTION > */
    "Error in KILL file entry `%s'\n\tin KILL file `%s':\n\tUnknown option `%c'.", /* entry, file, unknown option */
/* < UNKNOWN_KILL_REGEXP_ERROR > */
    "Unknown regular expression error in KILL file entry `%s' in KILL file `%s'.", /* entry */
/* < KNOWN_KILL_REGEXP_ERROR > */
    "Regular expression error in KILL file entry `%s'\n\tin KILL file `%s': %s.", /* entry, error string */
/* < KILL_TOO_LONG > */
    "Discarding too-long entry starting with `%s'\n\tin KILL file `%s'.", /* start of entry, file */
/* < NOT_IN_NEWSRC > */
    "Newsgroup `%s' is not in your .newsrc file.", /* newsgroup name */
/* < BOGUS_NG_REMOVING > */
    "Newsgroup `%s' does not exist.\n\tRemoving it from your .newsrc file.", /* newsgroup name */
/* < MISSING_NG_LISTING > */
    "Newsgroup `%s' not found in cache.\n\tRetrieving newsgroup list to find it.", /* newsgroup name */
/* < MAYBE_LIST > */
    "Newsgroup `%s' not found in cache.\nRetrieve newsgroup list to find it?", /* newsgroup name */
/* < DUP_NEWSRC_ENTRY > */
    "Duplicate .newsrc entry for newsgroup `%s'.\n\tUsing the first one.", /* newsgroup name */
/* < BAD_NEWSRC_LINE > */
    "Unable to parse line %d in .newsrc file.\n\tIgnoring it.", /* line number */
/* < CANT_OPEN_NEWSRC_COPYING > */
    "Cannot open .newsrc file `%s'\n\tfor copying: %s.", /* file name, error string */
/* < CANT_EXPAND > */
    "Cannot expand file name `%s'.", /* file name */
/* < EMPTY_NEWSRC_SAVE_NAME > */
    ".newsrc save file name is the empty string.",
/* < CANT_OPEN_NEWSRC_SAVE > */
    "Cannot open .newsrc save file `%s'\n\tfor writing: %s.", /* file name, error string */
/* < NEWSRC_SAVE_FILE_WRITE_ERR > */
    "Error writing to .newsrc save file `%s':\n\t%s.", /* file name, error string */
/* < CANT_READ_NEWSRC > */
    "Cannot read .newsrc file `%s':\n\t%s.", /* file name, error string */
/* < CREATING_NEWSRC > */
    "Creating .newsrc file `%s' for you.", /* file name */
/* < CANT_CREATE_NEWSRC > */
    "Cannot create .newsrc file `%s':\n\t%s.", /* file name, error string */
/* < CANT_STAT_NEWSRC > */
    "Cannot stat .newsrc file `%s':\n\t%s.", /* file name, error string */
/* < ZERO_LENGTH_NEWSRC > */
    ".newsrc file `%s' is empty.\n\tAborting.", /* file name */
/* < CANT_OPEN_NEWSRC > */
    "Cannot open .newsrc file `%s'\n\tfor reading: %s.", /* file name, error string */
/* < CANT_PARSE_NEWSRC > */
    "Cannot parse .newsrc file `%s' --\n\terror on line %d.", /* file name, error line */
/* < CANT_OPEN_NEWSRC_TEMP > */
    "Cannot open .newsrc temporary file `%s'\n\tfor writing: %s.", /* file name, error string */
/* < CANT_OPEN_NEWSRC_WRITING > */
    "Cannot open .newsrc file `%s'\n\tfor writing: %s.", /* file name, error string */
/* < ERROR_UNLINKING_NEWSRC > */
    "Error unlinking .newsrc file `%s':\n\t%s.", /* file name, error string */
/* < ERROR_RENAMING > */
    "Error renaming temporary file `%s'\n\tto file `%s':\n\t%s.", /* temporary file name, file name, error string */
/* < NO_MAIL_DIR > */
    "No Mail directory `%s'.", /* directory name */
/* < NO_SUCH_MAIL_DIR > */
    "No such folder `%s';\nCreate it?", /* directory name */
/* < CANT_STAT_MAIL_DIR > */
    "Cannot stat directory `%s':\n\t%s.", /* directory name, error string */
/* < MAIL_DIR_NOT_DIR > */
    "Cannot create folder --\n\tmail path `%s' is not a directory.", /* directory name */
/* < FOLDER_NOT_DIR > */
    "Path `%s' is not a folder.", /* folder name */
/* < NO_SUCH_RMAIL > */
    "No such RMAIL file `%s'.\nCreate it?", /* file name */
/* < CANT_OPEN_RMAIL > */
    "Cannot open RMAIL file `%s'\n\tfor writing: %s.", /* file name, error string */
/* < CANT_WRITE_RMAIL > */
    "Cannot write to RMAIL file `%s':\n\t%s.", /* file name, error string */
/* < UNKNOWN_CONFIRM_BUTTON > */
    "XRN error: unknown confirm button `%s'.", /* button name */
/* < CANT_EXECUTE_CMD_POPEN > */
    "Cannot execute command `%s' (`popen' failed).", /* command string */
/* < CANT_EXPAND_DIR > */
    "Cannot expand directory `%s'.", /* directory name */
/* < CANT_CREATE_SAVE_DIR > */
    "Cannot create save directory `%s':\n\t%s.", /* drectory name, error string */
/* < CANT_FIGURE_FILE_NAME > */
    "Cannot figure out file name `%s'.", /* file name */
/* < CANT_CREAT_APPEND_SAVE_FILE > */
    "Cannot %s file `%s':\n\t%s.", /* "create" or "append to", file name, error string */
/* < ERROR_WRITING_FILE > */
/* < ERROR_WRITING_SAVE_FILE > */
    "Error writing to file `%s':\n\t%s.", /* file name, error string */
/* < CONNECTING > */
    "Connecting to NNTP server `%s'...", /* server name */
/* < GETTING_LIST > */
    "Getting list of newsgroups...",
/* < GETTING_NEWGROUPS > */
    "Getting list of new newsgroups...",
/* < FAILED_CONNECT > */
    "Failed to connect to NNTP server `%s'.", /* server name */
/* < LOST_CONNECT_ATTEMPT_RE > */
    "Lost connection to the NNTP server.\n\tAttempting to reconnect.",
/* < RECONNECTED > */
    "Reconnected to the NNTP server.",
/* < CANT_TEMP_NAME > */
    "Cannot create temporary file name for article.",
/* < CANT_CREATE_TEMP > */
    "Cannot open temporary file `%s'\n\tfor writing: %s.", /* file name, error string */
/* < BOGUS_ACTIVE_ENTRY > */
    "Skipping bogus active file entry `%s'.", /* entry */
/* < BOGUS_ACTIVE_CACHE > */
    "Skipping bogus active cache entry `%s'.", /* entry */
/* < XHDR_ERROR > */
    "XHDR command to the NNTP server failed.\n\tEither the NNTP server does not support XHDR\n\t(in which case XRN will not work),\n\tor an internal XRN error occurred.",
/* < NNTP_ERROR > */
    "NNTP serious error: `%s'.", /* error string */
/* < MALFORMED_XHDR_RESPONSE > */
    "NNTP server sent malformed XHDR response.\n\tXHDR command was `%s',\n\tresponse was `%s'.", /* command, response */
/* < NO_APP_DEFAULTS > */
    "The current XRN Application Defaults file is not installed.\n\tAs a result, some XRN functionality may be missing.\n\tIf XRN was installed by someone else at your site,\n\tcontact the installer about this error.\n\tIf you are the installer, see the\n\tCOMMON-PROBLMS file in the XRN source directory\n\tto find out how to fix this problem.\n\t",
/* < VERSIONS > */
    "Installed Application Defaults file version is `%s'.\n\tXRN executable version is `%s'.", /* app-defaults version, executable version */
/* < NO_DOMAIN > */
    "Could not determine your host's domain.\n\tRerun XRN with the DOMAIN environment variable\nor the domainName X resource set\nin order to post or send mail.",
/* < NO_SERVER > */
    "Could not determine the news server.\nRerun XRN with the NNTPSERVER environment variable,\nthe nntpServer X resource or the -nntpServer\ncommand line argument.",
/* < UNKNOWN_LIST_REGEXP_ERROR > */
    "Unknown regular expression error\n\tin %s list entry `%s';\n\tentry ignored.", /* list name, entry */
/* < KNOWN_LIST_REGEXP_ERROR > */
    "Regular expression error\n\tin %s list entry `%s':\n\t%s; entry ignored.", /* list name, entry, error string */
/* < OPEARATION_APPLY_CURSOR > */
    "Operations apply to current selection or cursor position",
/* < NO_MORE_UNREAD_ART > */
    "No more unread articles in the subscribed to newsgroups",
/* < SEL_GROUPS_ADDSUB > */
    "Select groups to `add', `quit' unsubscribes remaining groups",
/* < ARE_YOU_SURE > */
    "Are you sure?",
/* SUB, UNSUB and IGNORED strings must have the same len */
/* < SUBED > */ 
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
/* < ERROR_NO_PARENT > */
    "Article has no parent.",
/* < ERROR_PARENT_UNAVAIL > */
    "Article's parent is unavailable.",
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
/* < LIST_OLD_NUMBER > */ 
    "First article to list:",
/* < ERROR_SUBJ_EXPR > */
    "Search for expression %s: no match was found", /* regular expression */
/* < ERROR_SEARCH > */ 
    "Search for expression %s", /* regular expression */
/* < REGULAR_EXPR > */
    "Regular Expression:",
/* < BEHIND_WHAT_GROUP > */
    "After which newsgroup?",
/* < ARTICLE_QUEUED > */
    "Article successfully queued",
/* < GROUP_SUB_TO > */
    "Group to subscribe to:",
/* < GROUP_TO_GO > */ 
    "Group to go to:",
/* < VIEW_ALLNG_SUB > */
    "View all available groups, with option to subscribe",
/* < SUB_DONE > */
    "You are now subscribed to `%s'.", /* newsgroup name */
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
/* < ASK_SEND > */
    "Send the message?",
/* < ASK_POST_SEND > */
    "Post and send the message?",
/* < RE_EDIT > */
    "Re-edit",
/* < AS_FOLLOWUP > */
    "as followup",
/* < AS_REPLY > */
    "as reply",
/* < AS_FOLLOWUP_REPLY > */
   "as followup/reply",
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
    "The default `Newsgroups' line of your followup contains multiple newsgroups.\n\tPlease be sure to remove inappropriate newsgroups before sending your message,\n\tand/or to put a more appropriate list of groups in your `Followup-To' line.",
/* < FOLLOWUP_FOLLOWUPTO > */
    "Note that the article to which you are responding contains a `Followup-To' line,\n\tso the default `Newsgroups' line of your followup has been set from that line\n\trather than from the `Newsgroups' line of the original article.",
/* < CROSSPOST_PROHIBIT > */
    "The `Newsgroups' line of your message contains %d newsgroups.\n\tThe maximum number of groups to which you are allowed to post a message is %d.\n\tPlease reduce the number of groups to which you are posting\n\tand then send your message again.",
/* < CROSSPOST_CONFIRM > */
    "The `Newsgroups' line of your message contains %d newsgroups.\n\tPlease consider reducing the number of groups to which you are posting.",
/* < FOLLOWUP_FOLLOWUPTO_CONFIRM > */
    "The `Newsgroups' line of your message contains %d newsgroups,\n\tand the `Followup-To' line contains %d newsgroups.\n\tPlease consider reducing the number of groups in your `Newsgroups' and/or `Followup-To' line.",
/* < FOLLOWUP_CONFIRM > */
    "The `Newsgroups' line of your message contains %d newsgroups.\n\tPlease consider either reducing the number of newsgroups to which you are posting\n\tor adding a `Followup-To' line containing a smaller number of groups.",
/* < ERROR_STRIPFIELD_NL > */
    "ouch!  can't find newline in stripField\n",
/* < FOLLOWUP_REPLY_TO_TITLE > */
    "Followup and reply to article %ld in `%s'", /* article number, newsgroup */
/* < FOLLOWUP_TO_TITLE > */
    "Followup to article %ld in `%s'", /* article number, newsgroup */
/* < REPLY_TO_TITLE > */
    "Reply to article %ld in `%s'", /* article number, newsgroup */
/* < FORWARD_TO_TITLE > */
    "Forward article %ld in `%s' via mail", /* article number, newsgroup */
/* < POST_ARTICLE > */
    "Post article",
/* < POST_ARTICLE_TO > */
    "Post article to `%s'", /* newsgroup */
/* < POST_MAIL_ARTICLE > */
    "Post and mail article",
/* < POST_MAIL_ARTICLE_TO > */
    "Post article to `%s' and mail it", /* newsgroup */
/* < MAIL > */
    "Send a mail message",
/* < USER_CANT_CANCEL > */
    "Not entitled to cancel the article",
/*
 ### may be the following messages shouldn't translate ###
 */
/* < REPLY_YOU_WRITE > */
    "In article %s,\n you write:\n", /* messageid */
/* < FORWARDED_ARTIKEL > */
    "\n------ Forwarded Article %s\n------ From %s\n\n", /* messageid , author */
/* < FORWARDED_ARTICLE_END > */
    "\n------ End of Forwarded Article\n",
/* < FOLLOWUP_AUTHOR_WRITE > */
    "In article %s,\n %s writes:\n", /* messageid , author */
/* #### end may be not translate #### */
/* < NEWSGROUPS_INDEX > */
    "%6s %7s %*s %4d article%1.1s +%6d old",
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
    "Cannot read the .newsrc file",
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
/* < ASK_FILE_MODIFIED > */
    "%s file %s\nhas been modified; overwrite it?", /* file type, file name */
/* < PENDING_COMPOSITION > */
    "You cannot exit when a composition is pending!",
/* < NNTP_PASSWORD > */
    "Enter NNTP password:",
/* < UNKNOWN_SORT_TYPE > */
    "Unknown subject sort type: %s", /* type */
/* < TOO_MANY_SORT_TYPES > */
    "Too many subject sort types specified.",
/* < UNPARSEABLE_DATE > */
    "Unparseable date (article %ld in %s):\n\t%s", /* number, newsgroup, string */
/* < THREADING_FOR > */
    "Threading newsgroup `%s'...", /* newsgroup */
/* < FILE_CACHE_OPEN > */
    "Error opening cache file in %s:\n\t%s", /* directory, error string */
/* < MESG_PANE_DISMISS > */
    "This window can be left open or dismissed.\n\tIf dismissed, it will reappear whenever there are new messages.",
/* < BAD_FROM > */
    "Bad `From' address `%s'.\n\tPlease fix it and resend, or abort your message.",
/* < NO_BODY > */
    "Your message has no body, or the blank line after the header is missing.\n\tPlease fix this and resend, or abort your message.",
/* < ONLY_INCLUDED > */
    "Message appears to contain only\nincluded text.  Post anyway?",
/* < COURTESY_COPY > */
    "[This is a courtesy copy of a message which was also posted to the\n newsgroup(s) shown in the header.]",
};

#endif /* XRN_LANG_english */

#ifdef XRN_LANG_french

char *message_strings[] = {
/* < BAD_BUTTON_NAME > */
    "Erreur XRN : nom de bouton incorrect `%s'.", /* button name */
/* < NO_SUCH_NG_DELETED > */
    "Le groupe de news `%s' n'existe pas.\n\tIl peut avoir ÈtÈ supprimÈ.", /* newsgroup name */
/* < UNKNOWN_FUNC_RESPONSE > */
    "Internal XRN error: unknown response %d from %s in %s.", /* return value, called function, calling function */
/* < DISPLAYING_LAST_UNREAD > */
    "Aucun article non lu dans `%s'.\n\tLecture du dernier article disponible.", /* newsgroup name */
/* < PROBABLY_KILLED > */
    "Aucun article non lu dans `%s'.\n\tIls ont probablement ÈtÈ tuÈs.", /* newsgroup name */
/* < NO_ARTICLES > */
    "Aucun article dans `%s'.", /* newsgroup name */
/* < PROBABLY_EXPIRED > */
    "Aucun article dans `%s'.\n\tIls ont probablement expirÈ ou ÈtÈ annulÈs.", /* newsgroup name */
/* < NO_NG_SPECIFIED > */
    "Pas de nom de groupe de news spÈcifiÈ.",
/* < NO_SUCH_NG > */
    "Le groupe de news `%s' n'existe pas.", /* newsgroup name */
/* < NO_PREV_NG > */
    "Pas de groupe de news prÈcÈdent",
/* < NO_GROUPS_SELECTED > */
    "Aucun groupe de news sÈlectionnÈ.",
/* < NG_NOT_MOVED > */
    "La nouvelle position d'un groupe ne peut pas figurer\n\tdans un bloc de groupes sÈlectionnÈs.\n\tLes groupes n'ont pas ÈtÈ dÈplacÈs.",
/* < SKIPPING_TO_NEXT_NG > */
    "Lecture du groupe de news suivant.",
/* < BAD_ART_NUM > */
    "Article numÈro `%s' invalide.", /* article number string */
/* < NO_ART_NUM > */
    "NumÈro d'article non spÈcifiÈ.",
/* < ART_NOT_AVAIL > */
    "L'article numÈro %d n'est pas disponible.", /* article number */
/* < ARTS_NOT_AVAIL > */
    "Les articles numÈro %d-%d ne sont pas disponibles.", /* first, last article number */
/* < NO_PREV_REGEXP > */
    "Pas d'expression rÈguliËre prÈcÈdente.",
/* < NO_PREV_ART > */
    "Pas d'article prÈcÈdent.",
/* < MSG_ABORTED > */
    "%s annulÈ.", /* "article" or "message" */
/* < NO_FILE_NOT_SAVED > */
    "Impossible de determiner le nom du fichier de sauvegarde.\n\tArticle/message non sauvÈ.",
/* < NO_SUBJECT > */
    "Le champ Subject manque dans votre message!\n\t",
/* < EMPTY_SUBJECT > */
    "Le champ Subject de votre message est vide!\n\t",
/* < NO_NEWSGROUPS > */
    "Le champ Newsgroups manque dans votre message!\n\t",
/* < MULTI > */
    "Il y a plusieurs champs %s dans votre message!\n\tVeuillez n'en conserver qu'un\n\tet l'envoyer ‡ nouveau.", /* field name */
/* < DEFAULT_ADDED > */
    "Un valeur par dÈfaut a ÈtÈ ajoutÈe.\n\tVeuillez la modifier si nÈcessaire\n\tet envoyer votre message ‡ nouveau.",
/* < EMPTY_ADDED > */
    "Une valeur vide a ÈtÈ ajoutÈe.\n\t",
/* < FILL_IN_RESEND > */
    "Veuillez le remplir et envoyer votre message ‡ nouveau.",
/* < NO_POSTABLE_NG > */
    "Pas de groupe appropriÈ sur la ligne `Newsgroups'.\n\tVeuillez corriger et envoyer votre message ‡ nouveau ou annuler.",
/* < SAVING_DEAD > */
    "SauvÈ dans `%s'.", /* file name */
/* < COULDNT_POST > */
    "Impossible de poster l'article.",
/* < POST_NOTALLOWED > */
    "Cette machine n'a pas l'autorisation de poster.",
/* < COULDNT_SEND > */
    "Impossible d'envoyer le message par mail.",
/* < MAILED_TO_MODERATOR > */
    "La ligne `Newsgroups' comporte au moins un groupe modÈrÈ.\n\tL'article sera envoyÈ par mail au modÈrateur.",
/* < ARTICLE_POSTED > */
    "Article postÈ.",
/* < MAIL_MESSAGE_SENT > */
    "Message envoyÈ par mail.",
/* < CANT_INCLUDE_CMD > */
    "Impossible d'exÈcuter includeCommand (`popen' n'a pas fonctionnÈ).",
/* < CANT_OPEN_ART > */
    "Impossible d'ouvrir le fichier correspondant ‡ l'article `%s':\n\t%s.", /* file name, error string */
/* < CANT_OPEN_FILE > */
    "Impossible d'ouvrir le fichier `%s':\n\t%s.", /* error string */
/* < NO_FILE_SPECIFIED > */
    "Aucun fichier spÈcifiÈ.",
/* < CANT_OPEN_TEMP > */
    "Impossible d'ouvrir le fichier temporaire `%s':\n\t%s.", /* file name, error string */
/* < CANT_STAT_TEMP > */
    "Impossible de trouver le fichier temporaire `%s':\n\t%s.", /* file name, error string */
/* < NO_CHANGE > */
    "Pas de modification dans le fichier temporaire `%s'.", /* file name */
/* < ZERO_SIZE > */
    "Le fichier temporaire `%s'\n\test de taille nulle.", /* file name */
/* < NO_MSG_TEMPLATE > */
    "Erreur interne XRN : pas de gabarit de message pour appeler Call_Editor.",
/* < CANT_EDITOR_CMD > */
    "Impossible d'exÈcuter la commande `%s' de l'Èditeur:\n\t%s.", /* command, error string */
/* < ONE_COMPOSITION_ONLY > */
    "Une seule composition permise a la fois.",
/* < EXECUTING_SIGNATURE > */
    "Execution de la commande de signature `%s'.", /* command */
/* < CANT_EXECUTE_SIGNATURE > */
    "Impossible d'exÈcuter la commande de signature `%s'.\n\tRemplacÈe par son nom.", /* signature file name */
/* < READING_SIGNATURE > */
    "Lecture du fichier de signature `%s'.", /* signature file name */
/* < CANT_READ_SIGNATURE > */
    "Impossible de lire le fichier de signature `%s':\n\t%s.", /* signature file name, error string */
/* < SIGNATURE_TOO_BIG > */
    "Signature file `%s'\n\tis too large; ignoring it.", /* signature file name */
/* < CANCEL_ABORTED > */
    "Article non annulÈ.",
/* < CANCELLED_ART > */
    "L'article a ÈtÈ annulÈ.",
/* < CANCEL_TO_MODERATOR > */
    "Message being canceled appears\n\tin one or more moderated newsgroups.\n\tCancel request will be\n\tmailed to moderator by server.",
/* < UNKNOWN_REGEXP_ERROR > */
    "Erreur inconnue dans l'expression rÈguliËre `%s'.", /* regexp string */
/* < KNOWN_REGEXP_ERROR > */
    "Erreur dans l'expression rÈguliËre `%s':\n\t%s.", /* regexp, error string */
/* < ART_NUMBERING_PROBLEM > */
    "ProblËme de numÈrotation d'articles.\n\tTous les articles du groupe `%s'\n\tsont marquÈs non lus.", /* newsgroup name */
/* < CANT_OPEN_KILL > */
    "Cannot open kill file `%s':\n\t%s.", /* file name, error string */
/* < CANT_OPEN_INCLUDED_KILL > */
    "Cannot open kill file `%s'\n\t(included from `%s'):\n\t%s.", /* file name, parent file name, error string */
/* < MALFORMED_KILL_ENTRY > */
    "Error in KILL file entry `%s'\n\tin KILL file `%s':\n\t%s.", /* entry, file, reason for error */
/* < ERROR_INCLUDE_MISSING > */
    "No newsgroup or file name specified in include directive",
/* < ERROR_INCLUDE_NOT_SEPARATED > */
   " Include operand not separated",
/* < KILL_ERROR_UNKNOWN_OPTION > */
    "Error in KILL file entry `%s'\n\tin KILL file `%s':\n\tUnknown option `%c'.", /* entry, file, unknown option */
/* < UNKNOWN_KILL_REGEXP_ERROR > */
    "Unknown regular expression error in KILL file entry `%s' in KILL file `%s'.", /* entry */
/* < KNOWN_KILL_REGEXP_ERROR > */
    "Regular expression error in KILL file entry `%s'\n\tin KILL file `%s': %s.", /* entry, error string */
/* < KILL_TOO_LONG > */
    "Discarding too-long entry starting with `%s'\n\tin KILL file `%s'.", /* start of entry, file */
/* < NOT_IN_NEWSRC > */
    "Le groupe `%s' ne figure pas dans votre fichier .newsrc.", /* newsgroup name */
/* < BOGUS_NG_REMOVING > */
    "Le groupe `%s' n'existe pas.\n\tIl est supprimÈ de votre fichier .newsrc.", /* newsgroup name */
/* < MISSING_NG_LISTING > */
    "Newsgroup `%s' not found in cache.\n\tRetrieving newsgroup list to find it.", /* newsgroup name */
/* < MAYBE_LIST > */
    "Newsgroup `%s' not found in cache.\nRetrieve newsgroup list to find it?", /* newsgroup name */
/* < DUP_NEWSRC_ENTRY > */
    "Le groupe `%s' figure plusieurs fois dans votre fichier .newsrc.\n\tSeule la premi`ere occurence est utilisÈe.", /* newsgroup name */
/* < BAD_NEWSRC_LINE > */
    "Impossible de lire la ligne %d de votre fichier .newsrc.\n\tLa ligne est ignorÈe.", /* line number */
/* < CANT_OPEN_NEWSRC_COPYING > */
    "Impossible d'ouvrir le fichier .newsrc `%s'\n\tpour copie: %s.", /* file name, error string */
/* < CANT_EXPAND > */
    "Cannot expand file name `%s'.", /* file name */
/* < EMPTY_NEWSRC_SAVE_NAME > */
    "Le nom du fichier de sauvegarde .newsrc est vide.",
/* < CANT_OPEN_NEWSRC_SAVE > */
    "Impossible d'ouvrir le fichier de sauvegarde .newsrc `%s'\n\ten Ècriture: %s.", /* file name, error string */
/* < NEWSRC_SAVE_FILE_WRITE_ERR > */
    "Erreur lors de l'Ècriture dans le fichier de sauvegarde .newsrc `%s':\n\t%s.", /* file name, error string */
/* < CANT_READ_NEWSRC > */
    "Impossible de lire le fichier .newsrc `%s':\n\t%s.", /* file name, error string */
/* < CREATING_NEWSRC > */
    "CrÈation du fichier .newsrc `%s' pour vous.", /* file name */
/* < CANT_CREATE_NEWSRC > */
    "Impossible de crÈer le fichier .newsrc `%s':\n\t%s.", /* file name, error string */
/* < CANT_STAT_NEWSRC > */
    "Impossible de trouver le fichier .newsrc `%s':\n\t%s.", /* file name, error string */
/* < ZERO_LENGTH_NEWSRC > */
    "Le fichier .newsrc `%s' est vide.\n\tAnnulation.", /* file name */
/* < CANT_OPEN_NEWSRC > */
    "Impossible d'ouvrir le fichier .newsrc `%s'\n\ten lecture: %s.", /* file name, error string */
/* < CANT_PARSE_NEWSRC > */
    "Impossible d'analyser le fichier .newsrc `%s' --\n\terreur ‡ la ligne %d.", /* file name, error line */
/* < CANT_OPEN_NEWSRC_TEMP > */
    "Impossible d'ouvrir le fichier temporaire `%s'\n\ten Ècriture: %s.", /* file name, error string */
/* < CANT_OPEN_NEWSRC_WRITING > */
    "Impossible d'ouvrir le fichier .newsrc `%s'\n\ten Ècriture: %s.", /* file name, error string */
/* < ERROR_UNLINKING_NEWSRC > */
    "Erreur lors de l'effacement du fichier .newsrc `%s':\n\t%s.", /* file name, error string */
/* < ERROR_RENAMING > */
    "Error renaming temporary file `%s'\n\tto file `%s':\n\t%s.", /* temporary file name, file name, error string */
/* < NO_MAIL_DIR > */
    "Pas de rÈpertoire de Mail `%s'.", /* directory name */
/* < NO_SUCH_MAIL_DIR > */
    "Pas de classeur `%s';\nle crÈer?", /* directory name */
/* < CANT_STAT_MAIL_DIR > */
    "Impossible de voir le rÈpertoire `%s':\n\t%s.", /* directory name, error string */
/* < MAIL_DIR_NOT_DIR > */
    "Impossible de crÈer le claseur --\n\tle chemin `%s' n'est pas un rÈpertoire.", /* directory name */
/* < FOLDER_NOT_DIR > */
    "Le chemin `%s' n'est pas un classeur.", /* folder name */
/* < NO_SUCH_RMAIL > */
    "Le fichier RMAIL `%s' n'existe pas.\nLe crÈer?", /* file name */
/* < CANT_OPEN_RMAIL > */
    "Impossible d'ouvrir le fichier RMAIL `%s'\n\ten Ècriture: %s.", /* file name, error string */
/* < CANT_WRITE_RMAIL > */
    "Impossible d'Ècrire dans le fichier RMAIL `%s':\n\t%s.", /* file name, error string */
/* < UNKNOWN_CONFIRM_BUTTON > */
    "Erreur XRN : bouton de confirmation inconnu `%s'.", /* button name */
/* < CANT_EXECUTE_CMD_POPEN > */
    "Impossible d'ÈxÈcuter la commande `%s' (`popen' a ÈchouÈ).", /* command string */
/* < CANT_EXPAND_DIR > */
    "Cannot expand directory `%s'.", /* directory name */
/* < CANT_CREATE_SAVE_DIR > */
    "Impossible de crÈer le rÈpertoire de sauvegarde `%s':\n\t%s.", /* drectory name, error string */
/* < CANT_FIGURE_FILE_NAME > */
    "Impossible de dÈterminer le nom de fichier `%s'.", /* file name */
/* < CANT_CREAT_APPEND_SAVE_FILE > */
    "Impossible de %s le fichier `%s':\n\t%s.", /* "create" or "append to", file name, error string */
/* < ERROR_WRITING_FILE > */
/* < ERROR_WRITING_SAVE_FILE > */
    "Erreur lors de l'Ècriture dans le fichier `%s':\n\t%s.", /* file name, error string */
/* < CONNECTING > */
    "Connexion au serveur NNTP `%s'...", /* server name */
/* < GETTING_LIST > */
    "Lecture de la liste des groupes de news...",
/* < GETTING_NEWGROUPS > */
    "Getting list of new newsgroups...",
/* < FAILED_CONNECT > */
    "La connexion au serveur NNTP `%s' a ÈchouÈ.", /* server name */
/* < LOST_CONNECT_ATTEMPT_RE > */
    "La connexion au serveur NNTP a ÈtÈ perdue.\n\tReconnexion en cours.",
/* < RECONNECTED > */
    "Reconnexion au serveur NNTP effectuÈe.",
/* < CANT_TEMP_NAME > */
    "Impossible de crÈer un fichier temporaire pour l'article",
/* < CANT_CREATE_TEMP > */
    "Impossible d'ouvrir le fichier temporaire `%s'\n\ten Ècriture : %s.", /* file name, error string */
/* < BOGUS_ACTIVE_ENTRY > */
    "EntrÈe erronÈe `%s' du fichier des groupes ignorÈe.", /* entry */
/* < BOGUS_ACTIVE_CACHE > */
    "Skipping bogus active cache entry `%s'.", /* entry */
/* < XHDR_ERROR > */
    "La requÍte XHDR auprËs du serveur NNTP a ÈchouÈ.\n\tLe serveur NNTP ne supporte pas XHDR\n\t(auquel cas XRN ne fonctionnera pas),\n\tou une erreur interne ‡ XRN s'est produite.",
/* < NNTP_ERROR > */
    "Erreur grave NNTP : `%s'.", /* error string */
/* < MALFORMED_XHDR_RESPONSE > */
    "Le serveur NNTP a retournÈ une rÈponse incorrecte.\n\tLa commande XHDR Ètait `%s',\n\tla rÈponse `%s'.", /* command, response */
/* < NO_APP_DEFAULTS > */
    "Le fichier de ressources par dÈfaut d'XRN n'est pas installÈ.\n\tEn consÈquence, certaines fonctionnalit'es d'XRN pourraient manquer.\n\tContacter la personne ayant rÈalisÈ l'installation\n\tpour rÈsoudre ce problËme.",
/* < VERSIONS > */
    "La version du fichier de ressource installÈ est `%s'.\n\tLa version de l'ÈxÈcutable XRN est `%s'.", /* app-defaults version, executable version */
/* < NO_DOMAIN > */
    "Impossible de dÈterminer le domaine de votre machine.\n\tRelancer XRN en positionnant la variable d'environnement DOMAIN\n\tou la ressource X domainName\nafin de poster ou d'envoyer un mail.",
/* < NO_SERVER > */
    "Impossible de trouver le serveur de news.\nRelanceer XRN en positionnant la variable d'environnement NNTPSERVER\nou la ressource X nntpServer\nou en utilisant l'option -nntpServer.",
/* < UNKNOWN_LIST_REGEXP_ERROR > */
    "Erreur d'expression rÈguliËre inconnue\n\tdans la liste %s;\n\tentrÈe `%s' ignorÈe.", /* list name, entry */
/* < KNOWN_LIST_REGEXP_ERROR > */
    "Regular expression error\n\tin %s list entry `%s':\n\t%s; entry ignored.", /* list name, entry, error string */
/* < OPEARATION_APPLY_CURSOR > */
    "Les opÈrations s'appliquent ‡ la sÈlection en cours ou ‡ la position du curseur",
/* < NO_MORE_UNREAD_ART > */
    "Plus d'article non lu dans les groupes auquels vous Ítes abonnÈ(e)",
/* < SEL_GROUPS_ADDSUB > */
    "SÈlectionner les groupes ‡ ajouter, `quit' refuse l'abonnement aux groupes restants",
/* < ARE_YOU_SURE > */
    " tes-vous s˚r(e)?",
/* SUB and UNSUB string must have the same len */
/* < SUBED > */ 
    "    abonnÈ",
/* < UNSUBED > */
    "non abonnÈ",
/* < IGNORED > */
    "ignored     ",
/* < OK_CATCHUP > */
    "Confirmation du rattrapage?",
/* < OK_CATCHUP_CUR > */ 
    "Confirmation du rattrapage jusqu'‡ la position courante?",
/* < OK_GETLIST > */
    "OK to fetch newsgroup list from the server?",
/* < OK_TO_UNSUB > */
    "Confirmation du dÈsabonnement?",
/* < OK > */
    "OK",
/* < EDIT > */
    "edit",
/* < SEARCH_ABORTED > */
    "Recherche abandonnÈe",
/* < ERROR_SUBJ_SEARCH > */ 
    "Recherche de sujet : %s", /* regular expression */
/* < ERROR_SUBJ_EXH > */ 
    "Le sujet a ÈtÈ ÈpuisÈ",
/* < ERROR_NO_PARENT > */
    "Article has no parent.",
/* < ERROR_PARENT_UNAVAIL > */
    "Article's parent is unavailable.",
/* < ERROR_SUBJ_ABORT > */
    "Recherche abandonnÈe",
/* < KILL_DONE > */
    "Returning to first unread article.",
/* < UNKNOWN_KILL_TYPE > */
    "Unknown kill type \"%s\" in \"%s\" kill request.", /* field type argument, field name */
/* < ERROR_CANT_UPDATE_NEWSRC > */
    "Impossible de mettre ‡ jour le fichier newsrc",
/* < ARTICLE_NUMBER > */ 
    "NumÈro d'article :",
/* < LIST_OLD_NUMBER > */ 
    "First article to list:",
/* < ERROR_SUBJ_EXPR > */
    "Recherche de l'expression %s : pas de sujet appropriÈ", /* regular expression */
/* < ERROR_SEARCH > */ 
    "Recherche de l'expression %s", /* regular expression */
/* < REGULAR_EXPR > */
    "Expression rÈguliËre :",
/* < BEHIND_WHAT_GROUP > */
    "AprËs quel groupe de news?",
/* < ARTICLE_QUEUED > */
    "Article insÈrÈ dans la file",
/* < GROUP_SUB_TO > */
    "Groupe auquel s'abonner :",
/* < GROUP_TO_GO > */ 
    "Groupe sur lequel aller :",
/* < VIEW_ALLNG_SUB > */
    "Visualiser tous les groupes disponibles, avec possibilitÈ d'abonnement",
/* < SUB_DONE > */
    "You are now subscribed to `%s'.", /* newsgroup name */
/* < AUTOMATIC_RESCAN > */
   "RafraÓchissement en cours...",
/* < RESCANNING_BACKGROUND > */
    "Rescanning in the background...",
/* < ERROR_UNSUP_TRANS > */
    "Transition impossible de %d ‡ %d", /* transition from, to */
/* < POST_FOLLOWUP > */
    "article",
/* < FOLLOWUP_REPLY > */ 
    "article et courrier Èlectronique",
/* < DEFAULT_MAIL > */
    "courrier Èlectronique",
/* < SAVE_IN > */
    "Sauvegarde dans %s",  /* file */
/* < ERROR_SEND_MAIL > */
    "Error lors de l'expÈdition du courrier :",
/* < ASK_FILE > */
    "Nom du fichier?",
/* < ASK_POST_ARTICLE > */
    "Poster l'article?",
/* < ASK_SEND > */
    "Send the message?",
/* < ASK_POST_SEND > */
    "Post and send the message?",
/* < RE_EDIT > */
    "Re-edit",
/* < AS_FOLLOWUP > */
    "as followup",
/* < AS_REPLY > */
    "as reply",
/* < AS_FOLLOWUP_REPLY > */
   "as followup/reply",
/* < ERROR_EXEC_FAILED > */
    "Erreur XRN : l'exÈcution de `%s' (execl) a ÈchouÈ\n", /* prog */
/* < ASK_POSTER_FANDR > */
    "La ligne `Followup-To' du message indique\nde rÈpondre directement ‡ l'auteur.\nVoulez-vous l'ignorer, et poster,\nou simplement envoyer un courrier?",
/* < ASK_POSTER_REPLY > */
    "La ligne `Followup-To' du message indique\nde rÈpondre directement ‡ l'auteur.\nVoulez-vous rÈpondre dans les news\nou envoyer une rÈponse par courrier?",
/* < POST_AND_SEND > */
    "poster et envoyer un courrier",
/* < SEND_MAIL > */
    "envoyer un courrier",
/* < POST > */
    "poster",
/* < FOLLOWUP_MULTIPLE_NGS > */
    "The default `Newsgroups' line of your followup contains multiple newsgroups.\n\tPlease be sure to remove inappropriate newsgroups before sending your message,\n\tand/or to put a more appropriate list of groups in your `Followup-To' line.",
/* < FOLLOWUP_FOLLOWUPTO > */
    "Note that the article to which you are responding contains a `Followup-To' line,\n\tso the default `Newsgroups' line of your followup has been set from that line\n\trather than from the `Newsgroups' line of the original article.",
/* < CROSSPOST_PROHIBIT > */
    "The `Newsgroups' line of your message contains %d newsgroups.\n\tThe maximum number of groups to which you are allowed to post a message is %d.\n\tPlease reduce the number of groups to which you are posting\n\tand then send your message again.",
/* < CROSSPOST_CONFIRM > */
    "The `Newsgroups' line of your message contains %d newsgroups.\n\tPlease consider reducing the number of groups to which you are posting.",
/* < FOLLOWUP_FOLLOWUPTO_CONFIRM > */
    "The `Newsgroups' line of your message contains %d newsgroups,\n\tand the `Followup-To' line contains %d newsgroups.\n\tPlease consider reducing the number of groups in your `Newsgroups' and/or `Followup-To' line.",
/* < FOLLOWUP_CONFIRM > */
    "The `Newsgroups' line of your message contains %d newsgroups.\n\tPlease consider either reducing the number of newsgroups to which you are posting\n\tor adding a `Followup-To' line containing a smaller number of groups.",
/* < ERROR_STRIPFIELD_NL > */
    "argh! je ne trouve pas de retour ‡ la ligne dans stripField\n",
/* < FOLLOWUP_REPLY_TO_TITLE > */
    "RÈpondre ‡ l'article %ld de `%s' dans les news et par courrier", /* article number, newsgroup */
/* < FOLLOWUP_TO_TITLE > */
    "RÈpondre ‡ l'article %ld de `%s' dans les news", /* article number, newsgroup */
/* < REPLY_TO_TITLE > */
    "RÈpondre ‡ l'article %ld de `%s' par courrier", /* article number, newsgroup */
/* < FORWARD_TO_TITLE > */
    "Faire suivre l'article %ld de `%s' ‡ un utilisateur", /* article number, newsgroup */
/* < POST_ARTICLE > */
    "Poster l'article",
/* < POST_ARTICLE_TO > */
    "Poster l'article dans `%s'", /* newsgroup */
/* < POST_MAIL_ARTICLE > */
    "Post and mail article",
/* < POST_MAIL_ARTICLE_TO > */
    "Post article to `%s' and mail it", /* newsgroup */
/* < MAIL > */
    "Send a mail message",
/* < USER_CANT_CANCEL > */
    "Vous n'avez pas le droit d'annuler l'article",
/*
 ### les messages ci-dessous ne devraient pas ÅÍtre traduits ###
 */
/* < REPLY_YOU_WRITE > */
    "In article %s,\n you write:\n", /* messageid */
/* < FORWARDED_ARTIKEL > */
    "\n------ Forwarded Article %s\n------ From %s\n\n", /* messageid , author */
/* < FORWARDED_ARTICLE_END > */
    "\n------ End of Forwarded Article\n",
/* < FOLLOWUP_AUTHOR_WRITE > */
    "In article %s,\n %s writes:\n", /* messageid , author */
/* #### fin des messages Å‡ ne pas traduire #### */
/* < NEWSGROUPS_INDEX > */
    "%9s %13s %*s %4d article%1.1s +%5d ancien%1.1s",
/* < UNREAD > */
    "(non lus)",
/* < NEWS_IN > */
    "Messages dans",
/* < NOT_ONE > */
    "s", /* see NEWSGROUPS_INDEX in STRING section */
/* < DONE > */
    "terminÈ",
/* < ABORTED > */
    "annulÈ",
/* < FAILED > */
    "echouÈ",
/* < CREATE > */
    "crÈer",
/* < APPEND > */
    "modifier",
/* < ERR_XRN_RUN > */
    "Un autre XRN vous appartenant existe sur %s (processus %d).\nS'il a disparu, supprimez le fichier \"%s\".\n", /* host, pid, lockfile */
/* < ERROR_CANT_READ_NEWSRC > */
    "Impossible de lire le fichier .newsrc",
/* < PROCESS_KILL_FOR > */
    "ExÈcution du fichier de sÈlection du groupe `%s'...", /* newsgroup */
/* < ERROR_REGEX_NOSLASH > */
    "pas de '/' pour terminer l'expression rÈguli`ere",
/* < ERROR_REGEX_NOSLASH_START > */
    "no slash preceding the regular expression",
/* < ERROR_REGEX_NOCOLON > */
    "pas de '.' aprËs l'expression rÈguliËre",
/* < ERROR_REGEX_UNKNOWN_COMMAND > */
    "commande inconnue (les commandes valides sont `j', `m', et `s')",
/* < KILL_LINE > */
    "Processing entry `%s' in KILL file `%s'.",
/* < KILL_KILLED > */
    "tuÈ - %s",         /* subject */
/* < KILL_UNREAD > */
    "marquÈ non lu - %s",  /* subject */
/* < KILL_SAVED > */
    "sauvÈ - %s",         /* subject */
/* < COUNT_KILLED > */
    "%d article%s tuÈs dans %s", /* count, "" or NOT_ONE_STRING , newsgroup */
/* < COUNT_UNREAD > */
    "%d article%s marquÈs non lus dans %s", /* count, "" or NOT_ONE_STRING , newsgroup */
/* < COUNT_SAVED > */
    "%d article%s sauvÈs dans %s", /* count, "" or NOT_ONE_STRING , newsgroup */
/* < ERROR_CORNERED > */
    "Erreur XRN dans `cornered' : nglist aurait d˚ Ítre valide\n",
/* < ERROR_OUT_OF_MEM > */
    "plus de mÈmoire disponible",
/* < PREFETCHING > */
    "Lecture de `%s'...",
/* < SERVER_POSTING_ERROR > */
    "Erreur dans le serveur NNTP : %s", /* error message */
/* < ASK_SAVEBOX > */
    "NomFichier, +NomClasseur, ou @NomClasseur?",
/* < SAVE_PIPE_TO > */
    "Envoi de l'article %ld vers la commande `%s'...",  /* articlenumber, command */
/* < ERROR_SAVE_PIPE > */
    "`%s' a terminÈ avec le status %d", /* command, status */
/* < SAVE_MH_REFILE > */
    "Archivage MH vers le dossier %s : %s", /* folder, status */
/* < SAVE_RMAIL_REFILE > */
    "Archivage RMAIL vers le dossier %s : %s", /* folder, status */
/* < SAVE_OK > */
    "Article %ld sauvÈ dans le fichier `%s'...", /* articlenumber, filename */
/* < SAVE_APPEND_OK > */
    "Article %ld ajoutÈ au fichier `%s'...", /* articlenumber, filename */
/* < SAVE_ARTICLE > */
    "Article: %ld de %s\n", /* articlenumber, newsgroup */
/* < ERROR_INFINITE_LOOP > */
    "Panique dans XRN : boucle infinie entre moveBeginning & moveEnd",
/* < ERROR_FINDARTICLE > */
    "NumÈro d'article valable non trouvÈ dans findArticle (cursor.c)",
/* < ERROR_STRIP_LEAVE_HEADERS > */
    "Use seule des ressource `stripHeaders' & `leaveHeaders' est autorisÈe",
/* < ERROR_REQUEST_FAILED > */
    "        La requÍte Ètait : `%s'\n        La rÈponse en dÈfaut Ètait : `%s'", /* command, message */
/* < ASK_FILE_MODIFIED > */
    "%s file %s\nhas been modified; overwrite it?", /* file type, file name */
/* < PENDING_COMPOSITION > */
    "You cannot exit when a composition is pending!",
/* < NNTP_PASSWORD > */
    "Enter NNTP password:",
/* < UNKNOWN_SORT_TYPE > */
    "Unknown subject sort type: %s", /* type */
/* < TOO_MANY_SORT_TYPES > */
    "Too many subject sort types specified.",
/* < UNPARSEABLE_DATE > */
    "Unparseable date (article %ld in %s):\n\t%s", /* number, newsgroup, string */
/* < THREADING_FOR > */
    "Threading newsgroup `%s'...", /* newsgroup */
/* < FILE_CACHE_OPEN > */
    "Error opening cache file in %s:\n\t%s", /* directory, error string */
/* < MESG_PANE_DISMISS > */
    "This window can be left open or dismissed.\n\tIf dismissed, it will reappear whenever there are new messages.",
/* < BAD_FROM > */
    "Bad `From' address `%s'.\n\tPlease fix it and resend, or abort your message.",
/* < NO_BODY > */
    "Your message has no body, or the blank line after the header is missing.\n\tPlease fix this and resend, or abort your message.",
/* < ONLY_INCLUDED > */
    "Message appears to contain only\nincluded text.  Post anyway?",
/* < COURTESY_COPY > */
    "[This is a courtesy copy of a message which was also posted to the\n newsgroup(s) shown in the header.]",
};

#endif /* XRN_LANG_french */

#ifdef XRN_LANG_german

/*
 * mesg_strings.c: global message string array
 *
 * --------------
 * section GERMAN
 * --------------
 * The German section was created and translated by K.Marquardt
 * (K.Marquardt@zhv.basf-ag.de).  Some revisions were provided by
 * T.Foks (foks@hub.de) and G.Niklasch (nikl@mathematik.tu-muenchen.de).
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
    "XRN Fehler: Falscher Knopf-Name `%s'.", /* button Name */
/* < NO_SUCH_NG_DELETED > */
    "Newsgruppe `%s' existiert nicht.\n\tM\366glicherweise wurde sie entfernt.", /* Newsgruppe Name */
/* < UNKNOWN_FUNC_RESPONSE > */
    "Interner XRN Fehler: unbekannte R\374ckmeldung %d von %s in %s.", /* return value, called function, calling function */
/* < DISPLAYING_LAST_UNREAD > */
    "Keine ungelesenen Artikel in `%s'.\n\tZeige letzten vorhandenen Artikel an.", /* Newsgruppe Name */
/* < PROBABLY_KILLED > */
    "Keine ungelesenen Artikel in `%s'.\n\tWahrscheinlich wurden sie ausgeblendet.", /* Newsgruppe Name */
/* < NO_ARTICLES > */
    "Keine Artikel in `%s'.", /* Newsgruppe Name */
/* < PROBABLY_EXPIRED > */
    "Keine Artikel in `%s'.\n\tSie sind wohl veraltet oder wurden zur\374ckgezogen.", /* Newsgruppe Name */
/* < NO_NG_SPECIFIED > */
    "Keine Newsgruppe angegeben.",
/* < NO_SUCH_NG > */
    "Newsgruppe `%s' existiert nicht.", /* Newsgruppe Name */
/* < NO_PREV_NG > */
    "Keine vorhergehende Newsgruppe.",
/* < NO_GROUPS_SELECTED > */
    "Keine Newsgruppen ausgew\344hlt.",
/* < NG_NOT_MOVED > */
    "Die neue Position kann nicht innerhalb\n\tder vorgemerkten Newsgruppen liegen.\n\tEs wurden keine Newsgruppen verschoben.",
/* < SKIPPING_TO_NEXT_NG > */
    "Zur n\344chsten Newsgruppe gesprungen.",
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
    "%s abgebrochen.", /* "Artikel" oder "E-Mail-Nachricht" */
/* < NO_FILE_NOT_SAVED > */
    "Kann den Dateinamen nicht ermitteln.\n\tArtikel/Nachricht nicht gespeichert.",
/* < NO_SUBJECT > */
    "Ihr Artikel enth\344lt keine `Subject'-Zeile!\n\t",
/* < EMPTY_SUBJECT > */
    "Die `Subject'-Zeile Ihres Artikels ist leer!\n\t",
/* < NO_NEWSGROUPS > */
    "Ihr Artikel enth\344lt keine `Newsgroups'-Zeile!\n\t",
/* < MULTI > */
    "Sie haben mehrere `%s'-Zeilen in Ihrem Artikel!\n\tBitte alle bis auf eine entfernen\n\tund erneut abschicken.", /* field name */
/* < DEFAULT_ADDED > */
    "Ein voreingestellter Wert wurde eingef\374gt.\n\tBitte passen Sie ihn entsprechend an,\n\tdann erneut abschicken.",
/* < EMPTY_ADDED > */
    "Es wurde ein leeres Feld eingef\374gt.\n\t",
/* < FILL_IN_RESEND > */
    "Bitte ausf\374llen und erneut abschicken.",
/* < NO_POSTABLE_NG > */
    "Keine der angegebenen Newsgruppen existiert und akzeptiert Artikel.\n\tBitte `Newsgroups'-Zeile ab\344ndern\n\tund erneut abschicken (oder sichern und abbrechen).",
/* < SAVING_DEAD > */
    "Abgespeichert in `%s'.", /* file Name */
/* < COULDNT_POST > */
    "Artikel konnte nicht eingespeist werden.",
/* < POST_NOTALLOWED > */
    "Von diesem Rechner aus d\374rfen\n\tkeine Artikel ver\366ffentlicht werden.",
/* < COULDNT_SEND > */
    "Konnte E-Mail-Nachricht nicht senden.",
/* < MAILED_TO_MODERATOR > */
    "Es wurden eine oder mehrere moderierte Newsgruppen angegeben.\n\tDer Artikel wird vom Server per E-Mail zum Moderator geschickt.",
/* < ARTICLE_POSTED > */
    "Artikel eingespeist.",
/* < MAIL_MESSAGE_SENT > */
    "E-Mail-Nachricht verschickt.",
/* < CANT_INCLUDE_CMD > */
    "Konnte includeCommand nicht ausf\374hren, `popen()' fehlgeschlagen.",
/* < CANT_OPEN_ART > */
    "Kann Artikeldatei `%s'\n\tnicht \366ffnen: %s.", /* file name, Fehler string */
/* < CANT_OPEN_FILE > */
    "Kann Datei `%s'\n\tnicht \366ffnen: %s.", /* Fehler string */
/* < NO_FILE_SPECIFIED > */
    "Keine Datei angegeben.",
/* < CANT_OPEN_TEMP > */
    "Kann tempor\344re Datei `%s'\n\tnicht \366ffnen: %s.", /* file name, Fehler string */
/* < CANT_STAT_TEMP > */
    "Kann den Status der tempor\344ren Datei `%s'\n\tnicht ermitteln: %s.", /* file name, Fehler string */
/* < NO_CHANGE > */
    "Keine Ver\344nderung der tempor\344ren Datei `%s'.", /* file Name */
/* < ZERO_SIZE > */
    "Die tempor\344re Datei `%s'\n\tist leer.", /* file Name */
/* < NO_MSG_TEMPLATE > */
    "Interner XRN Fehler: Kein Artikelschema beim Aufruf des Editors.",
/* < CANT_EDITOR_CMD > */
    "Kann Editorbefehl `%s'\n\t nicht ausf\374hren: %s.", /* command, Fehler string */
/* < ONE_COMPOSITION_ONLY > */
    "Es kann nur ein Artikel auf einmal geschrieben werden.",
/* < EXECUTING_SIGNATURE > */
    "Signaturbefehl `%s' wird ausgef\374hrt.", /* command */
/* < CANT_EXECUTE_SIGNATURE > */
    "Kann Signaturdatei `%s' nicht ausf\374hren\n\t.Lese sie stattdessen ein.", /* signature file Name */
/* < READING_SIGNATURE > */
    "Signaturdatei `%s' wird eingelesen.", /* signature file Name */
/* < CANT_READ_SIGNATURE > */
    "Kann Signaturdatei `%s'\n\tnicht lesen: %s.", /* signature file name, Fehler string */
/* < SIGNATURE_TOO_BIG > */
    "Signaturdatei `%s'\n\tist zu gro\337; wird ignoriert.", /* signature file name */
/* < CANCEL_ABORTED > */
    "Artikel nicht zur\374ckgezogen.",
/* < CANCELLED_ART > */
    "Artikel zur\374ckgezogen.",
/* < CANCEL_TO_MODERATOR > */
    "Der zur\374ckzuziehende Artikel erscheint\n\tin einer oder mehreren moderierten Newsgruppen.\n\tDie R\374ckzugswunsch wird durch den Server\n\tan die Moderatoren geschickt.",
/* < UNKNOWN_REGEXP_ERROR > */
    "Unbekannter Fehler im Suchmuster `%s'.", /* regexp string */
/* < KNOWN_REGEXP_ERROR > */
    "Fehler im Suchmuster `%s':\n\t%s.", /* regexp, Fehler string */
/* < ART_NUMBERING_PROBLEM > */
    "Probleme mit den Artikelnummern.\n\tAlle Artikel in `%s' als nicht gelesen markiert.", /* Newsgruppe Name */
/* < CANT_OPEN_KILL > */
    "Kann KILL-Datei `%s'\n\tnicht \366ffnen: %s.", /* file name, error string */
/* < CANT_OPEN_INCLUDED_KILL > */
    "Kann KILL-Datei `%s'\n\tnicht \366ffnen (include von `%s' aus):\n\t%s.", /* file name, parent file name, error string */
/* < MALFORMED_KILL_ENTRY > */
    "Fehler in Eintrag `%s'\n\tin KILL-Datei `%s':\n\t%s.", /* entry, file, reason for error */
/* < ERROR_INCLUDE_MISSING > */
    "Weder Newsgruppe noch Dateiname in include-Direktive angegeben",
/* < ERROR_INCLUDE_NOT_SEPARATED > */
    "Include-Operand nicht abgetrennt",
/* < KILL_ERROR_UNKNOWN_OPTION > */
    "Fehler in Eintrag `%s'\n\tin KILL-Datei `%s':\n\tUnbekannte Option `%c'.", /* entry, file, unknown option */
/* < UNKNOWN_KILL_REGEXP_ERROR > */
    "Unbekannter Fehler in Regul\344rem Ausdruck in Eintrag `%s'\n\tin KILL-Datei `%s'.", /* entry */
/* < KNOWN_KILL_REGEXP_ERROR > */
    "Fehler in Regul\344rem Ausdruck in Eintrag `%s' in KILL-Datei `%s': %s.", /* entry, error string */
/* < KILL_TOO_LONG > */
    "\334berlanger Eintrag beginnend mit `%s'\n\tin KILL-Datei `%s' wurde verworfen.", /* start of entry, file */
/* < NOT_IN_NEWSRC > */
    "Newsgruppe `%s' kommt nicht in der Datei .newsrc vor.", /* Newsgruppe Name */
/* < BOGUS_NG_REMOVING > */
    "Newsgruppe `%s' existiert nicht.\n\tSie wird aus der Datei .newsrc entfernt.", /* Newsgruppe Name */
/* < MISSING_NG_LISTING > */
    "Newsgruppe `%s' nicht im Cache gefunden.\n\tNewsgruppen-Liste wird geholt, um sie zu finden.", /* newsgroup name */
/* < MAYBE_LIST > */
    "Newsgruppe `%s' nicht im Cache gefunden.\nNewsgruppen-Liste holen, um sie zu finden?", /* newsgroup name */
/* < DUP_NEWSRC_ENTRY > */
    "Mehrfacher Eintrag in der Datei .newsrc f\374r `%s'.\n\tVerwendet wird der erste.", /* newsgroup name */
/* < BAD_NEWSRC_LINE > */
    "Kann Zeile %d in der Datei .newsrc nicht interpretieren.\n\t\334bergehe sie.", /* line number */
/* < CANT_OPEN_NEWSRC_COPYING > */
    "Kann .newsrc-Datei `%s'\n\tnicht kopieren: %s.", /* file name, Fehler string */
/* < CANT_EXPAND > */
    "Kann Dateinamen `%s'\n\tnicht expandieren.", /* file name */
/* < EMPTY_NEWSRC_SAVE_NAME > */
    "Dateiname zum Abspeichern der Datei .newsrc ist leer.",
/* < CANT_OPEN_NEWSRC_SAVE > */
    "Kann .newsrc-Datei `%s'\n\tnicht schreiben: %s.", /* file name, Fehler string */
/* < NEWSRC_SAVE_FILE_WRITE_ERR > */
    "Fehler beim Schreiben in .newsrc-Datei `%s':\n\t%s.", /* file name, Fehler string */
/* < CANT_READ_NEWSRC > */
    "Kann .newsrc-Datei `%s'\n\tnicht lesen: %s.", /* file Name, error string */
/* < CREATING_NEWSRC > */
    "Erzeuge .newsrc-Datei `%s'.", /* file Name */
/* < CANT_CREATE_NEWSRC > */
    "Kann .newsrc-Datei `%s'\n\tnicht erstellen: %s.", /* file name, Fehler string */
/* < CANT_STAT_NEWSRC > */
    "Kann Status der .newsrc-Datei `%s'\n\tnicht ermitteln: %s.", /* file name, Fehler string */
/* < ZERO_LENGTH_NEWSRC > */
    ".newsrc-Datei `%s'\n\tist leer, ich gebe auf.", /* file Name */
/* < CANT_OPEN_NEWSRC > */
    "Kann .newsrc-Datei `%s'\n\tnicht lesen: %s.", /* file name, Fehler string */
/* < CANT_PARSE_NEWSRC > */
    "Kann .newsrc-Datei `%s'\n\tnicht interpretieren -- Fehler in Zeile %d.", /* file name, error line */
/* < CANT_OPEN_NEWSRC_TEMP > */
    "Kann tempor\344re .newsrc-Datei `%s'\n\tnicht schreiben: %s.", /* file name, Fehler string */
/* < CANT_OPEN_NEWSRC_WRITING > */
    "Kann .newsrc-Datei `%s'\n\tnicht schreiben: %s.", /* file name, Fehler string */ 
/* < ERROR_UNLINKING_NEWSRC > */
    "Fehler beim Entfernen der .newsrc-Datei `%s':\n\t%s.", /* file name, Fehler string */
/* < ERROR_RENAMING > */
    "Fehler beim Umbenennen der Datei `%s'\n\tin `%s':\n\t%s.", /* temporary file name, file name, Fehler string */
/* < NO_MAIL_DIR > */
    "Kein E-Mail-Verzeichnis `%s'.", /* directory Name */
/* < NO_SUCH_MAIL_DIR > */
    "Kein E-Mail-Verzeichnis `%s';\nVerzeichnis anlegen?", /* directory name */
/* < CANT_STAT_MAIL_DIR > */
    "Kann Status des Verzeichnisses `%s'\n\tnicht ermitteln: %s.", /* directory name, error string */
/* < MAIL_DIR_NOT_DIR > */
    "Kann E-Mail-Verzeichnis nicht anlegen --\n\tPfad `%s' ist kein Verzeichnis.", /* directory Name */
/* < FOLDER_NOT_DIR > */
    "Pfad `%s' ist kein Ordner.", /* folder Name */
/* < NO_SUCH_RMAIL > */
    "RMAIL-Datei `%s' existiert nicht.\nDatei anlegen?", /* file name */
/* < CANT_OPEN_RMAIL > */
    "Kann RMAIL-Datei `%s'\n\tnicht \366ffnen: %s.", /* file name, Fehler string */
/* < CANT_WRITE_RMAIL > */
    "Kann nicht in die RMAIL-Datei `%s'\n\tschreiben: %s.", /* file name, Fehler string */
/* < UNKNOWN_CONFIRM_BUTTON > */
    "XRN Fehler: unbekannter Best\344tigungsknopf `%s'.", /* button Name */
/* < CANT_EXECUTE_CMD_POPEN > */
    "Konnte Befehl `%s'\n\tnicht ausf\374hren, `popen()' fehlgeschlagen.", /* command string */
/* < CANT_EXPAND_DIR > */
    "Kann Verzeichnisname `%s' nicht expandieren.", /* directory name */
/* < CANT_CREATE_SAVE_DIR > */
    "Kann Sicherungs-Verzeichnis `%s'\n\tnicht anlegen: %s.", /* drectory name, Fehler string */
/* < CANT_FIGURE_FILE_NAME > */
    "Kann mit Dateiname `%s'\n\tnichts anfangen.", /* file name */
/* < CANT_CREAT_APPEND_SAVE_FILE > */
    "%s Datei `%s'\n\tfehlgeschlagen: %s.", /* "create" or "append to", file name, Fehler string */
/* < ERROR_WRITING_FILE > */
/* < ERROR_WRITING_SAVE_FILE > */
    "Fehler beim Schreiben in Datei `%s':\n\t%s.", /* file name, Fehler string */
/* < CONNECTING > */
    "Verbindungsaufbau zum NNTP-Server `%s'...", /* server name */
/* < GETTING_LIST > */
    "Liste der Newsgruppen wird geholt...",
/* < GETTING_NEWGROUPS > */
    "Liste der neuen Newsgruppen wird geholt...",
/* < FAILED_CONNECT > */
    "Verbindungsaufbau zum NNTP-Server `%s' fehlgeschlagen.", /* server name */
/* < LOST_CONNECT_ATTEMPT_RE > */
    "Verbindung zum NNTP-Server unterbrochen.\n\tVersuche neuen Verbindungsaufbau.",
/* < RECONNECTED > */
    "Verbindung zum NNTP-Server wieder hergestellt.",
/* < CANT_TEMP_NAME > */
    "Kann keinen tempor\344ren Dateinamen f\374r Artikel erzeugen.",
/* < CANT_CREATE_TEMP > */
    "Kann tempor\344re Datei `%s'\n\tnicht schreiben: %s.", /* file name, Fehler string */
/* < BOGUS_ACTIVE_ENTRY > */
    "Fehlerhaften Eintrag `%s'\n\tder active-Datei \374bergangen.", /* entry */
/* < BOGUS_ACTIVE_CACHE > */
    "Fehlerhaften Eintrag `%s'\n\tim Cache \374bergangen.", /* entry */
/* < XHDR_ERROR > */
    "XHDR Befehl beim NNTP-Server fehlgeschlagen.\n\tEntweder unterst\374tzt der NNTP-Server den XHDR Befehl nicht\n\t(dann funktioniert XRN nicht),\n\toder es ist ein interner Fehler in XRN aufgetreten.",
/* < NNTP_ERROR > */
    "NNTP-Fehler: `%s'.", /* Fehler string */
/* < MALFORMED_XHDR_RESPONSE > */
    "NNTP-Server sendete nicht die erwartete XHDR-Antwort.\n\tXHDR-Befehl: `%s',\n\tAntwort `%s'.", /* command, response */
/* < NO_APP_DEFAULTS > */
    "Die XRN-Application-Defaults-Datei ist nicht vorhanden.\n\tEs ist m\366glich, dass einige XRN-Funktionen nicht verf\374gbar sind.\n\tFalls XRN von jemand anderem installiert wurde,\n\tteilen Sie ihm bitte diesen Fehler mit.\n\tWenn Sie XRN installiert haben, lesen Sie bitte die Datei\n\tCOMMON-PROBLEMS im XRN-Quellcode-Verzeichnis;\n\tdort steht, wie Sie das Problem beheben k\366nnen.\n\t",
/* < VERSIONS > */
    "Die Version der Application-Defaults-Datei ist `%s'.\n\tDie XRN-Version ist `%s'.", /* app-defaults version, executable version */
/* < NO_DOMAIN > */
    "Kann die Domain Ihres Rechners nicht ermitteln.\n\tSetzen Sie die Environmentvariable DOMAIN\n\toder die X-Ressource domainName\n\tzum Versenden von Artikeln und E-Mail,\n\tund starten Sie XRN erneut.",
/* < NO_SERVER > */
    "Kann den NNTP-Server nicht ermitteln.\nSetzen Sie die Environmentvariable NNTPSERVER\noder die X-Ressource nntpServer,\noder \374bergeben Sie beim Neustart von XRN\ndie Option -nntpServer.",
/* < UNKNOWN_LIST_REGEXP_ERROR > */
    "Unbekannter Suchmusterfehler in %s\n\tim Listeneintrag `%s';\n\tEintrag wird ignoriert.", /* list name, entry */
/* < KNOWN_LIST_REGEXP_ERROR > */
    "Suchmusterfehler in %s\n\tim Listeneintrag `%s': %s;\n\tEintrag wird ignoriert.", /* list name, entry, error string */
/* < OPEARATION_APPLY_CURSOR > */
    "Aktionen richten sich nach der aktuellen Auswahl und der Position des Mauszeigers",
/* < NO_MORE_UNREAD_ART > */
    "Keine weiteren ungelesenen Artikel in den abonnierten Newsgruppen.",
/* < SEL_GROUPS_ADDSUB > */
    "Zum Abonnieren Newsgruppen ausw\344hlen, verbleibende werden als `nicht abonniert' gekennzeichnet.",
/* < ARE_YOU_SURE > */
    "Sind Sie sicher?",
/* SUBED, UNSUBED and IGNORED strings must have the same len */
/* < SUBED > */ 
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
    "Newsgruppen-Liste vom Server holen?",
/* < OK_TO_UNSUB > */
    "Newsgruppe abbestellen?",
/* < OK > */
    "OK",
/* < EDIT > */
    "Editor",
/* < SEARCH_ABORTED > */
    "Suche wurde abgebrochen.",
/* < ERROR_SUBJ_SEARCH > */ 
    "Thema suchen: %s", /* regular expression */
/* < ERROR_SUBJ_EXH > */ 
    "Thema nicht (mehr) gefunden.",
/* < ERROR_NO_PARENT > */
    "Artikel hat keinen Vorg\344nger.",
/* < ERROR_PARENT_UNAVAIL > */
    "Vorg\344nger des Artikels nicht verf\374gbar.",
/* < ERROR_SUBJ_ABORT > */
    "Suche abgebrochen.",
/* < KILL_DONE > */
    "Zur\374ck zum ersten ungelesenen Artikel.",
/* < UNKNOWN_KILL_TYPE > */
    "Unbekannter Typ \"%s\" in \"%s\"-KILL-Befehl.", /* field type argument, field name */
/* < ERROR_CANT_UPDATE_NEWSRC > */
    "Kann die Datei .newsrc nicht aktualisieren.",
/* < ARTICLE_NUMBER > */ 
    "Artikel Nummer:",
/* < LIST_OLD_NUMBER > */ 
    "Erster anzuzeigender Artikel:",
/* < ERROR_SUBJ_EXPR > */
    "Suche nach %s: Keine Eintr\344ge gefunden.", /* regular expression */
/* < ERROR_SEARCH > */ 
    "Suche nach %s", /* regular expression */
/* < REGULAR_EXPR > */
    "Suchmuster:",
/* < BEHIND_WHAT_GROUP > */
    "Nach welcher Newsgrupppe?",
/* < ARTICLE_QUEUED > */
    "Artikel wurde zum Versand \374bergeben.",
/* < GROUP_SUB_TO > */
    "Zu abonnierende Gruppe:",
/* < GROUP_TO_GO > */ 
    "Gehe zur Gruppe:",
/* < VIEW_ALLNG_SUB > */
    "Anzeige aller Newsgruppen, Abonnieren m\366glich.",
/* < SUB_DONE > */
    "`%s' ist nun abonniert.", /* newsgroup name */
/* < AUTOMATIC_RESCAN > */
    "Automatische Abfrage des Servers wird ausgef\374hrt...",
/* < RESCANNING_BACKGROUND > */
    "Abfrage des Servers im Hintergrund...",
/* < ERROR_UNSUP_TRANS > */
    "Nicht unterst\344tzter \334bergang: %d nach %d", /* transition from, to */
/* < POST_FOLLOWUP > */
    "Artikel",
/* < FOLLOWUP_REPLY > */ 
    "Artikel und E-Mail-Nachricht",
/* < DEFAULT_MAIL > */
    "E-Mail-Nachricht",
/* < SAVE_IN > */
    "Sichere in %s",  /* file */
/* < ERROR_SEND_MAIL > */
    "Fehler beim Versenden einer E-Mail-Nachricht:",
/* < ASK_FILE > */
    "Dateiname?",
/* < ASK_POST_ARTICLE > */
    "Artikel einspeisen?",
/* < ASK_SEND > */
    "Send the message?",
/* < ASK_POST_SEND > */
    "Post and send the message?",
/* < RE_EDIT > */
    "Re-edit",
/* < AS_FOLLOWUP > */
    "as followup",
/* < AS_REPLY > */
    "as reply",
/* < AS_FOLLOWUP_REPLY > */
   "as followup/reply",
/* < ERROR_EXEC_FAILED > */
    "XRN Fehler: execl von `%s' fehlgeschlagen\n", /* prog */
/* < ASK_POSTER_FANDR > */
    "`Followup-To'-Zeile der Nachricht lautet `an Verfasser'.\nEintrag ignorieren und Artikel auch ver\366ffentlichen,\noder nur Nachricht an Verfasser senden?",
/* < ASK_POSTER_REPLY > */
    "`Followup-To' Zeile der Nachricht lautet `an Verfasser'.\nArtikel ver\366ffentlichen oder Nachricht an Verfasser senden?",
/* < POST_AND_SEND > */
    "Artikel einspeisen und versenden",
/* < SEND_MAIL > */
    "Nachricht senden",
/* < POST > */ 
    "Artikel einspeisen",
/* < FOLLOWUP_MULTIPLE_NGS > */
    "Die `Newsgroups'-Zeile Ihres Folgeartikels enth\344lt mehrere Newsgruppen.\n\tBitte entfernen Sie unpassende Newsgruppen, bevor Sie ihn versenden,\n\tund/oder f\374gen Sie passendere Newsgruppen in die 'Followup-To'-Zeile ein.",
/* < FOLLOWUP_FOLLOWUPTO > */
    "Beachten Sie bitte, dass der Artikel, auf den Sie antworten, eine 'Followup-To'-Zeile enth\344lt.\n\tDeshalb wurde die `Newsgroups'-Zeile Ihres Folgeartikels auf den Inhalt dieser Zeile gesetzt,\n\tstatt auf die `Newsgroups'-Zeile des urspr\374nglichen Artikels.",
/* < CROSSPOST_PROHIBIT > */
    "Die `Newsgroups'-Zeile Ihres Artikels enth\344lt %d Newsgruppen.\n\tDie maximale Anzahl von Newsgruppen, in denen Sie gleichzeitig ver\366ffentlichen d\374rfen, betr\344gt %d.\n\tBitte reduzieren Sie die Anzahl der Newsgruppen\n\tund versenden Sie dann Ihren Artikel erneut.",
/* < CROSSPOST_CONFIRM > */
    "Die `Newsgroups'-Zeile Ihres Artikels enth\344lt %d Newsgruppen.\n\tBitte erw\344gen Sie, diese Anzahl zu reduzieren.",
/* < FOLLOWUP_FOLLOWUPTO_CONFIRM > */
    "Die `Newsgroups'-Zeile Ihres Artikels enth\344lt %d Newsgruppen\n\tund die `Followup-To'-Zeile %d Newsgruppen.\n\tBitte erw\344gen Sie, die eine und/oder die andere Anzahl zu reduzieren.",
/* < FOLLOWUP_CONFIRM > */
    "Die 'Newsgroups'-Zeile Ihres Artikels enth\344lt %d Newsgruppen.\n\tBitte erw\344gen Sie, die Anzahl der Newsgruppen, in denen Sie ihren Artikel ver\366ffentlichen m\366chten, zu reduzieren\n\toder eine `Followup-To'-Zeile einzuf\374gen, die eine geringere Anzahl von Newsgruppen enth\344lt.",
/* < ERROR_STRIPFIELD_NL > */
    "Kein Zeilenvorschub in stripField gefunden.\n",
/* < FOLLOWUP_REPLY_TO_TITLE > */
    "Folgeartikel und E-Mail-Antwort zu Artikel %ld in %s", /* article number, newsgroup */
/* < FOLLOWUP_TO_TITLE > */
    "Folgeartikel zu Artikel %ld in %s", /* article number, newsgroup */
/* < REPLY_TO_TITLE > */
    "E-Mail-Antwort zu Artikel %ld in %s", /* article number, newsgroup */
/* < FORWARD_TO_TITLE > */
    "Weiterreichen des Artikel %ld in `%s' per E-Mail", /* article number, newsgroup */
/* < POST_ARTICLE > */
    "Artikel schreiben und ver\366ffentlichen",
/* < POST_ARTICLE_TO > */
    "Artikel schreiben und in `%s' ver\366ffentlichen", /* newsgroup */
/* < POST_MAIL_ARTICLE > */
    "Artikel ver\366ffentlichen und versenden",
/* < POST_MAIL_ARTICLE_TO > */
    "Artikel in `%s' ver\366ffentlichen und versenden", /* newsgroup */
/* < MAIL > */
    "E-Mail-Nachricht versenden",
/* < USER_CANT_CANCEL > */
    "Sie sind nicht berechtigt, den Artikel zur\374ckzuziehen.",
/* 
 ### Die folgenden Texte sollten evtl. nicht uebersetzt werden ###
 */
/* < REPLY_YOU_WRITE > */
    "In article %s,\n you write:\n", /* messageid */
/* < FORWARDED_ARTIKEL > */
    "\n------ Forwarded Article %s\n------ From %s\n\n", /* messageid , author */
/* < FORWARDED_ARTICLE_END > */
    "\n------ End of Forwarded Article\n",
/* < FOLLOWUP_AUTHOR_WRITE > */
    "In article %s,\n %s writes:\n", /* messageid , author */
/* #### Ende des evtl. nicht uebersetzen #### */
/* < NEWSGROUPS_INDEX > */
    "%4s %10s %*s %4d Artikel%1.1s +%6d alt",
/* < UNREAD > */
    "Neue",
/* < NEWS_IN > */
    "Artikel in",
/* < NOT_ONE > */
    " ", /* see NEWSGROUPS_INDEX in STRING section */    
/* < DONE > */
    "erledigt",
/* < ABORTED > */
    "abgebrochen",
/* < FAILED > */
    "fehlgeschlagen",
/* < CREATE > */
    "Erzeugen der",
/* < APPEND > */
    "Anh\344ngen an die",
/* < ERR_XRN_RUN > */
    "XRN l\344uft bereits auf %s als Prozess %d.\nFalls es nicht mehr l\344uft, entfernen Sie die Datei `%s'.\n", /* host, pid, lockfile */
/* < ERROR_CANT_READ_NEWSRC > */
    "Kann Datei .newsrc nicht lesen",
/* < PROCESS_KILL_FOR > */
    "Bearbeite KILL-Datei f\374r Newsgruppe `%s'...", /* newsgroup */
/* < ERROR_REGEX_NOSLASH > */
    "Fehlender Schr\344gstrich `/' am Ende des Suchmusters",
/* < ERROR_REGEX_NOSLASH_START > */
    "Fehlender Schr\344gstrich `/' am Anfang des Suchmusters",
/* < ERROR_REGEX_NOCOLON > */
    "Kein Komma nach dem Suchmuster",
/* < ERROR_REGEX_UNKNOWN_COMMAND > */
    "Unbekannter Befehl im Suchmuster (erlaubt sind `j', `m' und `s')",
/* < KILL_LINE > */
    "Bearbeite Eintrag `%s' in KILL-Datei `%s'.",
/* < KILL_KILLED > */
    "ausgeblendet - %s",    /* subject */
/* < KILL_UNREAD > */
    "als ungelesen markiert - %s",  /* subject */
/* < KILL_SAVED > */
    "gesichert - %s",      /* subject */
/* < COUNT_KILLED > */
    "%d Artikel%s in %s ausgeblendet", /* count, "" or NOT_ONE_STRING , newsgroup */
/* < COUNT_UNREAD > */
    "%d Artikel%s in %s als ungelesen markiert", /* count, "" or NOT_ONE_STRING , newsgroup */
/* < COUNT_SAVED > */
    "%d Artikel%s in %s abgespeichert", /* count, "" or NOT_ONE_STRING , newsgroup */
/* < ERROR_CORNERED > */
    "XRN Fehler in `cornered': erwarte g\374ltige nglist\n",
/* < ERROR_OUT_OF_MEM > */
    "kein Speicher mehr verf\374gbar",
/* < PREFETCHING > */
    "`%s' wird geholt...",
/* < SERVER_POSTING_ERROR > */
    "Fehlermeldung des NNTP-Servers: %s", /* error message */
/* < ASK_SAVEBOX > */
    "Dateiname, +Dateiname oder @Dateiname?",
/* < SAVE_PIPE_TO > */
    "Artikel %ld wird an Befehl `%s' \374bergeben...",  /* articlenumber, command */
/* < ERROR_SAVE_PIPE > */
    "Befehl `%s' beendet mit R\374ckmeldung %d.", /* command, status */
/* < SAVE_MH_REFILE > */
    "MH zu Verzeichnis %s %s", /* folder, status */
/* < SAVE_RMAIL_REFILE > */
    "RMAIL zu Verzeichnis %s %s", /* folder, status */
/* < SAVE_OK > */
    "Artikel %ld in Datei `%s' speichern...", /* articlenumber, filename */
/* < SAVE_APPEND_OK > */
    "Artikel %ld an Datei `%s' anh\344ngen...", /* articlenumber, filename */
/* < SAVE_ARTICLE > */
    "Artikel: %ld aus %s\n", /* articlenumber, newsgroup */
/* < ERROR_INFINITE_LOOP > */
    "XRN Fehler: moveBeginning / moveEnd in Endlosschleife",
/* < ERROR_FINDARTICLE > */
    "G\374ltige Artikelnummer nicht gefunden in findArticle",
/* < ERROR_STRIP_LEAVE_HEADERS > */
    "Es darf nur eine der Ressourcen `stripHeaders' oder `leaveHeaders' angegeben werden.",
/* < ERROR_REQUEST_FAILED > */
    "        Anfrage war: `%s'\n        R\374ckmeldung war: `%s'", /* command, message */
/* < ASK_FILE_MODIFIED > */
    "%s-Datei %s\nwurde modifiziert; \374berschreiben?", /* file type, file name */
/* < PENDING_COMPOSITION > */
    "Verlassen nicht m\366glich, solange noch ein Artikel verfa\374t wird!",
/* < NNTP_PASSWORD > */
    "NNTP-Passwort eingeben:",
/* < UNKNOWN_SORT_TYPE > */
    "Unbekanntes Sortier-Kriterium: %s", /* type */
/* < TOO_MANY_SORT_TYPES > */
    "Zu viele Sortier-Kriterien angegeben.",
/* < UNPARSEABLE_DATE > */
    "Unlesbares Datum (Artikel %ld in %s):\n\t%s", /* Nummer, Gruppe, String */
/* < THREADING_FOR > */
    "Newsgruppe `%s' wird nach Serien sortiert...", /* newsgroup */
/* < FILE_CACHE_OPEN > */
    "Fehler beim \326ffnen der Cache-Datei in %s:\n\t%s", /* directory, error string */
/* < MESG_PANE_DISMISS > */
    "Dieses Fenster kann offengelassen oder geschlossen werden.\n\tIm letzteren Fall erscheint es erneut,\n\tsobald neue Meldungen ausgegeben werden.",
/* < BAD_FROM > */
    "Bad `From' address `%s'.\n\tPlease fix it and resend, or abort your message.",
/* < NO_BODY > */
    "Your message has no body, or the blank line after the header is missing.\n\tPlease fix this and resend, or abort your message.",
/* < ONLY_INCLUDED > */
    "Message appears to contain only\nincluded text.  Post anyway?",
/* < COURTESY_COPY > */
    "[This is a courtesy copy of a message which was also posted to the\n newsgroup(s) shown in the header.]",
};

#endif /* XRN_LANG_german */
