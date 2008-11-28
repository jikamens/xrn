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
 *           english
 *           german
 *           french
 *
 *
 * The German section was created and translated by K.Marquardt
 * (K.Marquardt@zhv.basf-ag.de) based on a version from
 * Jansohn@zxt.basf-ag.de.  Some revisions were provided by by T.Foks
 * (foks@hub.de).
 *
 * The french section was created and translated by
 *            N. Courtel (courtel@cena.dgac.fr)  March 23, 1995
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
#ifndef XRN_LANG_french
#define XRN_LANG_english
#endif
#endif
#endif


#ifdef XRN_LANG_english

/* 
 * explanation strings add mode
 *
 * default version, international (english)
 */

#define ADDQUIT_EXSTR	"Quit add mode, unsubscribe all remaining groups"
#define ADDIGNORE_REST_EXSTR "Quit add mode, ignoring all remaining groups"
#define ADDFIRST_EXSTR	"Add the selected group(s) to the beginning of the .newsrc file"
#define ADDLAST_EXSTR	"Add the selected group(s) to the end of the .newsrc file"
#define ADDAFTER_EXSTR	"Add the selected group(s) after a particular group in the .newsrc file"
#define ADDUNSUB_EXSTR	"Add the selected group(s) as unsubscribed"
#define ADDIGNORE_EXSTR	"Ignore the selected group(s)"

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
#define NGGOTO_EXSTR	"Go to a newsgroup (and subscribe, if necessary)"
#define NGALLGROUPS_EXSTR	"View all available groups, with option to subscribe"
#define NGLISTOLD_EXSTR	"Show groups with no new articles (toggle)"
#define NGRESCAN_EXSTR	"Query the news server for new articles and groups"
#define NGGETLIST_EXSTR	"Get a complete list of groups from the news server"
#define NGPREVGROUP_EXSTR   "Return to the group just visited (and subscribe, if necessary)"
#define NGSELECT_EXSTR	"Mark current selection for subsequent move operations"
#define NGMOVE_EXSTR	"Move previously selected groups in front of the current selection"
#define NGEXIT_EXSTR	"Quit XRN, leaving the .newsrc file unchanged since the last rescan"
#define NGCHECKPOINT_EXSTR	"Update the .newsrc file"
#define NGGRIPE_EXSTR	"Mail a gripe to the XRN maintainers"
#define NGPOST_EXSTR	"Post an article to one or more newsgroups"
#define NGPOST_AND_MAIL_EXSTR	"Post an article to one or more newsgroups and mail it"
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
#define ALLIGNORE_EXSTR "Ignore the selected group(s)"
#define ALLGOTO_EXSTR	"Go to the current newsgroup"
#define ALLSELECT_EXSTR	"Mark current selection for subsequent move operations"
#define ALLMOVE_EXSTR	"Move previously selected groups to the current position"
#define ALLTOGGLE_EXSTR	"Change order of groups: alphabetical/.newsrc order"
#define ALLSCROLL_EXSTR	"Scroll the ALL groups screen forward"
#define ALLSCROLLBACK_EXSTR	"Scroll the ALL groups screen backward"
#define ALLSEARCH_EXSTR	"Search the group list"
#define ALLLIMIT_EXSTR	"Display a subset of the group list"
#define ALLPOST_EXSTR	"Post to the current newsgroup"
#define ALLPOST_AND_MAIL_EXSTR	"Post to the current newsgroup and mail the posting to someone"

/* 
 * explanation strings art mode
 *
 */

