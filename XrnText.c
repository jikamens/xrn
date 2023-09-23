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

#ifdef MOTIF
# include <Xm/Xm.h>
# include <Xm/Text.h>
# include <Xm/TextStrSoP.h>
# include <Xm/ScrolledW.h>
#else
# include <X11/Intrinsic.h>
# include <X11/StringDefs.h>
# include <X11/Xaw/AsciiText.h>
# include <X11/Xaw/AsciiSrc.h>
# include <X11/Xaw/AsciiSink.h>
#endif

#include "config.h"
#include "utils.h"
#include "cursor.h"
#include "Text.h"

#if !defined(XawFmt8Bit) && !defined(MOTIF)
#define XawFmt8Bit FMT8BIT
#endif


#define REDISPLAY_NUM 10

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
	    redisplay_array[i].changed = False;
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

    if (! ptr->changed) {
#ifdef MOTIF
	XmTextDisableRedisplay(w);
#else
	XawTextDisableRedisplay(w);
#endif
	ptr->changed = True;
    }
}


/*
  Create a new text widget and return it.  It's initially empty.
  */
Widget TextCreate(
		  _ANSIDECL(String,	name),
		  _ANSIDECL(Boolean,	read_only),
		  _ANSIDECL(Widget,	parent)
		  )
     _KNRDECL(String,	name)
     _KNRDECL(Boolean,	read_only)
     _KNRDECL(Widget,	parent)
{
    Widget w;
    Arg args[2];
    Cardinal num_args = 0;
 
#ifdef MOTIF
    XtSetArg(args[num_args], XmNvalue, ""); num_args++;
    XtSetArg(args[num_args], XmNeditable, !read_only); num_args++;

    XtSetArg(args[num_args], XmNeditMode, XmMULTI_LINE_EDIT); num_args++;
    w = XmCreateScrolledText(parent, name, args, num_args);
    XtManageChild(w);
#else

    XtSetArg(args[num_args], XtNstring, ""); num_args++;
    if (! read_only) {
	XtSetArg(args[num_args], XtNeditType, XawtextEdit); num_args++;
    }

    w = XtCreateManagedWidget(name, asciiTextWidgetClass, parent,
			      args, num_args);
#endif

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
#ifdef MOTIF
		  XmNvalue, string,
#else
		  XtNstring, string,
		  XtNtype, XawAsciiString,
#endif
		  (String)0);
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
    String s;

#ifdef MOTIF
    s = XmTextGetString(w);
    if (s == NULL)
    	s = XtNewString("");
#else
    String ptr;
    long right;
    XawTextPosition total_read;
    Widget source;
    XawTextBlock b;

    XtVaGetValues(w, XtNtextSource, &source, (String)0);

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
#endif

    return s;
}


/*
  Get the length of a string in a text widget.
  */
long TextGetLength(w)
    Widget w;
{
#ifdef MOTIF
    return XmTextGetLastPosition(w);
#else
    String str = TextGetString(w);
    long len = strlen(str);

    XtFree(str);
    return len;
#endif
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
#ifdef MOTIF
    FILE *fs;
    char buffer[BUFSIZ + 1];
    size_t size, end = 0;
#endif

    /*
      This is necessary because of a bug in the AsciiText widget -- it
      redraws the new file twice when the file being displayed is
      changed, if the top displayed position was greater than 0 in the
      old file.
      */
    TextClear(w);
    set_changed(w);

#ifdef MOTIF
    /* XXX This needs to be changed. - jik 5/28/97 */
    if (! (fs = fopen(file, "r"))) {
        sprintf(buffer, "Can't open article file %s", file);
        XmTextSetString(w, buffer);
        return;
    }
    XtUnmanageChild(w);
    while ((size = fread(buffer, 1, BUFSIZ, fs))) {
        buffer[size] = '\0'; /* make sure we're nul terminated */
        XmTextInsert(w, end, buffer);
        end += size;
    }
    fclose(fs);
    TextSetInsertionPoint(w, 0);
    XtManageChild(w);
#else
    XtVaSetValues(w,
		  XtNstring, file,
		  XtNtype, XawAsciiFile,
		  (String)0);
#endif
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
#ifdef MOTIF
    return XmTextGetString(w);
#else
    String file;

    XtVaGetValues(w, XtNstring, &file, (String)0);

    return XtNewString(file);
#endif
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
#ifdef MOTIF
    char save_char;

    set_changed(w);

    save_char = string[length];
    string[length] = '\0';
    XmTextReplace(w, left, right, string);
    string[length] = save_char;
#else
    XawTextBlock b;
    XawTextEditType type;

    b.firstPos = 0;
    b.length = length;
    b.ptr = string;
    b.format = XawFmt8Bit;

#ifdef XAW_REDISPLAY_BUG
    /* Xaw Bug causes the insertion point to move if redisplay is
       disabled and multiple text replacements are performed.  See
       RedHat bugzilla bug number 12801. */
    TextDisplay(w);
#endif

    set_changed(w);
    XtVaGetValues(w, XtNeditType, &type, (String)0);
    XtVaSetValues(w, XtNeditType, XawtextEdit, (String)0);
    XawTextReplace(w, (XawTextPosition) left, (XawTextPosition) right, &b);
    XtVaSetValues(w, XtNeditType, type, (String)0);
#endif
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
#ifdef MOTIF
    char save;

    save = string[right];
    string[right] = '\0';
    TextReplace(w, string + left, right - left, left, right);
    string[right] = save;
#else
    TextReplace(w, string + left, right - left, left, right);
#endif
}


