#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/AsciiSrc.h>
#include <X11/Xaw/AsciiSink.h>

#include "config.h"
#include "utils.h"
#include "cursor.h"
#include "Text.h"

#ifndef XawFmt8Bit
#define XawFmt8Bit FMT8BIT
#endif


#define REDISPLAY_NUM 5

typedef struct {
    Widget w;
    int count;
    Boolean changed;
} redisplay_t;

static redisplay_t redisplay_array[REDISPLAY_NUM];

static redisplay_t *find_redisplay _ARGUMENTS((Widget));

static redisplay_t *find_redisplay(w)
    Widget w;
{
    int i;

    for (i = 0; i < REDISPLAY_NUM; i++)
	if (redisplay_array[i].w == w)
	    return redisplay_array + i;

    return 0;
}

static redisplay_t *new_redisplay _ARGUMENTS((Widget));

static redisplay_t *new_redisplay(w)
    Widget w;
{
    int i;

    for (i = 0; i < REDISPLAY_NUM; i++)
	if (! redisplay_array[i].w) {
	    redisplay_array[i].w = w;
	    redisplay_array[i].count = 0;
	    redisplay_array[i].changed = 0;
	    return redisplay_array + i;
	}

    return 0;
}

static void free_redisplay _ARGUMENTS((redisplay_t *));

static void free_redisplay(ptr)
    redisplay_t *ptr;
{
    ptr->w = 0;
}

static void set_changed _ARGUMENTS((Widget));

static void set_changed(w)
    Widget w;
{
    redisplay_t *ptr;

    if (! (ptr = find_redisplay(w)))
	return;

    if (! ptr->changed++)
	XawTextDisableRedisplay(w);
}


/*
  Create a new text widget and return it.  It's initially empty.
  */
Widget TextCreate(name, read_only, parent)
    String name;
    Boolean read_only;
    Widget parent;
{
    Widget w;
    Arg args[2];
    Cardinal num_args = 0;
 
    XtSetArg(args[0], XtNstring, ""); num_args++;
    if (! read_only) {
	XtSetArg(args[1], XtNeditType, XawtextEdit); num_args++;
    }

    w = XtCreateManagedWidget(name, asciiTextWidgetClass, parent,
			      args, num_args);

    return w;
}

/*
  Destroy a text widget.
  */
void TextDestroy(w)
    Widget w;
{
    redisplay_t *ptr;

    if ((ptr = find_redisplay(w)))
	free_redisplay(ptr);
    XtDestroyWidget(w);
}


/*
  Clear the text in a text widget.
  */
void TextClear(w)
    Widget w;
{
    TextSetString(w, "");
}


/*
  Set the text in a text widget.  The current top displayed position
  and insertion point are not preserved.
  */
void TextSetString(w, string)
    Widget w;
    String string;
{
    set_changed(w);
    XtVaSetValues(w,
		  XtNstring, string,
		  XtNtype, XawAsciiString,
		  0);
}

/*
  Get the text in a text widget.  If the caller needs to modify the
  text, then TextInvalidate() should be used to indicate the modified
  sections, or the modified text should be passed into TextSet().

  The buffer returned is an allocated copy, which should be freed when
  it is no longer needed.

  If the widget is empty, an empty string will be returned.  Null will
  never be returned.
  */
String TextGetString(w)
    Widget w;
{
    String s, ptr;
    long right;
    XawTextPosition total_read;
    Widget source;
    XawTextBlock b;

    XtVaGetValues(w, XtNtextSource, &source, 0);

    right = XawTextSourceScan(source, (XawTextPosition) 0, XawstAll,
			      XawsdRight, 1, True);

    ptr = s = XtMalloc(right + 1);
    
    for (total_read = 0;
	 (total_read < right) &&
	     (XawTextSourceRead(source, total_read, &b, right - total_read) >
	      total_read);
	 total_read += b.length) {
	(void) strncpy(ptr, b.ptr, b.length);
	ptr += b.length;
    }
    *ptr = '\0';

    return s;
}


/*
  Get the length of a string in a text widget.
  */
long TextGetLength(w)
    Widget w;
{
    String str = TextGetString(w);
    long len = strlen(str);

    XtFree(str);
    return len;
}


