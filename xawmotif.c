#include "config.h"

#ifdef MOTIF

#include "copyright.h"
#include "utils.h"
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>

#include "cursor.h"
#include "xrn.h"
#include "buttons.h"

/**********************************************************************
Begin hacked XawText-type routines to interface with XmList.  See
MotifXawHack.h for a short description.
**********************************************************************/

char *TextMotifString = 0;			/* text for top list */
char *ArticleTextMotifString = 0;		/* text for bottom list */
static Boolean isTextSelection = False;		/* did the user highlight? */

/**********************************************************************
Convert an XmString to a C string
**********************************************************************/

#if 0 /* not currentl used */
char * decodeCS(string)
    XmString string;
{
    XmStringContext ctx;
    char *text = 0;
    XmStringCharSet charset;
    XmStringDirection dirRtoL;
    Boolean separator;

    if(!XmStringInitContext(&ctx, string))
	fprintf(stderr, "init failed\n");
    else if(!XmStringGetNextSegment(ctx, &text, &charset, &dirRtoL,
				    &separator))
	fprintf(stderr, "decode failed\n");
    return text;
}
#endif

/**********************************************************************
For the article text / all groups area there is actually both a text
widget and a list widget.  Only one of these is full-sized at a given
time, and this routine switches them.  When text is desired, we shrink
the list and vice-versa.  There is a hidden paned window containing
the two widgets.
**********************************************************************/

Widget ChooseText(b)
    Boolean b;
{
  Widget other, parent;
  Arg args[3];
  Dimension height;

  if ((ArticleText == ArticleTextText && b) ||
      (ArticleText != ArticleTextText && !b)) {
    return ArticleText;
  } else {
    if (ArticleText != ArticleTextText) {
      ArticleText = ArticleTextText;
      parent = ArticleTextText;
      other = XtParent(ArticleTextList);
    } else {
      ArticleText = ArticleTextList;
      parent = XtParent(ArticleTextList);
      other = ArticleTextText;
    }
    XtSetArg(args[0], XmNrefigureMode, False);
    XtSetValues(ArticleContainer, args, 1);
    XtSetArg(args[0], XmNheight, &height);
    XtGetValues(other, args, 1);
    XtSetArg(args[0], XmNpaneMinimum, 1);
    XtSetArg(args[1], XmNpaneMaximum, 1);
    XtSetValues(other, args, 2);
    XtSetArg(args[0], XmNpaneMinimum, height);
    XtSetArg(args[1], XmNpaneMaximum, 1000);
    XtSetValues(parent, args, 2);
    XtSetArg(args[0], XmNrefigureMode, True);
    XtSetValues(ArticleContainer, args, 1);
#ifndef MOTIF_BUG	/* Motif paned widget bug */
    XtUnmanageChild(BottomButtonBox);
    XtManageChild(BottomButtonBox);
#endif
    XtSetArg(args[0], XmNpaneMinimum, 1);
    XtSetValues(parent, args, 1);
    return ArticleText;
  }
}

/**********************************************************************
The stupid Motif paned widget grows the first "malleable" widget to
fill up space.  Therefore, it grows the top button box all the way and
gives the text widget one pixel.  In order to get around this, I
manually compute how high the top button box wants to be (I couldn't
get this value without the parent paned window mucking with its size
first).
**********************************************************************/

int DesiredBoxHeight(w, wlist, wnum)
    Widget w;
    Widget *wlist;
    int wnum;
{
  Arg args[10];
  int ct, height, pos, each;
  Dimension width, marginHeight, marginWidth, ht, wid;
  short spacing;

  ct = 0;
  XtSetArg(args[ct], XmNwidth, &width);  ct++;
  XtSetArg(args[ct], XmNmarginWidth, &marginWidth);  ct++;
  XtGetValues(Frame, args, ct);
  width -= marginWidth*2;
  
  ct = 0;
  XtSetArg(args[ct], XmNmarginHeight, &marginHeight);  ct++;
  XtSetArg(args[ct], XmNmarginWidth, &marginWidth);  ct++;
  XtSetArg(args[ct], XmNspacing, &spacing);  ct++;
  XtGetValues(w, args, ct);

  ct = 0;
  XtSetArg(args[ct], XmNheight, &ht);  ct++;
  XtGetValues(wlist[0], args, ct);

  height = marginHeight-spacing;
  pos = width;
  for (each=0; each<wnum; each++) {
    ct = 0;
    XtSetArg(args[ct], XmNwidth, &wid);  ct++;
    XtGetValues(wlist[each], args, ct);
    if (pos+wid+spacing >= width) {
      height += ht+spacing;
      pos = marginWidth-spacing;
    }
    pos += wid+spacing;
  }
  return height+marginHeight;
}

/**********************************************************************
When the user makes a list selection (i.e. when this is called and
there is a corresponding X event, remember that the user did so.  This
distinguishes between a "selection" vs. "current position," both of
which are indicated by a list item highlight.
**********************************************************************/

