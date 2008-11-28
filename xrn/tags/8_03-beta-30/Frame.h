/* Frame.h */

#ifndef FRAME_H
#define FRAME_H

Widget CreateMainFrame _ARGUMENTS((Widget shell));
void   DestroyMainFrame _ARGUMENTS((void));
void   GetMainFrameSize _ARGUMENTS((Widget Shell, char *geometry));

void XrnAddInput _ARGUMENTS((XtAppContext app_context, int source));
void XrnAddCloseCallbacks _ARGUMENTS((Widget shell));

#endif /* FRAME_H */