/*
  Set the file displayed in a text widget.

  The file name passed in is copied, and therefore doesn't have to be
  preserved.

  The current insertion point and top displayed position are *not*
  preserved.
  */
void TextSetFile(w, file)
    Widget w;
    String file;
{
    /*
      This is necessary because of a bug in the AsciiText widget -- it
      redraws the new file twice when the file being displayed is
      changed, if the top displayed position was greater than 0 in the
      old file.
      */
    TextClear(w);
    set_changed(w);
    XtVaSetValues(w,
		  XtNstring, file,
		  XtNtype, XawAsciiFile,
		  0);
}

/*
  Get the name of the file displayed in a text widget.

  The string returned may be the actual file name used by the widget,
  rather than a copy, so don't muck with it.

  Behavior is undefined when called with a widget that isn't
  displaying a file, unless the widget has been cleared, in which case
  null is returned.
  */
String TextGetFile(w)
    Widget w;
{
    String file;

    XtVaGetValues(w, XtNstring, &file, 0);

    return XtNewString(file);
}


/*
  Replace the text in a portion of a text widget.
  */
void TextReplace(w, string, length, left, right)
    Widget w;
    String string;
    int length;
    long left, right;
{
    XawTextBlock b;
    XawTextEditType type;

    b.firstPos = 0;
    b.length = length;
    b.ptr = string;
    b.format = XawFmt8Bit;

    set_changed(w);
    XtVaGetValues(w, XtNeditType, &type, 0);
    XtVaSetValues(w, XtNeditType, XawtextEdit, 0);
    XawTextReplace(w, (XawTextPosition) left, (XawTextPosition) right, &b);
    XtVaSetValues(w, XtNeditType, type, 0);
}

/*
  Invalidate the text in a portion of a text widget.  The "string"
  argument should be a buffer previously returned by TextGet(), and
  should be identical to the original returned text except for the
  region being invalidated.  The invalidated region will be copied
  from "string" into the text widget.
  */
void TextInvalidate(w, string, left, right)
    Widget w;
    String string;
    long left, right;
{
    TextReplace(w, string + left, right - left, left, right);
}


/*
  Get the boundaries of the current line of a text widget.  Returns
  True if there is text on the current line, or False if there is
  none.
  */
Boolean TextGetCurrentLine(w, left, right)
    Widget w;
    long *left, *right;
{
    Widget source;

    *left = *right = TextGetInsertionPoint(w);

    XtVaGetValues(w, XtNtextSource, &source, 0);

    *left = XawTextSourceScan(source, (XawTextPosition) *left,
			      XawstEOL, XawsdLeft, 1, False);
    *right = XawTextSourceScan(source, (XawTextPosition) *right,
			       XawstEOL, XawsdRight, 1, True);

    if (*left == *right)
	return False;

    return True;
}

/*
  Get the boundaries of the lines currently selected in a text widget.
  The region returned contains only complete lines, even if the
  selected region actually contains partial lines.  Returns True if
  there is a selection being returned, or False otherwise.
  */
Boolean TextGetSelectedLines(w, left, right)
    Widget w;
    long *left, *right;
{
    XawTextPosition left_ret, right_ret;
    Widget source;
    XawTextPosition right2;

    XawTextGetSelectionPos(w, &left_ret, &right_ret);
    *left = left_ret;
    *right = right_ret;

    if (*left == *right)
	return False;

    XtVaGetValues(w, XtNtextSource, &source, 0);
    *left = XawTextSourceScan(source, (XawTextPosition) *left,
			      XawstEOL, XawsdLeft, 1, False);
    right2 = XawTextSourceScan(source, (XawTextPosition) *right,
			       XawstEOL, XawsdLeft, 1, False);
    if (*right != right2)
	/* i.e., the end of the selection isn't already at a line beginning */
	*right = XawTextSourceScan(source, (XawTextPosition) *right,
				   XawstEOL, XawsdRight, 1, True);

    if (*left == *right)
	return False;

    return True;
}

/*
  Gets either the boundaries of the lines currently selected or the
  boundaries of the current line.  Returns True if there is either a
  selected region or text on the current line, or False otherwise.
  */
