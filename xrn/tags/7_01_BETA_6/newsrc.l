/* Bug in some(?) versions of lex makes /{sep} not work in    */
/* the expressions below, thus the silliness with unput() etc */

/* FLEX (gnu lex) has some detailed, Posix-related changes    */
/* classic lex, implied below */

%{
#if defined(FLEX_SCANNER) && !defined(FLEX_LEX_COMPAT)
/*
 * If you get an error on the line below when compiling, the problem
 * is probably that you are using "flex -l" instead of "flex".  You do
 * not need to give flex the "-l" argument in order to build this
 * file.  You can probably solve this problem by (a) deleting
 * lex.yy.c, (b) adding "LEX=flex" to the Imakefile, (c) rebuilding
 * the Makefile from the Imakefile (with "xmkmf" or whatever else you
 * used the first time), and (d) trying to compile again.
 */
int yylineno = 1;
#define YYLINE yylineno++
#undef yywrap
#else
#define YYLINE
#endif
%}

extern char *XtNewString();
letter            [a-zA-Z]
any               [^ \t\n]
digit             [0-9]
sep               [:!]
%%
                  yyin = Newsrcfp;
^[ \t]*"\n"       YYLINE;
"\n"              {YYLINE; return(EOL); };
"-"               return(DASH);
","               return(COMMA);
{sep}             {yylval.character = yytext[0]; return(SEPARATOR); };  
{digit}+          {yylval.integer = atoi((char *) yytext); return(NUMBER); };
{letter}{any}+{sep} |
^{any}+{sep}      {
		    char c = yytext[yyleng - 1];
                    yytext[yyleng-1] = '\0';
                    yylval.string = XtNewString((char *) yytext);
		    unput(c);
                    return(NAME);
                  };
[ \t]             ;
^"options ".*"\n" {
		    YYLINE;
                    optionsLine = XtNewString((char *) yytext);
                    optionsLine[utStrlen(optionsLine) - 1] = '\0';
                  };
%%
/*
 * $Id: newsrc.l,v 1.4 1994-11-23 01:48:17 jik Exp $
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

yywrap()
{
    return(1);
}

/*ARGSUSED*/
yyerror(s)
char *s;
{
    /* lint/kludge */
#ifdef lint
    (void) yyinput();
    (void) yyoutput(0);
    (void) yyunput(0);
#endif /* lint */
}