#define ARTQUIT_EXSTR		"Return to newsgroup mode"
#define ARTNEXTUNREAD_EXSTR	"Read the next unread article"
#define ARTNEXT_EXSTR		"Read the next article"
#define ARTCURRENT_EXSTR	"Read the article under the cursor"
#define ARTUP_EXSTR		"Move up one line in the subject list"
#define ARTDOWN_EXSTR		"Move down one line in the subject list"
#define ARTSCROLL_EXSTR		"Scroll current article forward"
#define ARTSCROLLBACK_EXSTR	"Scroll current article backward"
#define ARTSCROLLLINE_EXSTR	"Scroll current article one line forward"
#define ARTSCROLLBCKLN_EXSTR	"Scroll current article one line backward"
#define ARTSCROLLEND_EXSTR	"Scroll to end of current article"
#define ARTSCROLLBEG_EXSTR	"Scroll to beginning of current article"
#define ARTSCROLLINDEX_EXSTR	"Scroll index forward one page"
#define ARTSCROLLINDBCK_EXSTR	"Scroll index back one page"
#define ARTPREV_EXSTR		"Read the previous article"
#define ARTLAST_EXSTR		"Go back to the last article displayed"
#define ARTNEXTGROUP_EXSTR	"Go to the next unread newsgroup, skipping newsgroup mode"
#define ARTCATCHUP_EXSTR	"Mark all articles (up to the selected article, if any) in the group as read"
#define ARTFEEDUP_EXSTR		"Mark all articles in the current group as read and go to the next unread newsgroup "
#define ARTGOTOARTICLE_EXSTR	"Go to the specified article number in the current group"
#define ARTMARKREAD_EXSTR	"Mark selected article(s) as read"
#define ARTMARKUNREAD_EXSTR	"Mark selected article(s) as unread"
#define ARTSUB_EXSTR		"Subscribe to the current group"
#define ARTUNSUB_EXSTR		"Unsubscribe to the current group"
#define ARTSUBNEXT_EXSTR	"Search for the next article with the selected subject"
#define ARTSUBPREV_EXSTR	"Search for the previous article with the selected subject"
#define ARTPARENT_EXSTR		"Search for the parent of the current or selected article"
#define ARTKILLSUBJECT_EXSTR	"Mark all articles with this subject as read"
#define ARTKILLAUTHOR_EXSTR	"Mark all articles with this author as read"
#define ARTKILLTHREAD_EXSTR	"Mark all articles in this thread as read"
#define ARTKILLSUBTHREAD_EXSTR	"Mark all articles in this subthread as read"
#define ARTSUBSEARCH_EXSTR	"Search the subject lines for a regular expression"
#define ARTCONTINUE_EXSTR	"Continue the regular expression subject search"
#define ARTPOST_EXSTR		"Post an article to this newsgroup"
#define ARTPOST_MAIL_EXSTR	"Post an article to this newsgroup and mail it too"
#define MAIL_EXSTR		"Send a mail message"
#define ARTEXIT_EXSTR		"Return to newsgroup mode, marking all articles as unread"
#define ARTCHECKPOINT_EXSTR	"Update the .newsrc file"
#define ARTGRIPE_EXSTR		"Mail a gripe to the XRN maintainers"
#define ARTLISTOLD_EXSTR	"List all articles in the current group (may be slow)"
#define ARTRESORT_EXSTR		"Resort the article list"
#define ARTSAVE_EXSTR		"Save the current article in a file"
#define ARTREPLY_EXSTR		"Mail a reply to the author of the current article"
#define ARTFORWARD_EXSTR	"Forward an article to a user(s)"
#define ARTFOLLOWUP_EXSTR	"Post a followup to the current article"
#define ARTFOLLOWREPL_EXSTR	"Post and mail a single response to the current article"
#define ARTCANCEL_EXSTR		"Cancel the current article"
#define ARTROT13_EXSTR		"Decrypt an encrypted joke"
#define ARTXLATE_EXSTR		"Translate the current article"
#define ARTHEADER_EXSTR		"Display the complete/stripped header"
#define ARTPRINT_EXSTR		"Print the article"

#endif /* XRN_LANG_english */

#ifdef XRN_LANG_french

/* 
 * explanation strings add mode
 *
 * default version, international (english)
 */