void TextListSelection(w, data, calldata)
    Widget w;
    XtPointer data, calldata;
{
  XmListCallbackStruct *lc = (XmListCallbackStruct *) calldata;
  String s = "";
  if (lc->event) {
    if (w != Text) {
      fprintf(stderr, "Unexpected widget in TextListSelection\n");
    }
    isTextSelection = True;
    if (lc->reason == XmCR_DEFAULT_ACTION) {
      doTheRightThing(w, lc->event, &s, 0);
    }
  }
}

/**********************************************************************
These routines provide XawText-type manipulation of the list widget
**********************************************************************/

void XawNothing(w)
    Widget w;
{
  ;
}

/**********************************************************************
Convert a string index into the corresponding Motif list item position
**********************************************************************/

int XawTextToMotifIndex(str, pos)
    char *str;
    int pos;
{
  char *p;
  int result;

  result = 1;
  p = str;
  while (p && p-str < pos && (p = strchr(++p, '\n'))) {
    result++;
    if (p-str >= pos) {
      result--;
    }
  }
  return result;
}

/**********************************************************************
Convert a Motif list item position into its corresponding string index
**********************************************************************/

static int XawMotifIndexToText _ARGUMENTS((Widget, int));

static int XawMotifIndexToText(w, pos)
    Widget w;
    int pos;
{
  char *str, *p, len;
  int each;

  if (w == Text) {
    str = TextMotifString;
  } else if (w == ArticleText) {
    str = ArticleTextMotifString;
  } else {
    fprintf(stderr, "Unknown list widget in XawMotifIndexToText\n");
    exit(1);
  }
  p = str;
  for (each=1; each<pos && p; each++) {
    p = strchr(p, '\n');
    if (p) {
      p++;
    }
  };
  if (!p) {
    return 0;
  }
  return p-str;
}

/**********************************************************************
Redisplay the list items corresponding to the string indices from and
to, getting the new source from the associated string of the list
**********************************************************************/

void XawTextInvalidate(w, from, to)
    Widget w;
    XawTextPosition from, to;
{
  int each, num, *pos_list, pos_count;
  char *p, *q, *r;
  Arg args[3];
  XmString *xs, *items;

  if (!XmListGetSelectedPos(w, &pos_list, &pos_count)) {
    pos_count = 0;
  }
  if (w == Text) {
    p = TextMotifString;
  } else if (w == ArticleText) {
    p = ArticleTextMotifString;
  } else {
    fprintf(stderr, "Unknown list widget in XawTextInvalidate\n");
    exit(1);
  }
  if (!p || !strlen(p)) {
    XtSetArg(args[0], XmNitemCount, 0);
    XtSetArg(args[1], XmNitems, NULL);
    XtSetArg(args[2], XmNvalue, "");
    XtSetValues(w, args, 3);
  } else {
    if (!from && !to) {
      XawTextInvalidateAll(w, p);
      return;
    }
    if (!from) {
      from = 1;
    } else {
      from = XawTextToMotifIndex(p, from);
    }
    if (!to) {
      to = XawTextToMotifIndex(p, strlen(p)-1);
    } else {
      to = XawTextToMotifIndex(p, to);
    }
    XtSetArg(args[0], XmNitemCount, &num);
    XtSetArg(args[1], XmNitems, &items);
    XtGetValues(w, args, 2);
    if (from > num) {
      from = num+1;
    }
    xs = (XmString *) XtCalloc(to-from+1, sizeof(XmString));
    for (each=1; each<from; each++) {
      p = strchr(p, '\n');
      if (p) {
	p++;
      }
    }
    for (each=from; each<=to; each++) {
      if (p) {
	q = strchr(p, '\n');
	if (q) {
	  *q = '\0';
	}
	r = strchr(p, '\t');
	if (r) {
	  *r = ' ';
	}
	xs[each-from] = XmStringCreate(p, XmSTRING_DEFAULT_CHARSET);
	if (q) {
	  *q = '\n';
	}
	if (r) {
	  *r = '\t';
	}
      }
      if (!p) {
	fprintf(stderr, "Unexpected NULL pointer in XawTextInvalidate\n");
	exit(1);
      } else {
	p = strchr(p, '\n');
	if (p) {
	  p++;
	}
      }
    }
    if (to <= num) {
      XmListReplaceItemsPos(w, xs, to-from+1, from);
    } else {
      XmListReplaceItemsPos(w, xs, num-from+1, from);
      XmListAddItems(w, xs+num+1-from, to-num, num+1);
    }
    for (each=from; each<=to; each++) {
      XmStringFree(xs[each-from]);
    }
    XtFree((char *) xs);
    if (pos_count) {
      XmListSelectPos(w, pos_list[0], False);
      XtFree((char *) pos_list);
    }
  }
}

/**********************************************************************
Replace the entire list contents with the contents of the string
**********************************************************************/