/*
  Get the boundaries of the current line of a text widget.  Returns
  True if there is text on the current line, or False if there is
  none.

  If False is returned, the left and right return pointers are set to
  the the current cursor position.  If True is returned, they're set
  to the boundaries.
  */
Boolean TextGetCurrentLine(w, left_ptr, right_ptr)
    Widget w;
    long *left_ptr, *right_ptr;
{
#ifdef MOTIF
    XmTextPosition left, right;
    XmTextSource source;

    left = right = TextGetInsertionPoint(w);

    source = XmTextGetSource(w);

    left = source->Scan(source, left, XmSELECT_LINE, XmsdLeft, 1, False);
    right = source->Scan(source, right, XmSELECT_LINE, XmsdRight, 1, True);

#else
    Widget source;
    long left, right;

    left = right = TextGetInsertionPoint(w);

    XtVaGetValues(w, XtNtextSource, &source, (String)0);

    left = XawTextSourceScan(source, (XawTextPosition) left,
			     XawstEOL, XawsdLeft, 1, False);
    right = XawTextSourceScan(source, (XawTextPosition) right,
			      XawstEOL, XawsdRight, 1, True);
#endif

    *left_ptr = left;
    *right_ptr = right;

    if (left == right)
	return False;

    return True;
}

/*
  Get the boundaries of the lines currently selected in a text widget.
  The region returned contains only complete lines, even if the
  selected region actually contains partial lines.  Returns True if
  there is a selection being returned, or False otherwise.

  If False is returned, the left and right pointers are set to the
  current cursor position; otherwise, they're set to the boundaries.
  */
Boolean TextGetSelectedLines(w, left, right)
    Widget w;
    long *left, *right;
{
#ifdef MOTIF
    XmTextPosition left_ret, right_ret;
    XmTextSource source;
    XmTextPosition right2;

    XmTextGetSelectionPosition(w, &left_ret, &right_ret);

    if (left_ret == right_ret) {
	*left = *right = left_ret;
	return False;
    }

    source = XmTextGetSource(w);
    left_ret = source->Scan(source, left_ret, XmSELECT_LINE, XmsdLeft, 1, False);
    right2 = source->Scan(source, right_ret, XmSELECT_LINE, XmsdRight, 1, False);
#else
    XawTextPosition left_ret, right_ret;
    Widget source;
    XawTextPosition right2;

    XawTextGetSelectionPos(w, &left_ret, &right_ret);

    if (left_ret == right_ret) {
	*left = *right = left_ret;
	return False;
    }

    XtVaGetValues(w, XtNtextSource, &source, (String)0);
    left_ret = XawTextSourceScan(source, (XawTextPosition) left_ret,
				 XawstEOL, XawsdLeft, 1, False);
    right2 = XawTextSourceScan(source, (XawTextPosition) right_ret,
			       XawstEOL, XawsdLeft, 1, False);
    if (right_ret != right2)
	/* i.e., the end of the selection isn't already at a line beginning */
	right_ret = XawTextSourceScan(source, (XawTextPosition) right_ret,
				      XawstEOL, XawsdRight, 1, True);
#endif

    if (left_ret == right_ret) {
	*left = *right = left_ret;
	return False;
    }

    *left = left_ret;
    *right = right_ret;

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
#ifdef MOTIF
        XmTextClearSelection(w, XtLastTimestampProcessed(XtDisplay(w)));
#else
	XawTextUnsetSelection(w);
#endif
    }
}


