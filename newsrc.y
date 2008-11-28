%{
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: newsrc.y,v 1.6 1994-11-23 01:48:23 jik Exp $";
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
 * newsrc.y - yacc parser for the newsrc file
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include "avl.h"
#include "mesg.h"
#include "news.h"
#include "newsrcfile.h"
#include "mesg_strings.h"
#include "internals.h"

extern int yylineno;

int append_newsrc_messages = 0;

%}
    
%union {
    int integer;
    char *string;
    char character;
    struct list *item;
}


%token <string>     NAME
%token <character>  SEPARATOR
%token <integer>    NUMBER
%token EOL
%token DASH
%token COMMA

%type <item> artlist
%type <item> articles

%start goal

%%
goal        : newsrc_file ;

newsrc_file : newsrc_line
            | newsrc_file newsrc_line ;

newsrc_line : NAME SEPARATOR artlist EOL
                {
		    struct newsgroup *newsgroup;
		    char *dummy;

		    if (!avl_lookup(NewsGroupTable, $1, &dummy)) {
			mesgPane(XRN_SERIOUS | append_newsrc_messages,
				 BOGUS_NG_REMOVING_MSG, $1);
			append_newsrc_messages = XRN_APPEND;
		    } else {
			newsgroup = (struct newsgroup *) dummy;
			if (IS_NOENTRY(newsgroup)) {
			    CLEAR_NOENTRY(newsgroup);
			    newsgroup->status |= ($2 == ':' ? NG_SUB : NG_UNSUB);
			    newsgroup->nglist = $3;
			    updateArticleArray(newsgroup);
			    newsgroup->newsrc = MaxGroupNumber;
			    Newsrc[MaxGroupNumber++] = newsgroup;
			} else {
			    mesgPane(XRN_SERIOUS | append_newsrc_messages,
				     DUP_NEWSRC_ENTRY_MSG, $1);
			    append_newsrc_messages = XRN_APPEND;
			}
		    }
		    XtFree($1);
	        }
            | NAME SEPARATOR EOL
                {
		    struct newsgroup *newsgroup;
		    char *dummy;

		    if (!avl_lookup(NewsGroupTable, $1, &dummy)) {
			mesgPane(XRN_SERIOUS | append_newsrc_messages,
				 BOGUS_NG_REMOVING_MSG, $1);
			append_newsrc_messages = XRN_APPEND;
		    } else {
			newsgroup = (struct newsgroup *) dummy;
			if (IS_NOENTRY(newsgroup)) {
			    CLEAR_NOENTRY(newsgroup);
			    newsgroup->status |= ($2 == ':' ? NG_SUB : NG_UNSUB);
			    newsgroup->nglist = NIL(struct list);
			    updateArticleArray(newsgroup);
			    newsgroup->newsrc = MaxGroupNumber;
			    Newsrc[MaxGroupNumber++] = newsgroup;
			} else {
			    mesgPane(XRN_SERIOUS | append_newsrc_messages,
				     DUP_NEWSRC_ENTRY_MSG, $1);
			    append_newsrc_messages = XRN_APPEND;
			}
		    }
		    XtFree($1);
	        }
	    | error EOL {
		mesgPane(XRN_SERIOUS | append_newsrc_messages,
			 BAD_NEWSRC_LINE_MSG,
			 yylineno - 1);	/* yylineno stepped at EOL */
		append_newsrc_messages = XRN_APPEND;
		yyerrok;
		yyclearin;
	    }
            ;

artlist      : articles
                {
		    $$ = $1;
		}
            | artlist COMMA articles
                {
		    struct list *temp;

		    $$ = $1;
		    for (temp = $$; temp != NIL(struct list); temp = temp->next) {
			if (temp->next == NIL(struct list)) {
			    temp->next = $3;
			    break;
			}
		    }
		}
            ;

articles  : NUMBER
                {
		    $$ = ALLOC(struct list);
		    $$->type = SINGLE;
		    $$->contents.single = (art_num) $1;
		    $$->next = NIL(struct list);
		}
            | NUMBER DASH NUMBER
                {
		    $$ = ALLOC(struct list);
		    $$->type = RANGE;
		    $$->contents.range.start = (art_num) $1;
		    $$->contents.range.end = (art_num) $3;
		    $$->next = NIL(struct list);
		}
            ;

   
%%
#include "lex.yy.c"

