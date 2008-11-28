/*
  A file cache implementation.
  */

#include <assert.h>

#ifdef XRN

#include "config.h"
#include "utils.h"
#include "error_hnds.h"
#define MALLOC(size) XtMalloc(size)
#define CALLOC(nelems, size) XtCalloc(nelems, size)
#define strerror errmsg

#else /* ! XRN */

#include <stdio.h>
#include <memory.h>
#include <sys/stat.h>
#include <unistd.h>
#define MALLOC(size) malloc(size)
#define CALLOC(nelems, size) calloc(nelems, size)
#define FREE(ptr) free(ptr), ptr = NULL
#define utTempnam tempnam
#define utTempnamFree free

#ifndef _ARGUMENTS
#ifdef __STDC__
#define _ARGUMENTS(a) a
#else
#define _ARGUMENTS(a) ()
#endif /* __STDC __ */

#endif /* ! _ARGUMENTS */

#endif /* XRN else */

#include "file_cache.h"

#define MAX_FILES 100	/* if not specified, use this as the maximum
			   number of files in the cache by default */

#define FILE_LOCKED	(1<<0)
#define FILE_OPEN	(1<<1)
#define FILE_VALID	(1<<2)

struct _file_cache_file {
  char *name;
  size_t size;
  char flags;
  file_cache_file *self_ref;
};

struct _file_cache {
  char *dir, *prefix;
  file_cache_file files;
  int current;
  int num_files;
  size_t max_bytes;
  size_t current_bytes;
} file_cache_struct;

static char *file_cache_name_get _ARGUMENTS((file_cache));



/*
  Create a new file cache.  max_files specifies the maximum number of
  files in the cache at any given time; max_bytes specifies the
  maximum amount of space taken up by the files in the cache.  If
  max_files is not specified (i.e., it's 0), it defaults to MAX_FILES;
  if max_bytes is not specified, it defaults to unlimited size.

  Returns NULL on failure (out of memory) or a cache handle on
  success.  Can't return NULL when XRN is defined.
  */

file_cache file_cache_create(dir, prefix, max_files, max_bytes)
     char *dir, *prefix;
     int max_files;
     size_t max_bytes;
{
  file_cache cache;

  if (! (cache = (file_cache) MALLOC(sizeof *cache))) {
#ifdef DEBUG
    fprintf(stderr,
	    "file_cache_create: couldn't allocate space for cache structure\n");
#endif
    return NULL;
  }

  if (! max_files)
    max_files = MAX_FILES;

  cache->dir = dir;
  cache->prefix = prefix;

  if (! (cache->files = (file_cache_file) CALLOC(max_files,
						 sizeof(*cache->files)))) {
#ifdef DEBUG
    fprintf(stderr, "file_cache_create: couldn't allocate space for %d files",
	    max_files);
#endif
    FREE(cache);
    return NULL;
  }

  cache->current = 0;
  cache->num_files = max_files;
  cache->max_bytes = max_bytes;
  cache->current_bytes = 0;

#ifdef DEBUG
  fprintf(stderr,
	  "file_cache_create: created cache 0x%x with %d files, max_bytes %d\n",
	  (unsigned) cache, cache->num_files, cache->max_bytes);
#endif

  return cache;
}

/*
  Destroy a file cache and all the files stored in it.  Returns true
  on success, or false if any of the files in the cache were in use
  (in which case the cache and the files that were in use aren't
  destroyed).
  */

int file_cache_destroy(cache)
     file_cache cache;
{
  int i, in_use = 0;

  assert(cache);

  for (i = 0; i < cache->num_files; i++) {
    if (! (cache->files[i].flags & FILE_VALID))
      continue;
    if (cache->files[i].flags & (FILE_LOCKED|FILE_OPEN)) {
      in_use++;
      continue;
    }
    file_cache_file_destroy(cache, &cache->files[i]);
  }

  if (! in_use) {
#ifdef DEBUG
    fprintf(stderr, "file_cache_destroy: destroyed cache 0x%x\n",
	    (unsigned) cache);
#endif
    FREE(cache->files);
    FREE(cache);
  }
  else {
#ifdef DEBUG
    fprintf(stderr, "file_cache_destroy: %d files in use after destroy\n",
	    in_use);
#endif
  }

  return(! in_use);
}

/*
  Create a new file in the cache, returning a file handle to the file
  opened for writing.  Puts the handle to the cache entry into "file".
  Returns NULL if out of memory, or if out of disk space and couldn't
  free up space by deleting old cache entries, or if all files in the
  cache are locked or open.
  */

