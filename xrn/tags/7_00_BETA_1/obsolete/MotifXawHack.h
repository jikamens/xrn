/**********************************************************************

This header file declares the routines used to take in XawText-type
manipulations and instead perform operations on a Motif list widget.
This was the simplest way to convert xrn to Motif while still
retaining most of the original code.  By translating at a lower level,
more modifications and fixes to xrn can be done once without worry
about Xaw vs. Motif.

Currently, the code for these routines is in buttons.c.  It would be
more correct to separate this into a separate file, but then this
complicates the Makefile for when MOTIF is defined vs. when it isn't.

**********************************************************************/

typedef int XawTextPosition;

#define XawTextDisableRedisplay XawNothing	/* no equivalent */
#define XawTextEnableRedisplay XawNothing	/* no equivalent */

extern void XawNothing _ARGUMENTS((Widget));
extern int XawTextToMotifIndex _ARGUMENTS((char *,int));
extern void XawTextInvalidateAll _ARGUMENTS((Widget,char*));
extern void XawTextSetMotifString _ARGUMENTS((Widget,char*));
extern void XawTextSetInsertionPoint _ARGUMENTS((Widget,XawTextPosition));
extern XmTextPosition XawTextGetInsertionPoint _ARGUMENTS((Widget));
extern void XawTextUnsetSelection _ARGUMENTS((Widget));
extern int XawTextTopPosition _ARGUMENTS((Widget));
extern void XawTextInvalidate _ARGUMENTS((Widget,XawTextPosition,
    XawTextPosition));
extern void XawTextGetSelectionPos _ARGUMENTS((Widget,XawTextPosition *,
    XawTextPosition *));

extern Widget ChooseText _ARGUMENTS((Boolean));
extern int DesiredBoxHeight _ARGUMENTS((Widget,Widget *,int));
extern void TextListSelection _ARGUMENTS((Widget,XtPointer,
    XmListCallbackStruct *));