void XawTextInvalidateAll(w, p)
    Widget w;
    char *p;
{
  XawTextPosition from, to;
  int each, num, len;
  char *q, *r;
  Arg args[3];
  XmString *xs, *items;

  from = 1;
  to = XawTextToMotifIndex(p, strlen(p)-1);
  len = to;
  xs = (XmString *) XtCalloc(len, sizeof(XmString));
  for (each=0; each<len; each++) {
    xs[each] = (XmString) 0;
  }
  for (each=from; each<=to; each++) {
    if (p) {
      q = strchr(p, '\n');
      if (q) {
	*q = '\0';
      }
      if (xs[each-1]) {
	XmStringFree(xs[each-1]);
      }
      r = strchr(p, '\t');
      if (r) {
	*r = ' ';
      }
      xs[each-1] = XmStringCreate(p, XmSTRING_DEFAULT_CHARSET);
      if (q) {
	*q = '\n';
      }
      if (r) {
	*r = '\t';
      }
    }
    if (!p) {
      fprintf(stderr, "Unexpected NULL pointer in XawTextInvalidateAll\n");
      exit(1);
    } else {
      p = strchr(p, '\n');
      if (p) {
	p++;
      }
    }
  }
  XtSetArg(args[0], XmNitemCount, len);
  XtSetArg(args[1], XmNitems, xs);
  XtSetValues(w, args, 2);
  for (each=0; each<len; each++) {
    XmStringFree(xs[each]);
  }
  XtFree((char *) xs);
}

/**********************************************************************
Remember the string associated with this list, since it gives the new
source when portions of the list are invalidated
**********************************************************************/

void XawTextSetMotifString(w, source)
    Widget w;
    char *source;
{
  if (w == Text) {
    TextMotifString = source;
  } else if (w == ArticleText) {
    ArticleTextMotifString = source;
    if (source) {
      w = ChooseText(False);
    }
  } else {
    fprintf(stderr, "Unknown widget in XawTextSetMotifString\n");
    exit(1);
  }
  XawTextInvalidate(w, 0, 0);
  XawTextUnsetSelection(w);
}

/**********************************************************************
Highlight the list item associated with the string index
**********************************************************************/

void XawTextSetInsertionPoint(w, pos)
    Widget w;
    XawTextPosition pos;
{
  int topItemPosition, visibleItemCount;
  Arg args[2];

  if (w == Text) {
    pos = XawTextToMotifIndex(TextMotifString, pos);
  } else if (w == ArticleText) {
    pos = XawTextToMotifIndex(ArticleTextMotifString, pos);
  } else {
    fprintf(stderr, "Unknown widget in XawTextSetInsertionPoint\n");
    exit(1);
  }
  XtSetArg(args[0], XmNtopItemPosition, &topItemPosition);
  XtSetArg(args[1], XmNvisibleItemCount, &visibleItemCount);
  XtGetValues(w, args, 2);
  XmListDeselectAllItems(w);
  XmListSelectPos(w, pos, False);
  if (pos < topItemPosition ||
      pos >= topItemPosition+visibleItemCount) {
    XmListSetBottomPos(w, pos);
  }
}

/**********************************************************************
Return the string index corresponding to the highlighted list item
**********************************************************************/

XmTextPosition XawTextGetInsertionPoint(w)
    Widget w;
{
  int *pos_list, pos_count, pos;
  
  if (!XmListGetSelectedPos(w, &pos_list, &pos_count)) {
    return 0;
  }
  if (!pos_count) {
    return 0;
  } else {
    if (pos_count > 1) {
      fprintf(stderr,
	      "Unexpected multiple selection in XawTextGetInsertionPoint\n");
    }
    pos = pos_list[0];
    XtFree((char *) pos_list);
    return XawMotifIndexToText(w, pos);
  }
}

/**********************************************************************
Erase memory of the users selection.  This has no visible effect
because we still want the current list position highlighted.  However,
if we're asked for the selection later, we say there is none.
**********************************************************************/

void XawTextUnsetSelection(w)
    Widget w;
{
  if (w == Text) {
    isTextSelection = False;
  }
}

/**********************************************************************
Return the string range corresponding to the list items selected, if
any.  Note that even though an item might be highlighted, it might not
be a selection because the user didn't highlight it directly.
**********************************************************************/

void XawTextGetSelectionPos(w, left, right)
    Widget w;
    XawTextPosition *left, *right;
{
  XmString *selected;
  int count;
  Arg args[2];
  char *item, *p;

  XtSetArg(args[0], XmNselectedItemCount, &count);
  XtSetArg(args[1], XmNselectedItems, &selected);
  XtGetValues(w, args, 2);
  if (!count || (w == Text && !isTextSelection)) {
    *left = *right = 0;
  } else {
    *left = XawMotifIndexToText(w, XmListItemPos(w, selected[0]));
    *right = XawMotifIndexToText(w, XmListItemPos(w, selected[count-1])+1);
  }
}

/**********************************************************************
Return the string index corresponding to the top item in the list
**********************************************************************/

int XawTextTopPosition(w)
    Widget w;
{
  Arg args[1];
  int topItemPosition;

  XtSetArg(args[0], XmNtopItemPosition, &topItemPosition);
  XtGetValues(w, args, 1);
  return topItemPosition;
}

#endif /* MOTIF */