Boolean TextGetSelectedOrCurrentLines(w, left, right)
    Widget w;
    long *left, *right;
{
    if (TextGetSelectedLines(w, left, right))
	return True;
    else
	return TextGetCurrentLine(w, left, right);
}


/*
  Unset the selection in a text widget.
  */
void TextUnsetSelection(w)
    Widget w;
{
    long garbage;

    if (TextGetSelectedLines(w, &garbage, &garbage)) {
	set_changed(w);
	XawTextUnsetSelection(w);
    }
}


/*
  Get the top displayed position in a text widget.
  */
long TextGetTopPosition(w)
    Widget w;
{
    return XawTextTopPosition(w);
}

/*
  Set the top displayed position in a text widget.
  */
void TextSetTopPosition(w, pos)
    Widget w;
    long pos;
{
    long top = TextGetTopPosition(w);

    if (top == pos)
	return;
    set_changed(w);
    XtVaSetValues(w, XtNdisplayPosition, (XawTextPosition) pos, 0);
}


/*
  Disable redisplay in a text widget.  These can nest, i.e., if you
  call DisableRedisplay twice, you have to call EnableRedisplay twice
  to turn redisplay back on.
  */
void TextDisableRedisplay(w)
    Widget w;
{
    redisplay_t *ptr;

    if (! ((ptr = find_redisplay(w)) ||
	   (ptr = new_redisplay(w))))
	return;

    ptr->count++;
}

/*
  Enable redisplay in a text widget.
  */
void TextEnableRedisplay(w)
    Widget w;
{
    redisplay_t *ptr;

    if (! (ptr = find_redisplay(w)))
	return;

    if (! --ptr->count) {
	if (ptr->changed)
	    XawTextEnableRedisplay(w);
	free_redisplay(ptr);
    }
}

/*
  Force any pending redisplays to occur immediately.
  */
void TextDisplay(w)
    Widget w;
{
    redisplay_t *ptr;

    if (! (ptr = find_redisplay(w)))
	return;

    if (ptr->changed) {
	XawTextEnableRedisplay(w);
	ptr->changed = False;
    }
}


/*
  Get the current insertion point in a text widget.
  */
long TextGetInsertionPoint(w)
    Widget w;
{
    return XawTextGetInsertionPoint(w);
}

/*
  Set the current insertion point in a text widget.  The top displayed
  position is not preserved.
  */
void TextSetInsertionPoint(w, pos)
    Widget w;
    long pos;
{
    long insertion = TextGetInsertionPoint(w);

    if (insertion == pos)
	return;

    set_changed(w);
    XawTextSetInsertionPoint(w, (XawTextPosition) pos);
}


/*
  Remove the text on the line containing the indicated position in a
  text widget.  Deletes the entire line even if the cursor is in the
  middle of the line when the function is called.
  */
void TextRemoveLine(w, position)
    Widget w;
    long position;
{
    Widget source;
    long left, right;

    XtVaGetValues(w, XtNtextSource, &source, 0);

    left = XawTextSourceScan(source, (XawTextPosition) position,
			     XawstEOL, XawsdLeft, 1, False);
    right = XawTextSourceScan(source, (XawTextPosition) position,
			      XawstEOL, XawsdRight, 1, True);
    
    if (left != right)
	TextReplace(w, 0, 0, left, right);
}


/*
  Scroll forward or back a page in a text widget.
  */
void TextScrollPage(w, direction)
    Widget w;
    int direction;
{
    XtCallActionProc(w, (direction == BACK) ? "previous-page" : "next-page",
		     0, 0, 0);
}

/*
  Scroll to the end or beginning of the text in a text
  widget.
  */
void TextScrollEntire(w, direction)
    Widget w;
    int direction;
{
    XtCallActionProc(w, (direction == BACK) ? "beginning-of-file" :
		     "end-of-file", 0, 0, 0);
}

/*
  Scroll forward or back a single ine in a text widget.
  */
void TextScrollLine(w, direction)
    Widget w;
    int direction;
{
    XtCallActionProc(w, (direction == BACK) ? "scroll-one-line-down" :
		     "scroll-one-line-up", 0, 0, 0);
}


/*
  Move the cursor forward or back one line, if possible.
  */
