/*
 * xrn - an X-based NNTP news reader
 *
 * Copyright (c) 1994-2023, Jonathan Kamens.
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

#ifndef _TEXT_H_
#define _TEXT_H_

#include <X11/Intrinsic.h>

#ifdef MOTIF
# define TEXT_PANE_CHILD(w)	XtParent(w)
#else
# define TEXT_PANE_CHILD(w)	w
#endif

typedef enum { TextSearchLeft, TextSearchRight } TextDirection;

extern Widget	TextCreate			_ARGUMENTS((String,
							    Boolean,
							    Widget));
extern void	TextDestroy			_ARGUMENTS((Widget));
extern void	TextClear			_ARGUMENTS((Widget));
extern void 	TextSetString			_ARGUMENTS((Widget, String));
extern String	TextGetString			_ARGUMENTS((Widget));
extern long	TextGetLength			_ARGUMENTS((Widget));
extern void	TextSetFile			_ARGUMENTS((Widget, String));
extern String 	TextGetFile			_ARGUMENTS((Widget));
extern void 	TextReplace			_ARGUMENTS((Widget, String,
							    int, long, long));
extern void	TextInvalidate			_ARGUMENTS((Widget, String,
							    long, long));
extern Boolean	TextGetCurrentLine		_ARGUMENTS((Widget, long *,
							    long *));
extern Boolean	TextGetSelectedLines		_ARGUMENTS((Widget, long *,
							    long *));
extern Boolean	TextGetSelectedOrCurrentLines	_ARGUMENTS((Widget, long *,
							    long *));
extern void	TextUnsetSelection		_ARGUMENTS((Widget));
extern long	TextGetTopPosition		_ARGUMENTS((Widget));
extern void	TextSetTopPosition 		_ARGUMENTS((Widget, long));
extern void	TextDisableRedisplay		_ARGUMENTS((Widget));
extern void	TextEnableRedisplay		_ARGUMENTS((Widget));
extern void	TextDisplay			_ARGUMENTS((Widget));
extern long	TextGetInsertionPoint		_ARGUMENTS((Widget));
extern void	TextSetInsertionPoint		_ARGUMENTS((Widget, long));
extern void	TextRemoveLine			_ARGUMENTS((Widget, long));
extern void	TextScrollPage			_ARGUMENTS((Widget, int));
extern void	TextScrollEntire		_ARGUMENTS((Widget, int));
extern void	TextScrollLine			_ARGUMENTS((Widget, int));
extern void	TextMoveLine			_ARGUMENTS((Widget, int));
extern void	TextSetLines			_ARGUMENTS((Widget, int));
extern int	TextGetLines			_ARGUMENTS((Widget));
extern int	TextGetColumns			_ARGUMENTS((Widget));
extern Boolean	TextLastPage			_ARGUMENTS((Widget));
extern Boolean	TextPastLastPage		_ARGUMENTS((Widget));
extern void	TextSetLineSelections		_ARGUMENTS((Widget));
extern void	TextSetAllSelections		_ARGUMENTS((Widget));
extern void	TextSelectAll			_ARGUMENTS((Widget));
extern long	TextSearch			_ARGUMENTS((Widget, long,
							    TextDirection,
							    String));
extern void	TextSearchInteractive		_ARGUMENTS((Widget, XEvent *,
							    long, TextDirection,
							    String));
extern void	TextEnableWordWrap		_ARGUMENTS((Widget));
extern void	TextDisableWordWrap		_ARGUMENTS((Widget));

#endif /* _TEXT_H_ */