#define ADDQUIT_EXSTR		"Quitter le mode insertion, pas d'abonnement aux groupes restants"
#define ADDIGNORE_REST_EXSTR	"Quit add mode, ignoring all remaining groups"
#define ADDFIRST_EXSTR		"InsÅÈrer les groupes sÅÈlectionnÅÈs au dÅÈbut du fichier .newsrc"
#define ADDLAST_EXSTR		"InsÅÈrer les groupes sÅÈlectionnÅÈs Å‡ la fin du fichier .newsrc"
#define ADDAFTER_EXSTR		"InsÅÈrer les groupes sÅÈlectionnÅÈs aprÅËs un groupe dans le fichier .newsrc"
#define ADDUNSUB_EXSTR		"InsÅÈrer les groupes sÅÈlectionnÅÈs sans s'y abonner"
#define ADDIGNORE_EXSTR		"Ignore the selected group(s)"

/* 
 * explanation strings ng mode
 */

#define NGQUIT_EXSTR		"Quitter XRN"
#define NGREAD_EXSTR		"Lire les articles du groupe actuel"
#define NGNEXT_EXSTR		"DÅÈplacer le curseur vers le groupe suivant"
#define NGPREV_EXSTR		"DÅÈplacer le curseur vers le groupe prÅÈcÅÈdent"
#define NGCATCHUP_EXSTR		"Marquer tous les articles du groupe actuel comme lus"
#define NGSUBSCRIBE_EXSTR	"S'abonner Å‡ un groupe"
#define NGUNSUB_EXSTR		"Se dÅÈsabonner du groupe actuel"
#define NGGOTO_EXSTR		"Aller dans un groupe (en s'y abonnant si nÅÈcessaire)"
#define NGALLGROUPS_EXSTR	"Voir tous les groupes disponibles, avec la possibilitÅÈ de s'y abonner"
#define NGLISTOLD_EXSTR		"Voir tous les groupes sans nouvel article (bascule)"
#define NGRESCAN_EXSTR		"Demander au serveur de news les nouveaux articles et groupes"
#define NGGETLIST_EXSTR		"Get a complete list of groups from the news server"
#define NGPREVGROUP_EXSTR	"Retourner au groupe prÅÈcÅÈdent (en s'y abonnant si nÅÈcessaire)"
#define NGSELECT_EXSTR		"Marquer la sÅÈlection courante pour un futur dÅÈplacement"
#define NGMOVE_EXSTR		"DÅÈplacer les groupes sÅÈlectionnÅÈs prÅÈcÅÈdemment en tÅÍte de la sÅÈlection actuelle"
#define NGEXIT_EXSTR		"Quitter XRN, sans mettre Å‡ jour le fichier .newsrc"
#define NGCHECKPOINT_EXSTR	"Mettre Å‡ jour le fichier .newsrc"
#define NGGRIPE_EXSTR		"Envoyer une rÅÈclamation au support local XRN"
#define NGPOST_EXSTR		"Poster un article dans un ou plusieurs groupes de news"
#define NGPOST_AND_MAIL_EXSTR	"Post an article to one or more newsgroups and mail it"
#define NGSCROLL_EXSTR		"Avancer dans la liste des groupes de news"
#define NGSCROLLBACK_EXSTR	"Reculer dans la liste des groupes de news"

/* 
 * explanation strings all mode
 */

