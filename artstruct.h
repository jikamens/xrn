#ifndef _ARTSTRUCT_H_
#define _ARTSTRUCT_H_

void		artListInit _ARGUMENTS((struct newsgroup *));
void		artListSet _ARGUMENTS((struct newsgroup *));
void		artListFree _ARGUMENTS((struct newsgroup *));
struct article *artStructGet _ARGUMENTS((struct newsgroup *, art_num,
					 /* Boolean */ int));
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

#endif /* _ARTSTRUCT_H_ */
