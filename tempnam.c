

/* from Jonathan I. Kamens          <jik@pit-manager.mit.edu> */

#include "config.h"
#ifdef NEED_TEMPNAM

#include "utils.h"
#include <sys/file.h>

#ifndef P_tmpdir
#define P_tmpdir	"/usr/tmp/"
#endif

#ifdef HAVE_MKSTEMP
static char **open_file_list;
static int open_file_list_size = 0;
#endif

static char check_directory _ARGUMENTS((char *));

static char check_directory(dir)
    char *dir;
{
     struct stat statbuf;

     if (! dir)
	  return 0;
     else if (stat(dir, &statbuf) < 0)
	  return 0;
     else if ((statbuf.st_mode & S_IFMT) != S_IFDIR)
	  return 0;
     else if (access(dir, W_OK | X_OK) < 0)
	  return 0;
     else
	  return 1;
}

/* function for creating temporary filenames */
char *utTempnam(dir, pfx)
    char *dir, *pfx;
{
#ifndef STDC_HEADERS
     extern char *getenv();
#endif
     char *tmpdir = NULL, *env, *filename;
#ifdef HAVE_MKSTEMP
     int fd;
#else
     static char unique_letters[4] = "AAA";
#endif
     char addslash = 0;
     
     /*
      * Check TMPDIR first.  If that fails, then check the directory
      * (if any) that is passed in.  If that fails, check the
      * predefined constant P_tmpdir.  If that fails set, use "/tmp/".
      */

     if ((env = getenv("TMPDIR")) && check_directory(env))
       tmpdir = env;
     else if (dir && check_directory(dir))
	  tmpdir = dir;
#ifdef P_tmpdir
     else if (check_directory(P_tmpdir))
	  tmpdir = P_tmpdir;
#endif
     else
	  tmpdir = "/tmp/";
     
     /*
      * OK, now that we've got a directory, figure out whether or not
      * there's a slash at the end of it.
      */
     if (tmpdir[strlen(tmpdir) - 1] != '/')
	  addslash = 1;

#ifdef HAVE_MKSTEMP

     filename = XtMalloc(strlen(tmpdir) + addslash + utStrlen(pfx) + 7);
     (void) sprintf(filename, "%s%s%sXXXXXX", tmpdir, addslash ? "/" : "",
                    pfx ? pfx : "");
     fd = mkstemp(filename);
     if (fd >= open_file_list_size) {
       open_file_list = (char **) XtRealloc((char *) open_file_list,
                                            (fd + 1) * sizeof(*open_file_list));
       for (int i = open_file_list_size; i < fd; i++)
         open_file_list[i] = NULL;
       open_file_list_size = fd + 1;
     }
     open_file_list[fd] = filename;
     
#else /* ! HAVE_MKSTEMP */

     /*
      * Now figure out the set of unique letters.
      */
     unique_letters[0]++;
     if (unique_letters[0] > 'Z') {
	  unique_letters[0] = 'A';
	  unique_letters[1]++;
	  if (unique_letters[1] > 'Z') {
	       unique_letters[1] = 'A';
	       unique_letters[2]++;
	       if (unique_letters[2] > 'Z') {
		    unique_letters[2]++;
	       }
	  }
     }

     /*
      * Allocate a string of sufficient length.
      */
     filename = XtMalloc(strlen(tmpdir) + addslash + utStrlen(pfx) + 10);
     
     /*
      * And create the string.
      */
     (void) sprintf(filename, "%s%s%s%sa%05d", tmpdir, addslash ? "/" : "",
		    pfx ? pfx : "", unique_letters, getpid());

#endif /* ! HAVE_MKSTEMP */

     return filename;
}

#ifdef HAVE_MKSTEMP

void utTempnamFree(char *filename)
{
  int i;
  for (i = 0; i < open_file_list_size; i++)
    if (open_file_list[i] && ! strcmp(open_file_list[i], filename)) {
      close(i);
      open_file_list[i] = NULL;
      XtFree(filename);
      return;
    }
}

#endif /* HAVE_MKSTEMP */

#endif /* NEED_TEMPNAM */