#define ALLQUIT_EXSTR		"Retour au mode groupe, en sauvant les modifications"
#define ALLSUB_EXSTR		"S'abonner au groupe actuel, en conservant la position dans .newsrc"
#define ALLFIRST_EXSTR		"S'abonner aux groupes sÅÈlectionnÅÈs, en les insÅÈrant au dÅÈbut du fichier .newsrc"
#define ALLLAST_EXSTR		"S'abonner aux groupes sÅÈlectionnÅÈs, en les insÅÈrant Å‡ la fin du fichier .newsrc"
#define ALLAFTER_EXSTR		"S'abonner aux groupes sÅÈlectionnÅÈs, en les insÅÈrant aprÅËs un groupe du fichier .newsrc"
#define ALLUNSUB_EXSTR		"Se dÅÈsabonner des groupes sÅÈlectionnÅÈs"
#define ALLIGNORE_EXSTR 	"Ignore the selected group(s)"
#define ALLGOTO_EXSTR		"Aller dans le groupe actuel"
#define ALLSELECT_EXSTR		"Marquer la sÅÈlection actuelle pour un futur dÅÈplacement"
#define ALLMOVE_EXSTR		"DÅÈplacer les groupes prÅÈcedemment sÅÈlectionnÅÈs vers la position actuelle"
#define ALLTOGGLE_EXSTR		"Basculer l'ordre des groupes : ordre alphabÅÈtique/ordre de .newsrc"
#define ALLSCROLL_EXSTR		"Avancer dans l'ÅÈcran `tous les groupes'"
#define ALLSCROLLBACK_EXSTR	"Reculer dans l'ÅÈcran `tous les groupes'"
#define ALLSEARCH_EXSTR		"Search the group list"
#define ALLLIMIT_EXSTR		"Display a subset of the group list"
#define ALLPOST_EXSTR		"Poster dans le groupe de news actuel"
#define ALLPOST_AND_MAIL_EXSTR	"Post to the current newsgroup and mail the posting to someone"

/* 
 * explanation strings art mode
 *
 */

