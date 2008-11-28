
! From: sun!ai.mit.edu!dms (David M. Siegel)
! Date: Sun, 9 Dec 90 18:07:38 EST
!
! emacs-like control to the article window

XRn.artBindings: \
  c<Key>v:	artScroll()	\n\
  m<Key>v:	artScrollBack() \n\
  c<Key>n:	artScrollLine()	\n\
  c<Key>p:	artScrollBackLine() \n\
  m<Key>>:	artScrollEnd()	\n\
  m<Key><:	artScrollBeginning  \n\
  <Key>b:	artScrollBack()		\n\
  <Key>0x20:	doTheRightThing(goto)	\n\
  ^<Key>N:	artSubNext()	\n\
  :<Key>N:	artNext()	\n\
  :<Key>n:	artNextUnread()	\n\
  ^<Key>P:	artSubPrev()	\n\
  <Key>P:	artPrev()	\n\
  :<Key>-:	artLast()	\n\
  :<Key>/:	artSubSearch()	\n\
  <Key>f:	artFollowup()	\n\
  <Key>r:	artReply()	\n\
  <Key>s:	artSave()	\n\
  <Key>w:	artSave()	\n\
  <Key>|:	artSave()	\n\
  <Key>%:	artSave("| uudecode")	\n\
  :<Key>C:	artCancel()	\n\
  :<Key>v:	artHeader()	\n\
  ^<Key>X:	artRot13()	\n\
  :<Key>X:	artRot13()	\n\
  :<Key>c:	artCatchUp()	\n\
  :<Key>j:	artMarkRead()	\n\
  <Key>m:	artMarkUnread()	\n\
  :<Key>k:	artKillSession()	\n\
  :<Key>K:	artKillLocal()	\n\
  :<Key>u:	artUnsub()	\n\
  <Key>.:	artGotoArticle()	\n\
  <Key>q:	artQuit()

