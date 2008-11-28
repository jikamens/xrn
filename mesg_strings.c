#if !defined(lint) && !defined(SABER)
static char XRNrcsid[] = "$Header: /d/src/cvsroot/xrn/mesg_strings.c,v 1.1 1994-10-10 06:15:55 jik Exp $";
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
    "XRN error: bad button name `%s'.", /* button name */
    "Newsgroup `%s' does not exist.  It may have been deleted.", /* newsgroup name */
    "Internal XRN error: unknown response %d from enterNewsgroup in %s.", /* return value, function name */
    "No unread articles in `%s'.  Displaying last available article.", /* newsgroup name */
    "No unread articles in `%s'.  They were probably killed.", /* newsgroup name */
    "No articles in `%s'.", /* newsgroup name */
    "No articles in `%s'.  They were probably expired or cancelled.", /* newsgroup name */
    "No newsgroup name specified.",
    "Newsgroup `%s' does not exist.", /* newsgroup name */
    "No previous newsgroup.",
    "No newsgroups were selected.",
    "New position for newsgroups can't be in block of selected newsgroups.  Newsgroups have not been moved.",
    "Skipping to next newsgroup.",
    "First article (number %d) not available; rescan and try again.", /* article number */
    "Invalid article number `%s'.", /* article number string */
    "No article number specified.",
    "Article number %d not available.", /* article number */
    "No previous regular expression.",
    "No previous article.",
    "Aborted %s.", /* "article" or "message" */
    "Cannot determine save file name.  Article/message not saved.",
    "The Subject field is missing in your message!",
    "The Subject field in your message is empty!",
    "No postable newsgroups in `Newsgroups' line.  Please fix and resend or save and abort.",
    "Saving in `%s'.", /* file name */
    "Could not post article.",
    "Posting not allowed from this machine.",
    "Could not send mail message.",
    "One or more moderated newsgroups in `Newsgroups' line of message.  Article will be mailed to moderator by server.",
    "Article posted.",
    "Mail message sent.",
    "Cannot execute includeCommand (`popen' failed).",
    "Cannot open article file `%s': %s.", /* file name, error string */
    "Cannot open file `%s': %s.", /* error string */
    "No file specified.",
    "Cannot open temporary file `%s': %s.", /* file name, error string */
    "Cannot stat temporary file `%s': %s.", /* file name, error string */
    "No change in temporary file `%s'.", /* file name */
    "Temporary file `%s' has zero size.", /* file name */
    "Internal XRN error: no message template in call to Call_Editor.",
    "Cannot execute editor command `%s': %s.", /* command, error string */
    "Only one composition allowed at a time.",
    "Executing signature command `%s'.", /* command */
    "Cannot execute signature file `%s'.  Reading instead.", /* signature file name */
    "Reading signature file `%s'.", /* signature file name */
    "Cannot read signature file `%s': %s.", /* signature file name, error string */
    "Article not cancelled.",
    "Article has been cancelled.",
    "Unknown error in regular expression `%s'.", /* regexp string */
    "Error in regular expression `%s': %s.", /* regexp, error string */
    "Article numbering problem.  Marking all articles in newsgroup `%s' unread.", /* newsgroup name */
    "Cannot open %s kill file (`%s') for newsgroup `%s': %s.", /* "local" or "global", file name, newsgroup name, error string */
    "Error in KILL file entry `%s' in newsgroup `%s': %s.", /* entry, newsgroup name, reason for error */
    "Unknown regular expression error in KILL file entry `%s'.", /* entry */
    "Regular expression error in KILL file entry `%s': %s.", /* entry, error string */
    "Newsgroup `%s' is not in your .newsrc file.", /* newsgroup name */
    "Internal XRN error: unknown response %d from findUnreadArticle in %s.", /* return value, function name */
    "Newsgroup `%s' does not exist.  Removing it from your .newsrc file.", /* newsgroup name */
    "Duplicate .newsrc entry for newsgroup `%s'.  Using the first one.", /* newsgroup name */
    "Unable to parse line %d in .newsrc file.  Ignoring it.", /* line number */
    "Cannot open .newsrc file `%s' for copying: %s.", /* file name, error string */
    "Cannot expand .newsrc save file name `%s'.", /* file name */
    ".newsrc save file name is the empty string.",
    "Cannot open .newsrc save file `%s' for writing: %s.", /* file name, error string */
    "Error writing to .newsrc save file `%s': %s.", /* file name, error string */
    "Cannot expand .newsrc file name `%s'.", /* file name */
    "Cannot read .newsrc file `%s': %s.", /* file name, error string */
    "Creating .newsrc file `%s' for you.", /* file name */
    "Cannot create .newsrc file `%s': %s.", /* file name, error string */
    "Cannot stat .newsrc file `%s': %s.", /* file name, error string */
    ".newsrc file `%s' is empty.  Aborting.", /* file name */
    "Cannot open .newsrc file `%s' for reading: %s.", /* file name, error string */
    "Cannot parse .newsrc file `%s' -- error on line %d.", /* file name, error line */
    "Cannot open .newsrc temporary file `%s' for writing: %s.", /* file name, error string */
    "Cannot open .newsrc file `%s' for writing: %s.", /* file name, error string */
    "Error unlinking .newsrc file `%s': %s.", /* file name, error string */
    "Error renaming temporary .newsrc file `%s' to .newsrc file `%s': %s.", /* temporary file name, file name, error string */
    "No Mail directory `%s'.", /* directory name */
    "Cannot stat directory `%s': %s.", /* directory name, error string */
    "Cannot create folder -- mail path `%s' is not a directory.", /* directory name */
    "Path `%s' is not a folder.", /* folder name */
    "Cannot open RMAIL file `%s' for writing: %s.", /* file name, error string */
    "Cannot write to RMAIL file `%s': %s.", /* file name, error string */
    "XRN error: unknown confirm button `%s'.", /* button name */
    "Cannot execute command `%s' (`popen' failed).", /* command string */
    "Cannot expand save directory `%s'.", /* directory name */
    "Cannot create save directory `%s': %s.", /* drectory name, error string */
    "Cannot figure out file name `%s'.", /* file name */
    "Cannot %s file `%s': %s.", /* "create" or "append to", file name, error string */
    "Error writing to file `%s': %s.", /* file name, error string */
    "Lost connection to the NNTP server.  Attempting to reconnect.",
    "Reconnected to the NNTP server.",
    "Cannot create temporary file name for article.",
    "Cannot open temporary file `%s' for writing: %s.", /* file name, error string */
    "Skipping bogus active file entry `%s'.", /* entry */
    "Failed to reconnect to the NNTP server (%s).  Sleeping...", /* stage */
    "XHDR command to the NNTP server failed.  Either the NNTP server does not\n\tsupport XHDR (in which case XRN will not work), or an internal\n\tXRN error occurred.",
    "NNTP serious error: `%s'.", /* error string */
    "NNTP server sent malformed XHDR response.  XHDR command was `%s', response was `%s'.", /* command, response */
    "Current XRN Application Defaults file is not installed.\n\tAs a result, some XRN functionality may be missing.  Normally\n\tthe XRN application defaults file should be installed as\n\t/usr/lib/X11/app-defaults/%s (or as\n\t$OPENWINHOME/lib/app-defaults/%s for OpenWindows)", /* program class name (twice) */
    "Application defaults version is `%s'.  XRN executable version is `%s'.", /* app-defaults version, executable version */
};