#define ARTQUIT_EXSTR		"Retourner au mode groupes"
#define ARTNEXTUNREAD_EXSTR	"Lire le prochain article non lu"
#define ARTNEXT_EXSTR		"Lire le prochain article"
#define ARTCURRENT_EXSTR	"Read the article under the cursor"
#define ARTUP_EXSTR		"Move up one line in the subject list"
#define ARTDOWN_EXSTR		"Move down one line in the subject list"
#define ARTSCROLL_EXSTR		"Avancer dans l'article actuel"
#define ARTSCROLLBACK_EXSTR	"Reculer dans l'article actuel"
#define ARTSCROLLLINE_EXSTR	"Avancer d'une ligne dans l'article actuel"
#define ARTSCROLLBCKLN_EXSTR	"Reculer d'une ligne dans l'article actuel"
#define ARTSCROLLEND_EXSTR	"Aller Å‡ la fin de l'article actuel"
#define ARTSCROLLBEG_EXSTR	"Aller au debut de l'article actuel"
#define ARTSCROLLINDEX_EXSTR	"Avancer l'index d'une page"
#define ARTSCROLLINDBCK_EXSTR	"Reculer l'index d'une page"
#define ARTPREV_EXSTR		"Lire l'article prÅÈcÅÈdent"
#define ARTLAST_EXSTR		"Retourner au dernier article visualisÅÈ"
#define ARTNEXTGROUP_EXSTR	"Aller au prochain groupe non lu, sans passer par le mode groupes"
#define ARTCATCHUP_EXSTR	"Marquer tous les articles (jusqu'Å‡ celui sÅÈlectionnÅÈ par le bouton du milieu) commu lus"
#define ARTFEEDUP_EXSTR		"Marquer tous les articles du groupe actuel comme lus et aller au prochain groupe"
#define ARTGOTOARTICLE_EXSTR	"Aller Å‡ l'article de numÅÈro choisi dans le groupe actuel"
#define ARTMARKREAD_EXSTR	"Marquer les articles sÅÈlectionnÅÈs comme lus"
#define ARTMARKUNREAD_EXSTR	"Marquer les articles sÅÈlectionnÅÈs comme non lus"
#define ARTSUB_EXSTR		"Subscribe to the current group"
#define ARTUNSUB_EXSTR		"Se dÅÈsabonner du groupe actuel"
#define ARTSUBNEXT_EXSTR	"Rechercher le prochain article sur le mÅÍme sujet"
#define ARTSUBPREV_EXSTR	"Rechercher les articles prÅÈcÅÈdents sur le sujet sÅÈlectionnÅÈ"
#define ARTPARENT_EXSTR		"Search for the parent of the current or selected article"
#define ARTKILLSUBJECT_EXSTR	"Mark all articles with this subject as read"
#define ARTKILLAUTHOR_EXSTR	"Marquer tous les articles de cet auteur comme lus, pour cette session seulement"
#define ARTKILLTHREAD_EXSTR	"Mark all articles in this thread as read"
#define ARTKILLSUBTHREAD_EXSTR	"Mark all articles in this subthread as read"
#define ARTSUBSEARCH_EXSTR	"Rechercher une expression rÅÈguliÅËre dans les sujets"
#define ARTCONTINUE_EXSTR	"Continuer la recherche d'expression rÅÈguliÅËre"
#define ARTPOST_EXSTR		"Poster un article dans ce groupe de news"
#define ARTPOST_MAIL_EXSTR	"Post an article to this newsgroup and mail it too"
#define MAIL_EXSTR		"Send a mail message"
#define ARTEXIT_EXSTR		"Revenir au mode groupes, en marquant tous les articles comme non lus"
#define ARTCHECKPOINT_EXSTR	"Mettre Å‡ jour le fichier .newsrc"
#define ARTGRIPE_EXSTR		"Envoyer une rÅÈclamation au support local d'XRN"
#define ARTLISTOLD_EXSTR	"Lister tous les articles du groupe actuel (peut ÅÍtre long)"
#define ARTRESORT_EXSTR		"Resort the article list"
#define ARTSAVE_EXSTR		"Sauver l'article actuel dans un fichier"
#define ARTREPLY_EXSTR		"Envoyer un rÅÈponse par mail Å‡ l'auteur de l'article actuel"
#define ARTFORWARD_EXSTR	"Faire suivre un article Å‡ un utilisateur"
#define ARTFOLLOWUP_EXSTR	"Poster une rÅÈponse dans les news Å‡ l'article actuel"
#define ARTFOLLOWREPL_EXSTR	"Envoyer une rÅÈponse Å‡ l'aricle actuel par mail et dans les news"
#define ARTCANCEL_EXSTR		"Annuler l'article actuel"
#define ARTROT13_EXSTR		"DÅÈcrypter un article cryptÅÈ par rot-13"
#define ARTXLATE_EXSTR		"Traduire l'article actuel"
#define ARTHEADER_EXSTR		"Visualiser l'en-tÅÍte complet/rÅÈduit"
#define ARTPRINT_EXSTR		"Imprimer l'article"

#endif /* XRN_LANG_french */

#ifdef XRN_LANG_german

/* 
 * explanation strings 
 *
 * --------------
 * section GERMAN
 * --------------
 * The German section was created and translated by K.Marquardt
 * (K.Marquardt@zhv.basf-ag.de) based on a version from
 * Jansohn@zxt.basf-ag.de.  Some revisions were provided by by T.Foks
 * (Thomas.Foks@hub.de) and G. Niklasch (nikl@mathematik.tu-muenchen.de).
 *
 * german version (iso8859-1), use LANGUAGE= german in Imakefile/Makefile
 *
 * values of the iso8859-1 characters:
 *
 * "a = \344, "o = \366, "u = \374
 * "A = \304, "O = \326, "U = \334
 * sz = \337
 */

