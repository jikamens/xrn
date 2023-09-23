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

#ifndef _ARTSTRUCT_H_
#define _ARTSTRUCT_H_

void		artListInit _ARGUMENTS((struct newsgroup *));
void		artListSet _ARGUMENTS((struct newsgroup *));
void		artListExtend _ARGUMENTS((struct newsgroup *, art_num));
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