FILE *file_cache_file_open(cache, file)
     file_cache cache;
     file_cache_file *file;
{
  int i;
  file_cache_file which
#ifdef GCC_WALL
    = NULL
#endif
    ;
  FILE *fp;
  size_t space_needed = 1;
  
  assert(cache);

  for (i = cache->current; i < cache->current + cache->num_files; i++) {
    which = &cache->files[i % cache->num_files];
    if ((which->flags & FILE_VALID) &&
	(which->flags & (FILE_LOCKED|FILE_OPEN)))
      continue;
    break;
  }

  if (i == cache->current + cache->num_files) {
#ifdef DEBUG
    fprintf(stderr, "file_cache_file_open: no free slots!\n");
#endif
    return NULL;
  }

  if (which->flags & FILE_VALID)
    file_cache_file_destroy(cache, which);

  if (! (which->name = file_cache_name_get(cache))) {
#ifdef DEBUG
    fprintf(stderr, "file_cache_file_open: file_cache_name_get failed!\n");
#endif
    return NULL;
  }

  which->size = 0;

  while ((! (fp = fopen(which->name, "w+"))) &&
	 file_cache_free_space(cache, space_needed))
    space_needed *= 2;

  if (! fp) {
#ifdef DEBUG
    fprintf(stderr, "file_cache_file_open: couldn't create file %s: %s\n",
	    which->name, strerror(errno));
#endif
    utTempnamFree(which->name);
    return NULL;
  }

  which->flags = FILE_VALID|FILE_OPEN;
  which->self_ref = file;

  *file = which;

#ifdef DEBUG
  fprintf(stderr, "file_cache_file_open: created file %s in slot %d (0x%x)\n",
	  which->name, which - cache->files, (unsigned) which);
#endif

  cache->current = (i + 1) % cache->num_files;

  return fp;
}

/*
  Close a file, add its size to the total cache size, lock it, and
  free up old cache files as necessary to bring the cache below its
  maximum size.

  The file handle should have already been closed with fclose() before
  this function was called.

  Returns true on success, or false on fatal error.
  */
int file_cache_file_close(cache, file)
     file_cache cache;
     file_cache_file file;
{
  struct stat sb;
  
  assert(cache);

  file->flags &= ~FILE_OPEN;
  file->flags |= FILE_LOCKED;

  if (stat(file->name, &sb) < 0) {
#ifdef DEBUG
    fprintf(stderr, "file_cache_file_close: couldn't stat %s: %s\n",
	    file->name, strerror(errno));
#endif
    return 0;
  }

  cache->current_bytes += (file->size = sb.st_size);

#ifdef DEBUG
  fprintf(stderr,
	  "file_cache_file_close: closed file %s in slot %d (0x%x), size %d\n",
	  file->name, file - cache->files, (unsigned) file, file->size);
#endif

  (void) file_cache_free_space(cache, 0);

  return 1;
}

/*
  Destroy a file in the cache.  Make sure to close it with fclose()
  before calling this.
  */
void file_cache_file_destroy(cache, file)
     file_cache cache;
     file_cache_file file;
{
  assert(cache);

  if (! file) {
#ifdef DEBUG
    fprintf(stderr, "file_cache_file_destroy: null file\n");
#endif
    return;
  }

#ifdef DEBUG
  fprintf(stderr, "file_cache_file_destroy: destroying file %s "
	  "in slot %d (0x%x), size %d\n",
	  file->name, file - cache->files, (unsigned) file, file->size);
#endif

  file_cache_file_release(cache, file);
  (void) unlink(file->name);
  utTempnamFree(file->name);
  cache->current_bytes -= file->size;
  file->flags = 0;
}

/*
  Release a file in the cache.  Make sure to close it with fclose()
  before calling this.  After a file in the cache is released, the
  caller shouldn't use it anymore, and the file_class package will no
  longer try to reference the memory its handle was stored in
  (although it will erase it as part of releasing the handle).  A
  released file isn't necessary deleted until its space is needed.
  */
void file_cache_file_release(cache, file)
     file_cache cache;
     file_cache_file file;
{
  assert(cache);

  if (! file) {
#ifdef DEBUG
    fprintf(stderr, "file_cache_file_release: null file\n");
#endif
    return;
  }

#ifdef DEBUG
  fprintf(stderr,
	  "file_cache_file_release: releasing file %s in slot %d (0x%x), size %d\n",
	  file->name, file - cache->files, (unsigned) file, file->size);
#endif

  file->flags &= ~(FILE_OPEN|FILE_LOCKED);
  if (file->self_ref) {
    *file->self_ref = NULL;
    file->self_ref = NULL;
  }
}