#define ADDQUIT_EXSTR	"Auswahl neuer Gruppen beenden, restliche als `nicht abonniert' kennzeichnen."
#define ADDIGNORE_REST_EXSTR	"Auswahl neuer Gruppen beenden, die restlichen ignorieren."
#define ADDFIRST_EXSTR	"Gew\344hlte Newsgruppe(n) abonnieren und an den Anfang der Datei .newsrc setzen."
#define ADDLAST_EXSTR	"Gew\344hlte Newsgruppe(n) abonnieren und ans Ende der Datei .newsrc setzen."
#define ADDAFTER_EXSTR  "Gew\344hlte Newsgruppe(n) hinter eine andere in der Datei .newsrc setzen."
#define ADDUNSUB_EXSTR	"Gew\344hlte Newsgruppe(n) als `nicht abonniert' kennzeichnen."
#define ADDIGNORE_EXSTR	"Gew\344hlte Newsgruppe(n) ignorieren."

/* 
 * explanation strings ng mode
 */

#define NGQUIT_EXSTR		"Programm XRN beenden."
#define NGREAD_EXSTR		"Artikel der aktuellen Newsgruppe lesen."
#define NGNEXT_EXSTR		"N\344chste Newsgruppe ausw\344hlen."
#define NGPREV_EXSTR		"Vorhergehende Newsgruppe aus\344hlen."
#define NGCATCHUP_EXSTR		"Alle Artikel der aktuellen Newsgruppe als `gelesen' kennzeichnen."
#define NGSUBSCRIBE_EXSTR	"Newsgruppe als `abonniert' kennzeichnen."
#define NGUNSUB_EXSTR		"Aktuelle Newsgruppe als `nicht abonniert' kennzeichnen."
#define NGGOTO_EXSTR		"Zur gew\344hlten Newsgruppe gehen (und abonnieren, falls n\366tig)."
#define NGALLGROUPS_EXSTR	"Alle verf\374gbaren Newsgruppen anzeigen - auch die nicht abonnierten."
#define NGLISTOLD_EXSTR		"Abonnierte Newsgruppen auch anzeigen, wenn keine neuen Artikel vorliegen (ein/aus)."
#define NGRESCAN_EXSTR		"Den News-Server abfragen, ob neue Artikel oder Newsgruppen existieren."
#define NGGETLIST_EXSTR		"Eine komplette Liste der Newsgruppen vom News-Server holen."
#define NGPREVGROUP_EXSTR  	"Zur zuletzt gelesenen Newsgruppe zur\374ckkehren (und abonnieren, falls n\366tig)."
#define NGSELECT_EXSTR		"Aktuelle Auswahl f\374r eine nachfolgende Verschiebung vormerken."
#define NGMOVE_EXSTR		"Vorgemerkte Newsgruppe(n) vor die gew\344hlte Newsgruppe setzen."
#define NGEXIT_EXSTR		"Programm XRN verlassen, ohne die Datei .newsrc zu aktualisieren."
#define NGCHECKPOINT_EXSTR	"Die Datei .newsrc aktualisieren."
#define NGGRIPE_EXSTR		"Nachricht an die XRN-Systembetreuer senden."
#define NGPOST_EXSTR		"Artikel schreiben und in einer oder mehreren Newsgruppen ver\366ffentlichen."
#define NGPOST_AND_MAIL_EXSTR	"Artikel schreiben, in Newsgruppe(n) ver\366ffentlichen und als E-Mail versenden."
#define NGSCROLL_EXSTR		"Liste der Newsgruppen weiterbl\344ttern."
#define NGSCROLLBACK_EXSTR	"Liste der Newsgruppen zur\374ckbl\344ttern."

/* 
 * explanation strings all mode
 */

