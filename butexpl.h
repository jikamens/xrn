#ifndef BUTEXPL_H
#define BUTEXPL_H

/*
 * $Header: /afs/gza.com/software/xrn/src/RCS/butexpl.h
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
 * butexpl.h: define the button explanation strings
 */

/* ----------------------------------------------------
 * This is the  multilingual version of butexpl.h
 * ----------------------------------------------------
 * Currently supported languages are:
 *
 *           englishl
 *           german
 *
 *
 * The german section was created and translated by
 *            K.Marquardt (K.Marquardt@zhv.basf-ag.de)  Nov/23/94
 *            based on a version from Jansohn@zxt.basf-ag.de
 *
 * ...............................................................
 * Note: all items should appear in all sections, if you add a item,
 *       please add it in every section. Items can also appear in
 *       front of all language sections. 
 *
 *
 * The following items appear only in front of all sections,
 * please add them to all others:
 *
 *          <NONE>
 */

/* Default to English. */

#ifndef XRN_LANG_english
#ifndef XRN_LANG_german
#define XRN_LANG_english
#endif
#endif


#ifdef XRN_LANG_english

/* 
 * explanation strings add mode
 *
 * default version, international (english)
 */

#define ADDQUIT_EXSTR	"Quit add mode, unsubscribe all remaining groups"
#define ADDFIRST_EXSTR	"Add the selected group(s) to the beginning of the .newsrc file"
#define ADDLAST_EXSTR	"Add the selected group(s) to the end of the .newsrc file"
#define ADDAFTER_EXSTR	"Add the selected group(s) after a particular group in the .newsrc file"
#define ADDUNSUB_EXSTR	"Add the selected group(s) as unsubscribed"

/* 
 * explanation strings ng mode
 */

#define NGQUIT_EXSTR	"Quit XRN"
#define NGREAD_EXSTR	"Read the articles in the current group"
#define NGNEXT_EXSTR	"Move the cursor to the next group"
#define NGPREV_EXSTR	"Move the cursor to the previous group"
#define NGCATCHUP_EXSTR	"Mark all articles in the current group as read"
#define NGSUBSCRIBE_EXSTR	"Subscribe to a group"
#define NGUNSUB_EXSTR	"Unsubscribe to the current group"
#define NGGOTO_EXSTR	"Go to a newsgroup"
#define NGALLGROUPS_EXSTR	"View all available groups, with option to subscribe"
#define NGLISTOLD_EXSTR	"Show groups with no new articles (toggle)"
#define NGRESCAN_EXSTR	"Query the news server for new articles and groups"
#define NGPREVGROUP_EXSTR   "Return to the group just visited"
#define NGSELECT_EXSTR	"Mark current selection for subsequent move operations"
#define NGMOVE_EXSTR	"Move previously selected groups in front of the current selection"
#define NGEXIT_EXSTR	"Quit XRN, leaving the .newsrc file unchanged since the last rescan"
#define NGCHECKPOINT_EXSTR	"Update the .newsrc file"
#define NGGRIPE_EXSTR	"Mail a gripe to the XRN maintainers"
#define NGPOST_EXSTR	"Post an article to a newsgroup"
#define NGSCROLL_EXSTR	"Scroll the newsgroups list forward"
#define NGSCROLLBACK_EXSTR	"Scroll the newsgroups list backward"

/* 
 * explanation strings all mode
 */

#define ALLQUIT_EXSTR	"Return to newsgroup mode, saving changes"
#define ALLSUB_EXSTR	"Subscribe to current group, leaving .newsrc position unchanged"
#define ALLFIRST_EXSTR	"Subscribe to the selected group(s); put at the beginning of the .newsrc file"
#define ALLLAST_EXSTR	"Subscribe to the selected group(s); put at the end of the .newsrc file"
#define ALLAFTER_EXSTR	"Subscribe to the selected group(s); put after a specified group in the .newsrc file"
#define ALLUNSUB_EXSTR	"Unsubscribe to the selected group(s)"
#define ALLGOTO_EXSTR	"Go to the current newsgroup"
#define ALLSELECT_EXSTR	"Mark current selection for subsequent move operations"
#define ALLMOVE_EXSTR	"Move previously selected groups to the current position"
#define ALLTOGGLE_EXSTR	"Change order of groups: alphabetical/.newsrc order"
#define ALLSCROLL_EXSTR	"Scroll the ALL groups screen forward"
#define ALLSCROLLBACK_EXSTR	"Scroll the ALL groups screen backward"
#define ALLPOST_EXSTR	"Post to the current newsgroup"

/* 
 * explanation strings art mode
 *
 */