void TextMoveLine(w, direction)
    Widget w;
    int direction;
{
    set_changed(w);
    XtCallActionProc(w, (direction == BACK) ? "previous-line" :
		     "next-line", 0, 0, 0);
}


/*
  Resize a text widget so that it is the specified number of lines
  high.
  */
void TextSetLines(w, lines)
    Widget w;
    int lines;
{
    Widget sink;
    int height;
    Position tm, bm;

    XtVaGetValues(w,
		  XtNtextSink, &sink,
		  XtNbottomMargin, &bm,
		  XtNtopMargin, &tm,
		  0);
    height = XawTextSinkMaxHeight(sink, lines) + tm + bm;
    XtVaSetValues(w, XtNheight, (Dimension) height, 0);
}

/*
  Find out how many lines high a text widget is.
  */
int TextGetLines(w)
    Widget w;
{
    Widget sink;
    Dimension height;
    Position tm, bm;

    XtVaGetValues(w,
		  XtNtextSink, &sink,
		  XtNheight, &height,
		  XtNbottomMargin, &bm,
		  XtNtopMargin, &tm,
		  0);
    return XawTextSinkMaxLines(sink, height - tm - bm);
}


/*
  Return True if a text widget is currently displaying its last page
  of text (i.e., if the end of the text in the widget is visible), or
  False otherwise.

  May not always return True when it should have, but will always
  return False when it should have.  I.e., it may return false
  negatives, but it will never return false positives.  It can't
  always return true when it should have, because I can't figure out
  any way to reliably determine with an Xaw text widget whether or not
  we're on the last page, without modifying the text widget (and
  therefore causing screen flickers).
  */
Boolean TextLastPage(w)
    Widget w;
{
    return False;
}

/*
  Return True if the text widget is currently scrolled past the end of
  the text in it, or if the only text remaining from the top display
  position to the end of the widget is whitespace, or False otherwise.
  */
Boolean TextPastLastPage(w)
    Widget w;
{
    XawTextPosition top = TextGetTopPosition(w);
    String string = TextGetString(w), ptr;
    Boolean ret;

    if (top >= strlen(string)) {
	ret = True;
	goto done;
    }

    for (ptr = string + top; *ptr; ptr++)
	if (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')
	    continue;
	else
	    break;

    if (*ptr)
	ret = False;
    else
	ret = True;

  done:
    XtFree(string);
    return ret;
}
    

/*
  Tell a text widget that text is selectable by lines only.
  */
void TextSetLineSelections(w)
    Widget w;
{
    static XawTextSelectType array[] = {XawselectLine, XawselectNull};

    XtVaSetValues(w, XtNselectTypes, array, 0);
}

/*
  Tell a text widget that any kind of text chunk can be selected.
  */
void TextSetAllSelections(w)
    Widget w;
{
    static XawTextSelectType array[] = {
	XawselectPosition, XawselectChar, XawselectWord, XawselectLine,
	XawselectParagraph, XawselectAll, XawselectNull
    };

    XtVaSetValues(w, XtNselectTypes, array, 0);
}


/*
  Select all the text in a text widget.
  */
void TextSelectAll(w)
    Widget w;
{
    long len;

    len = TextGetLength(w);

    set_changed(w);
    XawTextSetSelection(w, (XawTextPosition) 0, (XawTextPosition) (len + 1));
}


/*
  Search for a string in a text widget.  The starting position, the
  direction in which to search, and the string to search for are
  specified.

  Returns a negative number of the search fails, or the position of
  the start of the match.
  */
long TextSearch(w, start, direction, string)
    Widget w;
    long start;
    TextDirection direction;
    String string;
{
    XawTextBlock b;
    Widget source;
    XawTextPosition ret;

    b.firstPos = 0;
    b.length = strlen(string);
    b.ptr = string;
    b.format = XawFmt8Bit;

    XtVaGetValues(w, XtNtextSource, &source, 0);

    ret = XawTextSourceSearch(source, (XawTextPosition) start,
			      (direction == TextSearchLeft) ?
			      XawsdLeft : XawsdRight, &b);
    if (ret == XawTextSearchError)
	return -1;
    else
	return ret;
}
