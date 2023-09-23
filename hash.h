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

#ifndef _HASH_H
#define _HASH_H

#ifndef _ARGUMENTS
#ifdef __STDC__
#define _ARGUMENTS(a) a
#else
#define _ARGUMENTS(a) ()
#endif
#endif

typedef int (*hash_calc_func) _ARGUMENTS((int, int, void *));
typedef int (*hash_compare_func) _ARGUMENTS((void *, void *));
typedef void (*hash_free_func) _ARGUMENTS((void *));

typedef void *hash_table_object;

hash_table_object hash_table_create
	_ARGUMENTS((int, hash_calc_func,
		    hash_compare_func, hash_compare_func,
		    hash_free_func, hash_free_func));

int hash_table_insert _ARGUMENTS((hash_table_object, void *, void *, int));

void *hash_table_retrieve _ARGUMENTS((hash_table_object, void *, void **));

void hash_table_delete _ARGUMENTS((hash_table_object, void *, void *));

void hash_table_destroy _ARGUMENTS((hash_table_object));

int hash_string_calc _ARGUMENTS((int, int, void *));
int hash_string_compare _ARGUMENTS((void *, void *));

int hash_int_calc _ARGUMENTS((int, int, void *));
int hash_int_compare _ARGUMENTS((void *, void *));

#ifdef DEBUG
void hash_print_stats _ARGUMENTS((hash_table_object));

#define HASH_PRINT_HASHES	(1<<0)
#endif

#define HASH_NO_VALUE (void *)-1

#endif /* _HASH_H */
