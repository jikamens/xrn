#if !defined(lint) && !defined(SABER)
static char XRNrcsid[] = "$Header: /d/src/cvsroot/xrn/mesg_strings.c,v 1.2 1994-10-17 14:47:12 jik Exp $";
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
 * mesg_strings.c: global message string array
 *
 */

/*
  Messages for use in mesgPane calls below.  Many of these messages
  are used multiple times, which is why they are all here are
  constants.  Also, putting them all here makes it easy to make sure
  they're all consistent.  Finally, putting them here saves space in
  the executable since there's only one copy of each string.
  */
char *message_strings[] = {
/* < BAD_BUTTON_NAME > */
    "XRN error: bad button name `%s'.", /* button name */
/* < NO_SUCH_NG_DELETED > */
    "Newsgroup `%s' does not exist.  It may have been deleted.", /* newsgroup name */
/* < UNKNOWN_ENTER_NG_RESPONSE > */
    "Internal XRN error: unknown response %d from enterNewsgroup in %s.", /* return value, function name */
/* < DISPLAYING_LAST_UNREAD > */
    "No unread articles in `%s'.  Displaying last available article.", /* newsgroup name */
/* < PROBABLY_KILLED > */
    "No unread articles in `%s'.  They were probably killed.", /* newsgroup name */
/* < NO_ARTICLES > */
    "No articles in `%s'.", /* newsgroup name */
/* < PROBABLY_EXPIRED > */
    "No articles in `%s'.  They were probably expired or cancelled.", /* newsgroup name */
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
/* < NO_PREV_REGEXP > */
    "No previous regular expression.",
/* < NO_PREV_ART > */
    "No previous article.",
/* < MSG_ABORTED > */
    "Aborted %s.", /* "article" or "message" */
/* < NO_FILE_NOT_SAVED > */
    "Cannot determine save file name.  Article/message not saved.",
/* < NO_SUBJECT > */
    "The Subject field is missing in your message!",
/* < EMPTY_SUBJECT > */
    "The Subject field in your message is empty!",
/* < NO_POSTABLE_NG > */
    "No postable newsgroups in `Newsgroups' line.  Please fix and resend or save and abort.",
/* < SAVING_DEAD > */
    "Saving in `%s'.", /* file name */
/* < COULDNT_POST > */
    "Could not post article.",
/* < POST_NOTALLOWED > */
    "Posting not allowed from this machine.",
/* < COULDNT_SEND > */
    "Could not send mail message.",
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
    "Cannot open %s kill file (`%s') for newsgroup `%s': %s.", /* "local" or "global", file name, newsgroup name, error string */
/* < MALFORMED_KILL_ENTRY > */
    "Error in KILL file entry `%s' in newsgroup `%s': %s.", /* entry, newsgroup name, reason for error */
/* < UNKNOWN_KILL_REGEXP_ERROR > */
    "Unknown regular expression error in KILL file entry `%s'.", /* entry */
/* < KNOWN_KILL_REGEXP_ERROR > */
    "Regular expression error in KILL file entry `%s': %s.", /* entry, error string */
/* < NOT_IN_NEWSRC > */
    "Newsgroup `%s' is not in your .newsrc file.", /* newsgroup name */
/* < UNKNOWN_FIND_UNRD_RESPONSE > */
    "Internal XRN error: unknown response %d from findUnreadArticle in %s.", /* return value, function name */
/* < BOGUS_NG_REMOVING > */
    "Newsgroup `%s' does not exist.  Removing it from your .newsrc file.", /* newsgroup name */
/* < DUP_NEWSRC_ENTRY > */
    "Duplicate .newsrc entry for newsgroup `%s'.  Using the first one.", /* newsgroup name */
/* < BAD_NEWSRC_LINE > */
    "Unable to parse line %d in .newsrc file.  Ignoring it.", /* line number */
/* < CANT_OPEN_NEWSRC_COPYING > */
    "Cannot open .newsrc file `%s' for copying: %s.", /* file name, error string */
/* < CANT_EXPAND_NEWSRC_SAVE > */
    "Cannot expand .newsrc save file name `%s'.", /* file name */
/* < EMPTY_NEWSRC_SAVE_NAME > */
    ".newsrc save file name is the empty string.",
/* < CANT_OPEN_NEWSRC_SAVE > */
    "Cannot open .newsrc save file `%s' for writing: %s.", /* file name, error string */
/* < NEWSRC_SAVE_FILE_WRITE_ERR > */
    "Error writing to .newsrc save file `%s': %s.", /* file name, error string */
/* < CANT_EXPAND_NEWSRC > */
    "Cannot expand .newsrc file name `%s'.", /* file name */
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
/* < ERROR_RENAMING_NEWSRC > */
    "Error renaming temporary .newsrc file `%s' to .newsrc file `%s': %s.", /* temporary file name, file name, error string */
/* < NO_MAIL_DIR > */
    "No Mail directory `%s'.", /* directory name */
/* < CANT_STAT_MAIL_DIR > */
    "Cannot stat directory `%s': %s.", /* directory name, error string */
/* < MAIL_DIR_NOT_DIR > */
    "Cannot create folder -- mail path `%s' is not a directory.", /* directory name */
/* < FOLDER_NOT_DIR > */
    "Path `%s' is not a folder.", /* folder name */
/* < CANT_OPEN_RMAIL > */
    "Cannot open RMAIL file `%s' for writing: %s.", /* file name, error string */
/* < CANT_WRITE_RMAIL > */
    "Cannot write to RMAIL file `%s': %s.", /* file name, error string */
/* < UNKNOWN_CONFIRM_BUTTON > */
    "XRN error: unknown confirm button `%s'.", /* button name */
/* < CANT_EXECUTE_CMD_POPEN > */
    "Cannot execute command `%s' (`popen' failed).", /* command string */
/* < CANT_EXPAND_SAVE_DIR > */
    "Cannot expand save directory `%s'.", /* directory name */
/* < CANT_CREATE_SAVE_DIR > */
    "Cannot create save directory `%s': %s.", /* drectory name, error string */
/* < CANT_FIGURE_FILE_NAME > */
    "Cannot figure out file name `%s'.", /* file name */
/* < CANT_CREAT_APPEND_SAVE_FILE > */
    "Cannot %s file `%s': %s.", /* "create" or "append to", file name, error string */
/* < ERROR_WRITING_FILE > */
/* < ERROR_WRITING_SAVE_FILE > */
    "Error writing to file `%s': %s.", /* file name, error string */
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
/* < FAILED_RECONNECT > */
    "Failed to reconnect to the NNTP server (%s).  Sleeping...", /* stage */
/* < XHDR_ERROR > */
    "XHDR command to the NNTP server failed.  Either the NNTP server does not\n\tsupport XHDR (in which case XRN will not work), or an internal\n\tXRN error occurred.",
/* < NNTP_ERROR > */
    "NNTP serious error: `%s'.", /* error string */
/* < MALFORMED_XHDR_RESPONSE > */
    "NNTP server sent malformed XHDR response.  XHDR command was `%s', response was `%s'.", /* command, response */
/* < NO_APP_DEFAULTS > */
    "Current XRN Application Defaults file is not installed.\n\tAs a result, some XRN functionality may be missing.  Normally\n\tthe XRN application defaults file should be installed as\n\t/usr/lib/X11/app-defaults/%s (or as\n\t$OPENWINHOME/lib/app-defaults/%s for OpenWindows)", /* program class name (twice) */
/* < VERSIONS > */
    "Application defaults version is `%s'.  XRN executable version is `%s'.", /* app-defaults version, executable version */
};