/*
  Lock a file in the cache.
  */
void file_cache_file_lock(cache, file)
     file_cache cache;
     file_cache_file file;
{
#ifdef DEBUG
  char old_flags = file->flags;
#endif

  assert(cache);

  file->flags |= FILE_LOCKED;

#ifdef DEBUG
  fprintf(stderr,
	  "file_cache_file_lock: locked file %s in slot %d (0x%x), "
	  "old flags 0x%x, new 0x%x\n",
	  file->name, file - cache->files, (unsigned) file, old_flags, file->flags);
#endif
}

/*
  Unlock a file in the cache.

  When a file is closed and unlocked, it may be removed from the cache
  at any point, in which case the contents of the file_cache_file
  pointer passed into file_cache_open will be nulled to tell the
  caller that the cache file has been removed.
  */
void file_cache_file_unlock(cache, file)
     file_cache cache;
     file_cache_file file;
{
#ifdef DEBUG
  char old_flags = file->flags;
#endif

  assert(cache);

  file->flags &= ~FILE_LOCKED;

#ifdef DEBUG
  fprintf(stderr, "file_cache_file_unlock: unlocked file %s in slot %d (0x%x), "
	  "old flags 0x%x, new 0x%x\n",
	  file->name, file - cache->files, (unsigned) file, old_flags, file->flags);
#endif
}

/*
  Get the name of a file in the cache.
  */
char *file_cache_file_name(cache, file)
     file_cache cache;
     file_cache_file file;
{
  assert(cache);

  return file->name;
}

/*
  If possible, free up at least space_needed bytes of space in the
  cache, and at the same time bring the amount of space taken up by
  the cache below its maximum size if it's currently above it.
  Returns true if any space at all was freed, or false if none
  could be freed.
  */
int file_cache_free_space(cache, space_needed)
     file_cache cache;
     size_t space_needed;
{
  int i;
  size_t space_freed = 0;

  assert(cache);

  for (i = (cache->current + cache->num_files - 1) % cache->num_files;
       i != cache->current; i = (i + cache->num_files - 1) % cache->num_files) {
    if ((!cache->max_bytes || cache->current_bytes <= cache->max_bytes) &&
	(space_freed >= space_needed))
      break;
    if (! (cache->files[i].flags & FILE_VALID))
      continue;
    if (cache->files[i].flags & (FILE_OPEN|FILE_LOCKED))
      continue;
    space_freed += cache->files[i].size;
#ifdef DEBUG
    fprintf(stderr, "file_cache_free_space: need %d, freed %d, over by %d\n",
	    space_needed, space_freed, cache->max_bytes ?
	    (cache->current_bytes - cache->max_bytes) : 0);
#endif
    file_cache_file_destroy(cache, &cache->files[i]);
  }

  return space_freed ? 1 : 0;
}

/*
  Copy a file in the file cache to another file, so that the second
  one will be preserved even if the first one is destroyed.
  */
file_cache_file file_cache_file_copy(cache, file)
     file_cache cache;
     file_cache_file file;
{
  FILE *fp;
  char *old_name, *new_name;
  char old_flags;
  file_cache_file new_file;

  assert(cache);

  if (! (file && (old_name = file->name))) {
#ifdef DEBUG
    fprintf(stderr, "file_cache_file_copy: attempt to copy null file!\n");
#endif
    return NULL;
  }

  old_flags = file->flags;

  file->flags |= FILE_LOCKED;

  if (! (fp = file_cache_file_open(cache, &new_file))) {
    file->flags = old_flags;
    return NULL;
  }

  file->flags = old_flags;

  new_name = new_file->name;

  (void) fclose(fp);
  (void) unlink(new_name);

  if (link(old_name, new_name) < 0) {
    file_cache_file_destroy(cache, new_file);
    return NULL;
  }

  if (! file_cache_file_close(cache, new_file)) {
    file_cache_file_destroy(cache, new_file);
    return NULL;
  }

#ifdef DEBUG
  fprintf(stderr, "file_cache_file_copy: copied file %s in slot %d (0x%x) "
	  "into file %s in slot %d (0x%x)\n",
	  file->name, file - cache->files, (unsigned) file,
	  new_file->name, new_file - cache->files, (unsigned) new_file);
#endif

  return new_file;
}

char *file_cache_dir_get(cache)
     file_cache cache;
{
  assert(cache);

  return(cache->dir);
}


static char *file_cache_name_get(cache)
     file_cache cache;
{
  assert(cache);

  return utTempnam(cache->dir, cache->prefix);
}
