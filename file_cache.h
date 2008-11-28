#ifndef _FILE_CACHE_H_
#define _FILE_CACHE_H_

typedef struct _file_cache *file_cache;
typedef struct _file_cache_file *file_cache_file;

extern file_cache file_cache_create _ARGUMENTS((char *, char *, int, size_t));
extern int file_cache_destroy _ARGUMENTS((file_cache));
extern FILE * file_cache_file_open _ARGUMENTS((file_cache, file_cache_file *));
extern int file_cache_file_close _ARGUMENTS((file_cache, file_cache_file));
extern void file_cache_file_destroy _ARGUMENTS((file_cache, file_cache_file));
extern void file_cache_file_release _ARGUMENTS((file_cache, file_cache_file));
extern void file_cache_file_lock _ARGUMENTS((file_cache, file_cache_file));
extern void file_cache_file_unlock _ARGUMENTS((file_cache, file_cache_file));
extern file_cache_file file_cache_file_copy _ARGUMENTS((file_cache,
							file_cache_file));
extern char *file_cache_file_name _ARGUMENTS((file_cache, file_cache_file));
extern int file_cache_free_space _ARGUMENTS((file_cache, size_t));

#endif /* _FILE_CACHE_H_ */