#define ARTQUIT_EXSTR		"Return to newsgroup mode"
#define ARTNEXTUNREAD_EXSTR	"Read the next unread article"
#define ARTNEXT_EXSTR		"Read the next article"
#define ARTSCROLL_EXSTR		"Scroll current article forward"
#define ARTSCROLLBACK_EXSTR	"Scroll current article backward"
#define ARTSCROLLLINE_EXSTR	"Scroll current article one line forward"
#define ARTSCROLLBACKLINE_EXSTR	"Scroll current article one line backward"
#define ARTSCROLLEND_EXSTR	"Scroll to end of current article"
#define ARTSCROLLBEGINNING_EXSTR	"Scroll to beginning of current article"
#define ARTSCROLLINDEX_EXSTR	"Scroll index forward one page"
#define ARTSCROLLINDEXBACK_EXSTR	"Scroll index back one page"
#define ARTPREV_EXSTR		"Read the previous article"
#define ARTLAST_EXSTR		"Go back to the last article displayed"
#define ARTNEXTGROUP_EXSTR	"Go to the next unread newsgroup, skipping newsgroup mode"
#define ARTCATCHUP_EXSTR	"Mark all articles (up to the middle-button selected article) in the group as read"
#define ARTFEEDUP_EXSTR		"Mark all articles in the current group as read and go to the next unread newsgroup "
#define ARTGOTOARTICLE_EXSTR	"Go to the specified article number in the current group"
#define ARTMARKREAD_EXSTR	"Mark selected article(s) as read"
#define ARTMARKUNREAD_EXSTR	"Mark selected article(s) as unread"
#define ARTUNSUB_EXSTR		"Unsubscribe to the current group"
#define ARTSUBNEXT_EXSTR	"Search for the next article with the selected subject"
#define ARTSUBPREV_EXSTR	"Search for the previous article with the selected subject"
#define ARTKILLSESSION_EXSTR	"Mark all articles with this subject as read, this session only"
#define ARTKILLLOCAL_EXSTR	"Mark all articles with this subject as read in this group for this and all future sessions"
#define ARTKILLGLOBAL_EXSTR	"Mark all articles with this subject as read for all groups for this and all future sessions"
#define ARTKILLAUTHOR_EXSTR	"Mark all articles with this author as read, this session only"
#define ARTSUBSEARCH_EXSTR	"Search the subject lines for a regular expression"
#define ARTCONTINUE_EXSTR	"Continue the regular expression subject search"
#define ARTPOST_EXSTR		"Post an article to this newsgroup"
#define ARTEXIT_EXSTR		"Return to newsgroup mode, marking all articles as unread"
#define ARTCHECKPOINT_EXSTR	"Update the .newsrc file"
#define ARTGRIPE_EXSTR		"Mail a gripe to the XRN maintainers"
#define ARTLISTOLD_EXSTR	"List all articles in the current group (may be slow)"
#define ARTSAVE_EXSTR		"Save the current article in a file"
#define ARTREPLY_EXSTR		"Mail a reply to the author of the current article"
#define ARTFORWARD_EXSTR	"Forward an article to a user(s)"
#define ARTFOLLOWUP_EXSTR	"Post a followup to the current article"
#define ARTFOLLOWUPANDREPLY_EXSTR	"Post and mail a single response to the current article"
#define ARTCANCEL_EXSTR		"Cancel the current article"
#define ARTROT13_EXSTR		"Decrypt an encrypted joke"
#define ARTXLATE_EXSTR		"Translate the current article"
#define ARTHEADER_EXSTR		"Display the complete/stripped header"
#define ARTPRINT_EXSTR		"Print the article"

#endif /* XRN_LANG_english */

#ifdef XRN_LANG_german

/* 
 * explanation strings 
 *
 * --------------
 * section GERMAN
 * --------------
 * The GERMAN section was created and translated by
 *            K.Marquardt (K.Marquardt@zhv.basf-ag.de)  Nov/23/94
 *            based on a version from Jansohn@zxt.basf-ag.de
 *
 * german version (iso8859-1), use #define LANG GERMAN in config.h
 *
 * values of the iso8859-1 characters:
 *
 * "a = \344, "o = \366, "u = \374
 * "A = \304, "O = \326, "U = \334
 * sz = \337
 */

#define ADDQUIT_EXSTR	"Gruppieren verlassen und verbleibende Gruppen als `nicht abboniert' kennzeichnen."
#define ADDFIRST_EXSTR	"Gew\344hlte Newsgruppe(n) an den Anfang der Datei .newsrc setzen."
#define ADDLAST_EXSTR	"Gew\344hlte Newsgruppe(n) an das Ende der Datei .newsrc setzen."
#define ADDAFTER_EXSTR  "Gew\344hlte Newsgruppe(n) hinter eine andere in Newsgruppe setzen."
#define ADDUNSUB_EXSTR	"Gew\344hlte Newsgruppe(n) als `nicht abboniert' kennzeichnen."

