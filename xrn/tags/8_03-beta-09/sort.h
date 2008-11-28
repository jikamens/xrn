#ifndef _XRN_SORT_H
#define _XRN_SORT_H

#include "utils.h"

void art_sort_parse_sortlist _ARGUMENTS((char *));
int art_sort_need_dates _ARGUMENTS((void));
int art_sort_need_threads _ARGUMENTS((void));

void *art_sort_init _ARGUMENTS((struct newsgroup *, art_num, art_num,
				/* unsigned char */ int));
char *art_sort_done _ARGUMENTS((void *, int));
void art_sort_cancel _ARGUMENTS((void *));

char *art_sort_doit _ARGUMENTS((struct newsgroup *, art_num, art_num,
				/* unsigned char */ int, int));

void art_sort_by_subject _ARGUMENTS((void *));
void art_sort_by_date _ARGUMENTS((void *));
void art_sort_by_thread _ARGUMENTS((void *));

#endif /* _XRN_SORT_H */
