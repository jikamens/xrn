/* Frame.c */
/* and some general X code from xrn.c for the main screen */

#ifdef MOTIF
# include <Xm/Xm.h>
# include <Xm/PanedW.h>
#else
# include <X11/Xos.h>
# include <X11/Intrinsic.h>
# include <X11/Xutil.h>
# include <X11/StringDefs.h>
# include <X11/Shell.h>
# include <X11/Xaw/Paned.h>
#endif

#include "compose.h"
#include "error_hnds.h"

static Widget Frame;
static Arg frameArgs[] = {                  /* main window description */
    {XtNx, (XtArgVal) 10},
    {XtNy, (XtArgVal) 10},
    {XtNheight, (XtArgVal) 800},
    {XtNwidth, (XtArgVal) 680},
};

void
GetMainFrameSize(Widget shell, char *geometry)
{
    int bmask;
    bmask = XParseGeometry(geometry,		    /* geometry specification */
			   (int *) &frameArgs[0].value,             /* x      */
			   (int *) &frameArgs[1].value,             /* y      */
			   (unsigned int *) &frameArgs[3].value,    /* width  */
			   (unsigned int *) &frameArgs[2].value);   /* height */

    /* handle negative x and y values */
    if ((bmask & XNegative) == XNegative) {
	frameArgs[0].value += (XtArgVal) DisplayWidth(XtDisplay(shell),
						      DefaultScreen(XtDisplay(shell)));
	frameArgs[0].value -= (int) frameArgs[3].value;
    }
    if ((bmask & YNegative) == YNegative) {
	frameArgs[1].value += (XtArgVal) DisplayHeight(XtDisplay(shell),
						       DefaultScreen(XtDisplay(shell)));
	frameArgs[1].value -= (int) frameArgs[2].value;
    }
}

Widget 
CreateMainFrame(Widget shell)
{
    Frame = XtCreateManagedWidget("vpane",
#ifdef MOTIF
                                  xmPanedWindowWidgetClass,
#else
                                  panedWidgetClass,
#endif
                                  shell, frameArgs, XtNumber(frameArgs));
    return (Frame);
}

void
DestroyMainFrame(void)
{
    XtDestroyWidget(Frame);
}

/*===========================================================================*/

void
XrnAddInput(XtAppContext app_context, int source)
{
    XtAppAddInput(app_context, source, (XtPointer)XtInputReadMask,
                  processMessage, (XtPointer) 0);
}

void
XrnAddCloseCallbacks(Widget shell)
{
#if XtSpecificationRelease > 5
    XtAddCallback(shell, XtNsaveCallback, saveNewsrcCB, NULL);
    XtAddCallback(shell, XtNdieCallback, ehDieCB, NULL);
#endif /* X11R6 or greater */
}