#define ALLQUIT_EXSTR		"Die Datei .newsrc aktualisieren; weiter zur \334bersicht der abonnierten Gruppen."
#define ALLSUB_EXSTR		"Aktuelle Newsgruppe als `abonniert' kennzeichnen, ohne die Reihenfolge zu \344ndern."
#define ALLFIRST_EXSTR		"Gew\344hlte Newsgruppe(n) als `abonniert' kennzeichnen und an den Anfang setzen."
#define ALLLAST_EXSTR		"Gew\344hlte Newsgruppe(n) als `abonniert' kennzeichnen und ans Ende setzen."
#define ALLAFTER_EXSTR		"Gew\344hlte Newsgruppe(n) als `abonniert' kennzeichnen und plazieren."
#define ALLUNSUB_EXSTR		"Gew\344hlte Newsgruppe(n) als `nicht abonniert' kennzeichnen."
#define ALLIGNORE_EXSTR		"Gew\344hlte Newsgruppe(n) ignorieren."
#define ALLGOTO_EXSTR		"Aktuelle Newsgruppe lesen."
#define ALLSELECT_EXSTR		"Aktuelle Auswahl f\374r eine nachfolgende Verschiebung vormerken."
#define ALLMOVE_EXSTR		"Vorgemerkte Newsgruppe(n) vor die gew\344hlte Newsgruppe setzen."
#define ALLTOGGLE_EXSTR		"Reihenfolge der Anzeige umschalten: alphabetisch/.newsrc ."
#define ALLSCROLL_EXSTR		"Liste der Newsgruppen weiterbl\344ttern."
#define ALLSCROLLBACK_EXSTR	"Liste der Newsgruppen zur\374ckbl\344ttern."
#define ALLSEARCH_EXSTR		"Suchen in der Gruppenliste."
#define ALLLIMIT_EXSTR		"Gruppenliste gem\344\337 Suchmuster einschr\344nken."
#define ALLPOST_EXSTR		"Artikel schreiben und in der aktuellen Newsgruppe ver\366ffentlichen."
#define ALLPOST_AND_MAIL_EXSTR	"Artikel schreiben, in der aktuellen Newsgruppe ver\366ffentlichen und als E-Mail senden."

/* 
 * explanation strings art mode
 *
 */