/*
  Get the top displayed position in a text widget.
  */
long TextGetTopPosition(w)
    Widget w;
{
#ifdef MOTIF
    return XmTextGetTopCharacter(w);
#else
    return XawTextTopPosition(w);
#endif
}

/*
  Set the top displayed position in a text widget.
  */
void TextSetTopPosition(w, pos)
    Widget w;
    long pos;
{
    long top;

    /*
      XXX This is necessary because of a bug in the Xaw Text widget.
      If the top displayed position changes while text redisplay is
      disabled, the widget doesn't realize it until the batched
      updates are processed.  Therefore, in order to get an accurate
      idea of where the top position really is, and in order to force
      the top position to change properly when we set it, we need to
      process batched updates now.
      */
    TextDisplay(w);

    top = TextGetTopPosition(w);

    if (top == pos)
	return;
    set_changed(w);
#ifdef MOTIF
    XmTextSetTopCharacter(w, pos);
#else
    XtVaSetValues(w, XtNdisplayPosition, (XawTextPosition) pos, (String)0);
#endif
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
#ifdef MOTIF
            XmTextEnableRedisplay(w);
#else
	    XawTextEnableRedisplay(w);
#endif
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
#ifdef MOTIF
	XmTextEnableRedisplay(w);
#else
	XawTextEnableRedisplay(w);
#endif
	ptr->changed = False;
    }
}


/*
  Get the current insertion point in a text widget.
  */
long TextGetInsertionPoint(w)
    Widget w;
{
#ifdef MOTIF
    return XmTextGetInsertionPosition(w);
#else
    return XawTextGetInsertionPoint(w);
#endif
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
#ifdef MOTIF
    XmTextSetInsertionPosition(w, pos);
#else
    XawTextSetInsertionPoint(w, (XawTextPosition) pos);
#endif
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
#ifdef MOTIF
    XmTextPosition left, right;
    XmTextSource source;

    source = XmTextGetSource(w);
    left = source->Scan(source, position, XmSELECT_LINE, XmsdLeft, 1, False);
    right = source->Scan(source, position, XmSELECT_LINE, XmsdRight, 1, True);
#else
    Widget source;
    long left, right;

    XtVaGetValues(w, XtNtextSource, &source, (String)0);

    left = XawTextSourceScan(source, (XawTextPosition) position,
			     XawstEOL, XawsdLeft, 1, False);
    right = XawTextSourceScan(source, (XawTextPosition) position,
			      XawstEOL, XawsdRight, 1, True);
#endif
    
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
  Scroll forward or back a single line in a text widget.
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
#ifdef MOTIF
    XtVaSetValues(w, XmNrows, (short)lines, NULL);
#else
    Widget sink;
    int height;
    Position tm, bm;

    XtVaGetValues(w,
		  XtNtextSink, &sink,
		  XtNbottomMargin, &bm,
		  XtNtopMargin, &tm,
		  (String)0);
    height = XawTextSinkMaxHeight(sink, lines) + tm + bm;
    XtVaSetValues(w, XtNheight, (Dimension) height, (String)0);
#endif
}

/*
  Find out how many lines high a text widget is.
  */
int TextGetLines(w)
    Widget w;
{
#ifdef MOTIF
    short rows;

    XtVaGetValues(w, XmNrows, &rows, NULL);
    return rows;
#else
    Widget sink;
    Dimension height;
    Position tm, bm;

    XtVaGetValues(w,
		  XtNtextSink, &sink,
		  XtNheight, &height,
		  XtNbottomMargin, &bm,
		  XtNtopMargin, &tm,
		  (String)0);
    return XawTextSinkMaxLines(sink, height - tm - bm);
#endif
}


/*
  Find out how many columns wide a text widget is.  Note that if the
  font in the widget is proportional, this tells what the minimum
  number of columns is, assuming that columns are of maximum width.
  */
