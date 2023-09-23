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

#ifndef _FILE_CACHE_H_
#define _FILE_CACHE_H_

typedef struct _file_cache *file_cache;
typedef struct _file_cache_file *file_cache_file;

extern file_cache file_cache_create _ARGUMENTS((char *, char *, int, size_t));
extern int file_cache_destroy _ARGUMENTS((file_cache));
extern char *file_cache_dir_get _ARGUMENTS((file_cache));
extern FILE * file_cache_file_open _ARGUMENTS((file_cache, file_cache_file *));
extern int file_cache_file_close _ARGUMENTS((file_cache, file_cache_file));
extern void file_cache_file_destroy _ARGUMENTS((file_cache, file_cache_file));
extern void file_cache_file_release _ARGUMENTS((file_cache, file_cache_file));
extern void file_cache_file_lock _ARGUMENTS((file_cache, file_cache_file));
extern void file_cache_file_unlock _ARGUMENTS((file_cache, file_cache_file));
extern void file_cache_file_copy _ARGUMENTS((file_cache, file_cache_file,
					     file_cache_file *));
extern char *file_cache_file_name _ARGUMENTS((file_cache, file_cache_file));
extern int file_cache_free_space _ARGUMENTS((file_cache, size_t));

#endif /* _FILE_CACHE_H_ */