#define ARTQUIT_EXSTR		"Zur Gruppen\374bersicht zur\374ckkehren."
#define ARTNEXTUNREAD_EXSTR	"N\344chsten noch nicht gelesenen Artikel anzeigen."
#define ARTNEXT_EXSTR		"N\344chsten Artikel anzeigen."
#define ARTCURRENT_EXSTR	"Gew\344hlten Artikel anzeigen."
#define ARTUP_EXSTR		"Eine Zeile nach oben in der Themenliste."
#define ARTDOWN_EXSTR		"Eine Zeile nach unten in der Themenliste."
#define ARTSCROLL_EXSTR		"Aktuellen Artikel weiterbl\344ttern."
#define ARTSCROLLBACK_EXSTR	"Aktuellen Artikel zur\374ckbl\344ttern."
#define ARTSCROLLLINE_EXSTR	"Aktuellen Artikel eine Zeile weiterschieben."
#define ARTSCROLLBCKLN_EXSTR	"Aktuellen Artikel eine Zeile zur\374ckschieben."
#define ARTSCROLLEND_EXSTR	"Zum Ende des aktuellen Artikels gehen."
#define ARTSCROLLBEG_EXSTR	"Zum Anfang des aktuellen Artikels gehen."
#define ARTSCROLLINDEX_EXSTR	"Themenliste weiterbl\344ttern."
#define ARTSCROLLINDBCK_EXSTR	"Themenliste zur\374ckbl\344ttern."
#define ARTPREV_EXSTR		"Vorhergehenden Artikel anzeigen."
#define ARTLAST_EXSTR		"Den zuletzt gelesenen Artikel anzeigen."
#define ARTNEXTGROUP_EXSTR	"Zur n\344chsten ungelesenen Newsgruppe gehen; Gruppen\374bersicht \374berspringen."
#define ARTCATCHUP_EXSTR	"Alle Artikel (bis zum gew\344hlten) als `gelesen' markieren; dann Gruppen\374bersicht."
#define ARTFEEDUP_EXSTR		"Alle Artikel als `gelesen' markieren; dann zur n\344chsten ungelesenen Newsgruppe."
#define ARTGOTOARTICLE_EXSTR	"Einen bestimmten Artikel in der aktuellen Newsgruppe anzeigen."
#define ARTMARKREAD_EXSTR	"Gew\344hlte Artikel als `gelesen' kennzeichnen."
#define ARTMARKUNREAD_EXSTR	"Gew\344hlte Artikel als `nicht gelesen' kennzeichnen."
#define ARTSUB_EXSTR		"Aktuellen Gruppe abonnieren."
#define ARTUNSUB_EXSTR		"Aktuelle Gruppe als `nicht abonniert' kennzeichnen."
#define ARTSUBNEXT_EXSTR	"Den n\344chsten Artikel mit dem gleichen Thema suchen."
#define ARTSUBPREV_EXSTR	"Einen vorhergehenden Artikel mit dem gleichen Thema suchen."
#define ARTPARENT_EXSTR		"Den Vorg\344nger des aktuellen oder gew\344hlten Artikels suchen."
#define ARTKILLSUBJECT_EXSTR	"Markiere alle Artikel mit diesem Thema als gelesen."
#define ARTKILLAUTHOR_EXSTR	"Markiere alle Artikel von diesem Autor als gelesen."
#define ARTKILLTHREAD_EXSTR	"Markiere alle Artikel in dieser Serie als gelesen."
#define ARTKILLSUBTHREAD_EXSTR	"Markiere alle Artikel in dieser Teilserie als gelesen."
#define ARTSUBSEARCH_EXSTR	"Artikel zu einem bestimmten Thema suchen."
#define ARTCONTINUE_EXSTR	"Suchen nach Thema fortsetzen."
#define ARTPOST_EXSTR		"Artikel schreiben und in dieser Newsgruppe ver\366ffentlichen."
#define ARTPOST_MAIL_EXSTR	"Artikel schreiben, in dieser Newsgruppe ver\366ffentlichen und als E-Mail versenden."
#define MAIL_EXSTR		"E-Mail-Nachricht schreiben und versenden."
#define ARTEXIT_EXSTR		"Alle Artikel als `nicht gelesen' markieren; dann zur Gruppen\374bersicht."
#define ARTCHECKPOINT_EXSTR	"Die Datei .newsrc aktualisieren."
#define ARTGRIPE_EXSTR		"Nachricht an den XRN-Systembetreuer senden."
#define ARTLISTOLD_EXSTR	"Alle, auch alte, Artikel der aktuellen Newsgruppe zeigen (evtl. langsam)."
#define ARTRESORT_EXSTR		"Themenliste neu sortieren."
#define ARTSAVE_EXSTR		"Aktuellen Artikel in eine Datei speichern."
#define ARTREPLY_EXSTR		"Antwort per E-Mail an den Verfasser des aktuellen Artikels senden."
#define ARTFORWARD_EXSTR	"Aktuellen Artikel an weitere Benutzer senden."
#define ARTFOLLOWUP_EXSTR	"Folgeartikel zum aktuellen Artikel schreiben und ver\366ffentlichen."
#define ARTFOLLOWREPL_EXSTR	"Folgeartikel schreiben, ver\366ffentlichen, und dem Autor des aktuellen Artikels senden."
#define ARTCANCEL_EXSTR		"Aktuellen, eigenen Artikel zur\374ckziehen."
#define ARTROT13_EXSTR		"Aktuellen Artikel entschl\374sseln."
#define ARTXLATE_EXSTR		"Aktuellen Artikel an Zeichensatz anpassen."
#define ARTHEADER_EXSTR		"Den kompletten/gek\374rzten Artikelkopf zeigen (wechselweise)."
#define ARTPRINT_EXSTR		"Aktuellen Artikel auf dem Drucker ausgeben."

#endif /* XRN_LANG_german */

#endif /* BUTEXPL_H */