int TextGetColumns(w)
    Widget w;
{
#ifdef MOTIF
    short columns;

    XtVaGetValues(w, XmNcolumns, &columns, NULL);
    return columns;
#else
    XFontStruct *font;
    Dimension lm, width;
    Position rm;
    
    XtVaGetValues(w,
		  XtNfont, &font,
		  XtNwidth, &width,
		  XtNleftMargin, &lm,
		  XtNrightMargin, &rm,
		  (String)0);

    return((width - lm - rm) / font->max_bounds.width);
#endif
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
/* In Motif, we could do this by (steps 3&4 emulate goto last line on screen):
   * turning display updates off,
   * getting our position and save it,
   * jump to the to of the "page",
   * go down the number of rows showing minus 1,
   * goto the end-of-line,
   * get the position comparing it to XmTextGetLastPosition,
   * put ourselves back to the stored position before we started this mess,
   * turning display updates on,
   * and return the value of the compare.
   But is it really worth the trouble?  (I see no equivalent in Xaw either.)
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
    long top = TextGetTopPosition(w);
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
#ifdef MOTIF
    static XmTextScanType array[] = {XmSELECT_LINE};

    XtVaSetValues(w, XmNselectionArrayCount, XtNumber(array),
		  XmNselectionArray, array, (String)0);
#else
    static XawTextSelectType array[] = {XawselectLine, XawselectNull};

    XtVaSetValues(w, XtNselectTypes, array, (String)0);
#endif
}

/*
  Tell a text widget that any kind of text chunk can be selected.
  */
void TextSetAllSelections(w)
    Widget w;
{
#ifdef MOTIF
    static XmTextScanType array[] = {
	XmSELECT_POSITION, XmSELECT_WORD, XmSELECT_LINE, XmSELECT_ALL
    };

    XtVaSetValues(w, XmNselectionArrayCount, XtNumber(array),
		  XmNselectionArray, array, (String)0);
#else
    static XawTextSelectType array[] = {
	XawselectPosition, XawselectChar, XawselectWord, XawselectLine,
	XawselectParagraph, XawselectAll, XawselectNull
    };

    XtVaSetValues(w, XtNselectTypes, array, (String)0);
#endif
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

#ifdef MOTIF
    XmTextSetSelection(w, 0, len + 1, XtLastTimestampProcessed(XtDisplay(w)));
#else
    XawTextSetSelection(w, (XawTextPosition) 0, (XawTextPosition) (len + 1));
#endif
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
#ifdef MOTIF
    Boolean rc;
    XmTextPosition position;
    
    rc = XmTextFindString(w, (XmTextPosition)start, string,
			  (direction == TextSearchLeft) ?
			  XmTEXT_BACKWARD : XmTEXT_FORWARD, &position);
    return (long)(rc ? position : -1);
#else
    XawTextBlock b;
    Widget source;
    XawTextPosition ret;

    b.firstPos = 0;
    b.length = strlen(string);
    b.ptr = string;
    b.format = XawFmt8Bit;

    XtVaGetValues(w, XtNtextSource, &source, (String)0);

    ret = XawTextSourceSearch(source, (XawTextPosition) start,
			      (direction == TextSearchLeft) ?
			      XawsdLeft : XawsdRight, &b);
    if (ret == XawTextSearchError)
	return -1;
    else
	return ret;
#endif
}

/*
  Do an interactive search of the contents of the Text widget.
  */
void TextSearchInteractive(w, e, start, direction, initial)
     Widget w;
     XEvent *e;
     long start;
     TextDirection direction;
     String initial;
{
  String params[2];
  Cardinal num_params = 1;
  
  if (direction == TextSearchRight)
    params[0] = "forward";
  else
    params[0] = "forward";

  if (initial) {
    params[1] = initial;
    num_params++;
  }

  if (start > 0)
    TextSetInsertionPoint(w, start);

  XtCallActionProc(w, "search", e, params, num_params);
}

/*
  Enable word wrap on a Text widget.
  */
void TextEnableWordWrap(w)
    Widget w;
{
#ifdef MOTIF
    XtVaSetValues(w, XmNwordWrap, True, (String)0);
#else
    XtVaSetValues(w, XtNwrap, XawtextWrapWord, (String)0);
#endif
}

/*
  Disable word wrap on a Text widget.
  */
void TextDisableWordWrap(w)
    Widget w;
{
#ifdef MOTIF
    XtVaSetValues(w, XmNwordWrap, False, (String)0);
#else
    XtVaSetValues(w, XtNwrap, XawtextWrapNever, (String)0);
#endif
}