/* 
 * explanation strings ng mode
 */

#define NGQUIT_EXSTR		"Programm XRN beenden."
#define NGREAD_EXSTR		"Artikel der aktuellen Newsgruppe lesen."
#define NGNEXT_EXSTR		"n\344chste Newsgruppe ausw\344hlen."
#define NGPREV_EXSTR		"Vorhergehende Newsgruppe aus\344ehlen."
#define NGCATCHUP_EXSTR		"Alle Artikel der aktuellen Newsgruppe als `gelesen' kennzeichnen."
#define NGSUBSCRIBE_EXSTR	"Newsgruppe als `abboniert' kennzeichnen."
#define NGUNSUB_EXSTR		"Aktuelle Newsgruppe als `nicht abboniert' kennzeichnen."
#define NGGOTO_EXSTR		"Zur gew\344hlten Newsgruppe gehen"
#define NGALLGROUPS_EXSTR	"Alle verf\374gbaren Newsgruppen anzeigen - auch die nicht gekennzeichneten."
#define NGLISTOLD_EXSTR		"Alle gew\344hlten Newsgruppen anzeigen - auch wenn keine neuen Artikel (toggle)."
#define NGRESCAN_EXSTR		"Den News Server abfragen, ob neue Artikel oder Newsgruppen existieren."
#define NGPREVGROUP_EXSTR  	"Zur zuletzt gelesenen Newgruppe zur\374ckkehren."
#define NGSELECT_EXSTR		"Aktuelle Auswahl fuer eine nachfolgende Verschiebung merken."
#define NGMOVE_EXSTR		"Gemerkte Auswahl vor die gew\344hlte Newsgruppe setzen."
#define NGEXIT_EXSTR		"Programm XRN verlassen, ohne die Datei .newsrc zu aktualisieren."
#define NGCHECKPOINT_EXSTR	"Die Datei .newsrc aktualisieren."
#define NGGRIPE_EXSTR		"Eine Nachricht an die XRN-Systembetreuer senden."
#define NGPOST_EXSTR		"Einen Artikel zu einer Newsgruppe verfassen."
#define NGSCROLL_EXSTR		"Liste der Newsgruppen nach oben schieben."
#define NGSCROLLBACK_EXSTR	"Liste der Newsgruppen nach unten schieben."

/* 
 * explanation strings all mode
 */

#define ALLQUIT_EXSTR		"Die Datei .newsrc aktualisieren und zur Gruppenauswahl zur\374ckkehren."
#define ALLSUB_EXSTR		"Aktuelle Newsgruppe als `abboniert' kennzeichnen ohne die Reihenfolge zu \344ndern."
#define ALLFIRST_EXSTR		"Gew\344hlte Newsgruppe(n) als `abboniert' kennzeichnen und an den Anfang setzen."
#define ALLLAST_EXSTR		"Gew\344hlte Newsgruppe(n) als `abboniert' kennzeichnen und an das Ende Setzen."
#define ALLAFTER_EXSTR		"Gew\344hlte Newsgruppe(n) als `abboniert' kennzeichnen und platzieren."
#define ALLUNSUB_EXSTR		"Gew\344hlte Newsgruppe(n) als `nicht abboniert' kennzeichnen"
#define ALLGOTO_EXSTR		"Zur gew\344hlten Newsgruppe gehen"
#define ALLSELECT_EXSTR		"Aktuelle Auswahl f\374r eine nachfolgende Verschiebung merken."
#define ALLMOVE_EXSTR		"Gemerkte Auswahl vor die gew\344hlte Newsgruppe setzen."
#define ALLTOGGLE_EXSTR		"Reihenfolge der Anzeige \344ndern - alphabtisch/.newsrc ."
#define ALLSCROLL_EXSTR		"Liste der Newsgruppen nach oben schieben."
#define ALLSCROLLBACK_EXSTR	"Liste der Newsgruppen nach unten schieben."
#define ALLPOST_EXSTR		"Einen Artikel zu einer Newsgruppe verfassen."

/* 
 * explanation strings art mode
 *
 */

