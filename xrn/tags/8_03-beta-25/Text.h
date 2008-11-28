#ifndef _TEXT_H_
#define _TEXT_H_

#include <X11/Intrinsic.h>

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
extern void	TextEnableWordWrap		_ARGUMENTS((Widget));
extern void	TextDisableWordWrap		_ARGUMENTS((Widget));

#endif /* _TEXT_H_ */
