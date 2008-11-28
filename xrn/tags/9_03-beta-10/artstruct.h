#ifndef _ARTSTRUCT_H_
#define _ARTSTRUCT_H_

void		artListInit _ARGUMENTS((struct newsgroup *));
void		artListSet _ARGUMENTS((struct newsgroup *));
void		artListFree _ARGUMENTS((struct newsgroup *));
struct article *artStructGet _ARGUMENTS((struct newsgroup *, art_num, Boolean));
void		artStructSet _ARGUMENTS((struct newsgroup *, struct article **));
void		artStructReplace _ARGUMENTS((struct newsgroup *,
					     struct article **, struct article *,
					     art_num));
struct article *artStructNext _ARGUMENTS((struct newsgroup *, struct article *,
					  art_num *, art_num *));
struct article *artStructPrevious _ARGUMENTS((struct newsgroup *,
					      struct article *, art_num *,
					      art_num *));
struct article *artListFirst _ARGUMENTS((struct newsgroup *, art_num *, art_num *));
struct article *artListLast _ARGUMENTS((struct newsgroup *, art_num *, art_num *));

int		artStructNumChildren _ARGUMENTS((struct article *));
void		artStructAddChild _ARGUMENTS((struct article *, art_num));
void		artStructRemoveChild _ARGUMENTS((struct article *, art_num));

/* Every call to artStructGet "locks" the artstruct interface.  That
   means you can't do another call to artStructGet until you call
   artStructSet, artStructReplace or ART_STRUCT_UNLOCK.  This is
   intended to allow us to easily detect when we're doing nested calls
   into the artstruct interface which could trash each other's
   data. */

#ifndef ARTSTRUCT_C
extern int art_struct_locked;
#endif
#define ART_STRUCT_LOCK art_struct_locked = 1
#define ART_STRUCT_UNLOCK art_struct_locked = 0

#endif /* _ARTSTRUCT_H_ */