#define ARTQUIT_EXSTR		"Zur Gruppenauswahl zur\374ckkehren."
#define ARTNEXTUNREAD_EXSTR	"N\344echsten noch nicht gelesenen Artikel anzeigen."
#define ARTNEXT_EXSTR		"N\344echsten Artikel anzeigen."
#define ARTSCROLL_EXSTR		"Aktuellen Artikel nach oben schieben."
#define ARTSCROLLBACK_EXSTR	"Aktuellen Artikel nach unten schieben."
#define ARTSCROLLLINE_EXSTR	"Aktuellen Artikel eine Zeile nach oben schieben."
#define ARTSCROLLBACKLINE_EXSTR	"Aktuellen Artikel eine Zeile nach unten schieben."
#define ARTSCROLLEND_EXSTR	"Zum Ende des Aktuellen Artikel gehen."
#define ARTSCROLLBEGINNING_EXSTR	"Zum Anfang des Aktuellen Artikel gehen."
#define ARTSCROLLINDEX_EXSTR	"Liste der Artikel nach oben schieben."
#define ARTSCROLLINDEXBACK_EXSTR	"Liste der Artikel nach unten schieben."
#define ARTPREV_EXSTR		"Den vorhergehenden Artikel lesen."
#define ARTLAST_EXSTR		"Den zuletzt angezeigten Artikel lesen."
#define ARTNEXTGROUP_EXSTR	"Zur n\344chsten noch nicht gelesenen News-Gruppe gehen, dabei Gruppenauswahl \374bergehen."
#define ARTCATCHUP_EXSTR	"Alle Artikel (bis zum aktuellen) als `gelesen' kennzeichnen und Newsgruppe verlassen."
#define ARTFEEDUP_EXSTR		"Alle Artikel als `gelesen' kennzeichnen und noch nicht gelesenen Newsgruppe verlassen."
#define ARTGOTOARTICLE_EXSTR	"Einen bestimmten Artikel in der aktuellen Newsgruppe lesen."
#define ARTMARKREAD_EXSTR	"Gew\344hlte Artikel als `gelesen' kennzeichnen."
#define ARTMARKUNREAD_EXSTR	"Gew\344hlte Artikel als `nicht gelesen' kennzeichnen."
#define ARTUNSUB_EXSTR		"Aktuelle Newsgruppe als `nicht abboniert' kennzeichnen."
#define ARTSUBNEXT_EXSTR	"Den n\344chsten Artikel mit dem gleichen Thema suchen."
#define ARTSUBPREV_EXSTR	"Einen vorhergehenden Artikel mit dem gleichen Thema suchen."
#define ARTKILLSESSION_EXSTR	"Alle Artikel mit diesem Thema in dieser Sitzung als `gelesen' kennzeichnen."
#define ARTKILLLOCAL_EXSTR	"Alle Artikel mit diesem Thema, in dieser Gruppe, fuer alle Sitzungen als `gelesen' kennzeichnen."
#define ARTKILLGLOBAL_EXSTR	"Alle Artikel mit diesem Thema fuer alle Sitzungen als `gelesen' kennzeichnen."
#define ARTKILLAUTHOR_EXSTR	"Alle Artikel dieses Autors in dieser Sitzung als `gelesen' kennzeichnen."
#define ARTSUBSEARCH_EXSTR	"Artikel zu einem bestimmten Thema suchen."
#define ARTCONTINUE_EXSTR	"Suchen nach einem bestimmten Thema fortsetzen."
#define ARTPOST_EXSTR		"Einen Artikel zu dieser Newsgruppe verfassen."
#define ARTEXIT_EXSTR		"Zur Gruppenauswahl zur\374ckkehren und alle Artikel als `nicht gelesen' kennzeichnen."
#define ARTCHECKPOINT_EXSTR	"Die Datei .newsrc aktualisieren."
#define ARTGRIPE_EXSTR		"Eine Nachricht an die XRN-Systembetreuer senden."
#define ARTLISTOLD_EXSTR	"Alle Artikel der aktuellen Gruppe zeigen (evtl. langsam)."
#define ARTSAVE_EXSTR		"Aktuellen Artikel in eine Datei speichern."
#define ARTREPLY_EXSTR		"Eine Nachricht an den Verfasser des aktuellen Artikels senden."
#define ARTFORWARD_EXSTR	"Aktuellen Artikel an weitere Benutzer senden."
#define ARTFOLLOWUP_EXSTR	"Einen, zum aktuellen Artikel bezugnehmenden, Artikel verfassen."
#define ARTFOLLOWUPANDREPLY_EXSTR	"Einen bezugnehmenden Artikel verfassen und diesen auch an den Verfasser senden."
#define ARTCANCEL_EXSTR		"Aktuellen, eigenen Artikel zur\374ckziehen."
#define ARTROT13_EXSTR		"Aktuellen Artikel entschl\374sseln."
#define ARTXLATE_EXSTR		"Aktuellen Artikel an Zeichensatz anpassen."
#define ARTHEADER_EXSTR		"Den kompletten/gek\374rzten Artikelkopf ausgeben (toggle)."
#define ARTPRINT_EXSTR		"Aktuellen Artikel auf dem Drucker ausgeben."

#endif /* XRN_LANG_german */
#endif /* BUTEXPL_H */
