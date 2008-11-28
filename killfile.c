#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <errno.h>

#include "config.h"
#include "utils.h"
#include "news.h"
#include "killfile.h"
#include "internals.h"
#include "server.h"
#include "mesg.h"
#include "mesg_strings.h"
#include "resources.h"
#include "error_hnds.h"
#include "avl.h"
#include "dialogs.h"

#define BUFFER_SIZE 1024

/*
  Eventually, if 'h' isn't specified, only the subject should be
  killed, but this is how XRN behaved before, so I'm continuing to
  do it this way for backward compatibility. XXX
  */
#define DEF_CHECK_FLAGS		(KILL_SUBJECT|KILL_AUTHOR)
#define ALL_CHECK_FLAGS		(KILL_SUBJECT|KILL_AUTHOR|\
				 KILL_NEWSGROUPS|KILL_DATE|\
				 KILL_ID|KILL_REFERENCES)

static void free_entry_contents _ARGUMENTS((kill_entry *));
static char field_to_check_flag _ARGUMENTS((char *, int));
static Boolean entry_expired _ARGUMENTS((kill_entry *, time_t));
static void write_kf _ARGUMENTS((kill_file *));
static kill_file *read_kf _ARGUMENTS((char *file_name,
				      char *reference,
				      unsigned char *fetch_flags));


/*
  Return true if the indicated character, which should be in the
  string pointed to by start, is backslash-quoted, or false otherwise.
  Deals correctly with quoted backslashes immediately following a
  non-quoted character.
  */
static Boolean isQuoted _ARGUMENTS((char *, char *));

static Boolean isQuoted(character, start)
    char *character, *start;
{
    /*
      Figure out how many backslashes there are between the indicated
      character and either the first non-backslash or the beginning of
      the string.

      If the number of backslashes is odd, then return True.
      Otherwise, return False.
      */
    Boolean quoted = False;

    for (character--;
	 (character >= start) && (*character == '\\');
	 character--)
	quoted = ! quoted;

    return quoted;
}

static void parse_kill_entry _ARGUMENTS((char *, char *, kill_file *,
					 kill_entry *,
					 unsigned char *, int));

/*
  Parse a KILL-file entry.  If there's a parse error, this routine
  will display  it.  The resulting parsed entry is put into entry,
  or it's unmodified if the line should not be added to the kill-entry
  list.

  The kill_entry structure contains a union whose value is dependent
  on what type of entry has been parsed.  Right now, only "entry",
  "include" and "other" (i.e., strings which are ignored by XRN) are
  supported.  The "entry" type contains a pointer to the compiled regexp
  (if necessary), a set of flags indicating which fields to check, and a
  set of flags indicating the action (junk the article, mark it read,
  or save it). The "include" type contains the operand of the include 
  (which either is the name of the kill file, or the name of a newsgroup),
  a pointer to the kill file contents, and a flag indicating whether this
  file belongs to an existing newsgroup.

  Lines beginning with "&", lines beginning with "#", and blank lines
  (or lines containing only whitespace) are ignored.  "THRU" lines are
  put into the kill-file structure.

  If there's a trailing newline in the line being parsed, it will be
  replaced in-place with a null.  Otherwise, the input string will not
  be modified.

  If this function determines that the "Newsgroups" line of the
  article is going to have to be checked while processing this entry,
  and newsgroup->fetch & FETCH_NEWSGROUPS is false, it will add
  FETCH_NEWSGROUPS to *fetch_flags.  The caller should detect this
  and react appropriately. Ditto for the other FETCH_* fields.
  */
static void parse_kill_entry(file_name, in_str, file, entry,
			     fetch_flags, mesg_name)
     char *file_name;
     char *in_str;
     kill_file *file;
     kill_entry *entry;
     unsigned char *fetch_flags;
     int mesg_name;
{
  kill_entry my_entry;
  char *ptr, *ptr2;
#ifdef POSIX_REGEX
  int reRet;
#else
# ifndef SYSV_REGEX
  char *reRet;
# endif /* ! SYSV_REGEX */
#endif /* POSIX_REGEX */
  char pattern[MAX_KILL_ENTRY_LENGTH];
  char include_arg[BUFFER_SIZE];
  char *str = in_str;
  struct newsgroup *otherGroup;

  if (! strncmp(str, "THRU ", 5)) {
    file->thru = atol(str + 5);
    return;
  }

  /*
    Ignore the trailing newline.
    */
  if ((ptr = index(str, '\n')))
    *ptr = '\0';

  memset((char *) &my_entry, 0, sizeof(my_entry));

  /*
    Ignore whitespace at the beginning of the line.
    */
  for (ptr = str; *ptr && isspace((unsigned char)*ptr); ptr++)
    /* empty */;

  if ((*ptr == '&') || (*ptr == '#') || (! *ptr)) {
    my_entry.type = KILL_OTHER;
    goto done;
  }

  if (index(app_resources.verboseKill, 'l'))
    mesgPane(XRN_INFO, mesg_name, KILL_LINE_MSG, str, file_name);

  if (!strncmp (ptr, "include", 7)) {
    ptr += 7;

    if (!isspace((unsigned char)*ptr)) {
      mesgPane(XRN_SERIOUS, mesg_name, MALFORMED_KILL_ENTRY_MSG, str,
               file_name, ERROR_INCLUDE_NOT_SEPARATED_MSG);
      my_entry.type = KILL_OTHER;
      goto done;
    }

    for (; *ptr && isspace((unsigned char)*ptr); ptr++)
      /* empty */;

    if (! *ptr) {
      mesgPane(XRN_SERIOUS, mesg_name, MALFORMED_KILL_ENTRY_MSG,
	       str, file_name, ERROR_INCLUDE_MISSING_MSG);
      my_entry.type = KILL_OTHER;
      goto done;
    }

    for (ptr2 = strchr(ptr, '\0');
	 (ptr2 > ptr) && isspace((unsigned char)*(ptr2 - 1));
	 ptr2--)
      /* empty */;

    strncpy(include_arg, ptr, MIN(ptr2 - ptr, sizeof(include_arg) - 1));
    include_arg[MIN(ptr2 - ptr, sizeof(include_arg) - 1)] = '\0';

    if (verifyGroup(include_arg, &otherGroup, True)) {
      my_entry.include.is_ngfile = True;
      my_entry.include.operand = XtNewString(include_arg);
    }
    else {
      my_entry.include.is_ngfile = False;

      if (*include_arg != '/')
        strcpy (include_arg, utTildeExpand (include_arg));

      if (*include_arg != '/') {
	int i;
	i = strlen(app_resources.expandedSaveDir);
	(void) memmove(&include_arg[i+1], include_arg,
		       MIN(strlen(include_arg)+1, sizeof(include_arg)-i));
	(void) memmove(include_arg, app_resources.expandedSaveDir, i);
        include_arg[i] = '/';
      }

      my_entry.include.operand = XtNewString(include_arg);
    }

    my_entry.type = KILL_INCLUDE;
    goto done;
  }

  if (*ptr != '/') {
    mesgPane(XRN_SERIOUS, mesg_name, MALFORMED_KILL_ENTRY_MSG, str,
	     file_name, ERROR_REGEX_NOSLASH_START_MSG);
    my_entry.type = KILL_OTHER;
    goto done;
  }

  /*
    Find the end of the regular expression
    */
  for (ptr++, ptr2 = ptr;
       *ptr2 && ((*ptr2 != '/') || isQuoted(ptr2, ptr)); ptr2++)
    /* empty */;

  if (! *ptr2) {
    mesgPane(XRN_SERIOUS, mesg_name, MALFORMED_KILL_ENTRY_MSG, str,
	     file_name, ERROR_REGEX_NOSLASH_MSG);
    my_entry.type = KILL_OTHER;
    goto done;
  }

  /*
    We're leaving a character free at the beginning of "pattern" so
    that we can anchor the pattern later if we have to.
    */
  strncpy(pattern + 1, ptr, MIN(ptr2 - ptr, sizeof(pattern) - 1));
  pattern[MIN(ptr2 - ptr + 1, sizeof(pattern) - 1)] = '\0';

  ptr = pattern + 1;

  my_entry.entry.pattern = XtNewString(ptr);

  /*
    rn puts ": *" at the front of its KILL file entries; XRN doesn't.
    */
  if (! strncmp(ptr, ": *", 3))
    ptr += 3;
  /*
    I'm not convinced this is correct, but it was here before, so I'm
    leaving it in.
    */
  if (*ptr == ':')
    ptr++;

  for (ptr2++; *ptr2 && (*ptr2 != ':'); ptr2++) {
    if (*ptr2 == 'h') {
      char *ptr3 WALL(= 0);

      if (*ptr == '^') {
	if ((ptr3 = strchr(ptr + 1, ':'))) {
	  my_entry.entry.check_flags =
	    field_to_check_flag(ptr + 1, ptr3 - (ptr + 1));
	}
      }

      if (my_entry.entry.check_flags) {
	ptr = ptr3 + 1;
	while (*ptr && isspace((unsigned char)*ptr))
	  ptr++;
	*--ptr = '^';
      }
      else
	my_entry.entry.check_flags = ALL_CHECK_FLAGS;
    }
    else if (*ptr2 == 't') {	/* kill timeout */
      for ( ; *(ptr2+1) && isdigit((unsigned char)*(ptr2 + 1)); ptr2++) {
	my_entry.entry.timeout *= 10;
	my_entry.entry.timeout += (*(ptr2+1) - '0');
      }
    }
    else if (*ptr2 == 'u') {	/* last used */
      for ( ; *(ptr2+1) && isdigit((unsigned char)*(ptr2 + 1)); ptr2++) {
	my_entry.entry.last_used *= 10;
	my_entry.entry.last_used += (*(ptr2+1) - '0');
      }
    }
    else {
      mesgPane(XRN_SERIOUS, mesg_name, KILL_ERROR_UNKNOWN_OPTION_MSG,
	       str, file_name, *ptr2);
      my_entry.type = KILL_OTHER;
      goto done;
    }
  }

  if (my_entry.entry.timeout && !my_entry.entry.last_used)
    my_entry.entry.last_used = time(0);

  if (! my_entry.entry.check_flags) {
    my_entry.entry.check_flags = DEF_CHECK_FLAGS;
  }

  if (! *ptr2) {
    mesgPane(XRN_SERIOUS, mesg_name, MALFORMED_KILL_ENTRY_MSG, str,
	     file_name, ERROR_REGEX_NOCOLON_MSG);
    my_entry.type = KILL_OTHER;
    goto done;
  }

  /*
    Eventually, multiple actions should be allowed in one entry. XXX
    */
  switch (*++ptr2) {
  case 'j':
    my_entry.entry.action_flags = KILL_JUNK;
    break;
  case 'm':
    my_entry.entry.action_flags = KILL_MARK;
    break;
  case 's':
    my_entry.entry.action_flags = KILL_SAVE;
    break;
  case 't':
    my_entry.entry.action_flags = KILL_SUBTHREAD;
    break;
  case 'T':
    my_entry.entry.action_flags = KILL_THREAD;
    break;
  default:
    mesgPane(XRN_SERIOUS, mesg_name, MALFORMED_KILL_ENTRY_MSG, str,
	     file_name, ERROR_REGEX_UNKNOWN_COMMAND_MSG);
    my_entry.type = KILL_OTHER;
    goto done;
  }

#ifdef POSIX_REGEX
  if ((reRet = regcomp(&my_entry.entry.reStruct, ptr, REG_NOSUB))) {
    regerror(reRet, &my_entry.entry.reStruct, error_buffer, sizeof(error_buffer));
    mesgPane(XRN_SERIOUS, mesg_name, KNOWN_KILL_REGEXP_ERROR_MSG,
	     str, file_name, error_buffer);
    my_entry.type = KILL_OTHER;
    goto done;
  }
#else
# ifdef SYSV_REGEX
  if (! (my_entry.entry.reStruct = regcmp(ptr, NULL))) {
    mesgPane(XRN_SERIOUS, mesg_name, UNKNOWN_KILL_REGEXP_ERROR_MSG,
	     str, file_name);
    my_entry.type = KILL_OTHER;
    goto done;
  }
# else
  if ((reRet = re_comp(ptr))) {
    mesgPane(XRN_SERIOUS, mesg_name, KNOWN_KILL_REGEXP_ERROR_MSG,
	     str, file_name, reRet);
    my_entry.type = KILL_OTHER;
    goto done;
  }
  my_entry.entry.reStruct = XtNewString(ptr);
# endif /* SYSV_REGEX */
#endif /* POSIX_REGEX */

  if (my_entry.entry.check_flags & KILL_NEWSGROUPS)
    *fetch_flags |= FETCH_NEWSGROUPS;
  if (my_entry.entry.check_flags & KILL_DATE)
    *fetch_flags |= FETCH_DATES;
  if (my_entry.entry.check_flags & KILL_ID)
    *fetch_flags |= FETCH_IDS;
  if (my_entry.entry.check_flags & KILL_REFERENCES)
    *fetch_flags |= FETCH_REFS;
  if (my_entry.entry.check_flags & KILL_XREF)
    *fetch_flags |= FETCH_XREF;

  if (my_entry.entry.action_flags == KILL_SUBTHREAD)
    *fetch_flags |= FETCH_IDS;
  if (my_entry.entry.action_flags == KILL_THREAD)
    *fetch_flags |= FETCH_IDS & FETCH_REFS;

  my_entry.type = KILL_ENTRY;

done:
  if (my_entry.type != KILL_ENTRY && my_entry.type != KILL_INCLUDE) {
    free_entry_contents(&my_entry);
  }
  my_entry.any.value = XtNewString(in_str);

  *entry = my_entry;
}


struct kftab_entry {
  char ref_count;
  unsigned char fetch_flags;
  kill_file *kill_file;
};

static avl_tree *kf_table = 0;

static void clear_seen _ARGUMENTS((void));

static void clear_seen()
{
  avl_generator *gen;
  struct kftab_entry *tab_entry;
  struct newsgroup *newsgroup;

  if (! kf_table)
    return;

  gen = avl_init_gen(kf_table, AVL_FORWARD);

  while (avl_gen(gen, 0, (char **) &tab_entry))
    tab_entry->kill_file->flags &= ~KF_SEEN;

  avl_free_gen(gen);

  gen = avl_init_gen(NewsGroupTable, AVL_FORWARD);

  while (avl_gen(gen, 0, (char **) &newsgroup))
    if (newsgroup->kill_file)
      ((kill_file *)newsgroup->kill_file)->flags &= ~KF_SEEN;

  avl_free_gen(gen);
}


static void unparse_kill_entry _ARGUMENTS((kill_entry *, char *));

static void unparse_kill_entry(entry, buffer)
     kill_entry *entry;
     char *buffer;
{
  char *ptr;

  if (entry->type == KILL_OTHER) {
    (void) sprintf(buffer, "%.*s\n", MAX_KILL_ENTRY_LENGTH-2,
		   entry->any.value);
  }
  else if (entry->type == KILL_INCLUDE) {
    (void) sprintf(buffer, "%.*s\n", MAX_KILL_ENTRY_LENGTH-2,
		   entry->any.value);
    if (! entry->include.is_ngfile) {
      struct kftab_entry *tab_entry;
      int ret;

      assert(kf_table);
      ret = avl_lookup(kf_table, entry->include.kf->file_name,
		       (char **) &tab_entry);
      assert(ret);

      if (! --tab_entry->ref_count) {
	char *old_file_name = entry->include.kf->file_name;
	char *file_name = old_file_name;

	write_kf(entry->include.kf);
	entry->include.kf = 0;

	ret = avl_delete(kf_table, &file_name, 0);
	XtFree(old_file_name);
	XtFree((char *)tab_entry);
      }
    }
  }
  else if (entry->type == KILL_ENTRY) {
    (void) sprintf(buffer, "/%s/", entry->entry.pattern);
    ptr = &buffer[strlen(buffer)];
    if (entry->entry.check_flags != DEF_CHECK_FLAGS)
      *ptr++ = 'h';
    if (entry->entry.timeout) {
      (void) sprintf(ptr, "t%d", entry->entry.timeout);
      ptr += strlen(ptr);
      if (entry->entry.last_used) {
	(void) sprintf(ptr, "u%ld", entry->entry.last_used);
	ptr += strlen(ptr);
      }
    }
    *ptr++ = ':';
    switch(entry->entry.action_flags) {
    case KILL_JUNK:
      *ptr++ = 'j';
      break;
    case KILL_MARK:
      *ptr++ = 'm';
      break;
    case KILL_SAVE:
      *ptr++ = 's';
      break;
    case KILL_SUBTHREAD:
      *ptr++ = 't';
      break;
    case KILL_THREAD:
      *ptr++ = 'T';
      break;
    default:
      assert(0);
    }
    *ptr++ = '\n';
    *ptr = '\0';
  }
  else {
    assert(0);
  }
}


static kill_file *GlobalKillFile = 0;
static unsigned char GlobalFetchFlags = 0;


void read_global_kill_file(newsgroup)
     struct newsgroup *newsgroup;
{

  if (!GlobalKillFile) {
    GlobalKillFile = read_kf(globalKillFile(), 0, &GlobalFetchFlags);
  }

  newsgroup->fetch |= GlobalFetchFlags;
}

void read_local_kill_file(newsgroup)
     struct newsgroup *newsgroup;
{
  char *file_name;

  if (newsgroup->kill_file)
    return;

  file_name = localKillFile(newsgroup, 0);

  newsgroup->kill_file = (void *) read_kf(file_name, 0, &newsgroup->fetch);
}

static kill_file *read_kf(file_name, reference, fetch_flags)
     char *file_name;
     char *reference;
     unsigned char *fetch_flags;
{
  int entries_size;
  FILE *fp;
  kill_file *kf;
  struct stat statbuf;
  char buf[MAX_KILL_ENTRY_LENGTH];
  int mesg_name = newMesgPaneName();

  kf = (kill_file *) XtCalloc(1, sizeof(*kf));
  kf->file_name = XtNewString(file_name);

  if ((stat(file_name, &statbuf) < 0) || 
      (! (fp = fopen(file_name, "r")))) {
    if (reference)
      mesgPane(XRN_SERIOUS, mesg_name, CANT_OPEN_INCLUDED_KILL_MSG,
	       file_name, reference, errmsg(errno));
    else if (errno != ENOENT)
      mesgPane(XRN_SERIOUS, mesg_name, CANT_OPEN_KILL_MSG,
	       file_name, errmsg(errno));
    kf->mod_time = 0;
    return kf;
  }

  kf->mod_time = statbuf.st_mtime;

  entries_size = 1;
  kf->entries = (kill_entry *) XtCalloc(entries_size, sizeof(*kf->entries));
  
  while (fgets(buf, sizeof(buf), fp)) {
    kill_entry *entry;

    if ((! strchr(buf, '\n')) && (! (feof(fp) || ferror(fp)))) {
      mesgPane(XRN_SERIOUS, mesg_name, KILL_TOO_LONG_MSG, buf, file_name);
      do {
	*buf = fgetc(fp);
      } while (! (feof(fp) || ferror(fp) || (*buf == '\n')));
      continue;
    }
    if (kf->count == entries_size) {
      entries_size *= 2;
      kf->entries = (kill_entry *) XtRealloc((char *) kf->entries,
					     entries_size * sizeof(*kf->entries));
      memset((char *) &kf->entries[kf->count], 0,
	     (entries_size / 2) * sizeof(*kf->entries));
    }

    entry = &kf->entries[kf->count];

    parse_kill_entry(file_name, buf, kf, entry, fetch_flags, mesg_name);

    if (entry->type)
      kf->count++;

    if (entry->type == KILL_INCLUDE) {
      if (entry->include.is_ngfile) {
	struct newsgroup *otherGroup = 0;

	verifyGroup(entry->include.operand, &otherGroup, True);
	assert(otherGroup);

	read_local_kill_file(otherGroup);
	*fetch_flags |= otherGroup->fetch;
	entry->include.kf = (kill_file *) otherGroup->kill_file;
      }
      else {
	struct kftab_entry *tab_entry;

	if (! kf_table)
	  kf_table = avl_init_table(strcmp);

	if (avl_lookup(kf_table, entry->include.operand,
		       (char **) &tab_entry)) {
	  tab_entry->ref_count++;
	}
	else {
	  int ret;

	  tab_entry = (struct kftab_entry *) XtCalloc(1, sizeof(*tab_entry));
	  tab_entry->ref_count = 1;
	  tab_entry->fetch_flags = 0;
	  tab_entry->kill_file = read_kf(entry->include.operand,
					 file_name,
					 &tab_entry->fetch_flags);

	  ret = avl_insert(kf_table, tab_entry->kill_file->file_name,
			   (char *) tab_entry);
	  assert(! ret);
	}
	
	entry->include.kf = tab_entry->kill_file;
	*fetch_flags |= tab_entry->fetch_flags;
      }
    }
  }

  (void) fclose(fp);

  return kf;
}

static kill_entry *kf_iter _ARGUMENTS((kill_file *, kill_file_iter_handle *,
				       Boolean));

static kill_entry *kf_iter(
			   _ANSIDECL(kill_file *,		kf),
			   _ANSIDECL(kill_file_iter_handle *,	handle_p),
			   _ANSIDECL(Boolean,			expand_includes)
			   )
     _KNRDECL(kill_file *,		kf)
     _KNRDECL(kill_file_iter_handle *,	handle_p)
     _KNRDECL(Boolean,			expand_includes)
{
  kill_entry *sub_entry = 0;
  int handle = (int) *handle_p;
  kill_entry *this_entry;

  assert(kf);

  if (! handle_p) {
    handle = 1;
    if (expand_includes)
      kf->flags |= KF_SEEN;
  }
  else if (kf->cur_sub_kf) {
    assert(expand_includes);
    assert(kf->cur_entry);

    sub_entry = kf_iter(kf->cur_sub_kf, handle_p, expand_includes);

    if (sub_entry)
      return sub_entry;

    kf->cur_sub_kf = 0;
    handle = kf->cur_entry;
    handle++;
  } else {
    handle++;
  }

  if (handle > kf->count)
    return 0;

  this_entry = &kf->entries[handle-1];

#if !defined(POSIX_REGEX) && !defined(SYSV_REGEX)
  if (this_entry->type == KILL_ENTRY)
    (void) re_comp(this_entry->entry.reStruct);
#endif

  if ((this_entry->type == KILL_INCLUDE) && expand_includes) {
    if (! (this_entry->include.kf->flags & KF_SEEN)) {
      kill_file_iter_handle sub_handle = 0;

      sub_entry = kf_iter(this_entry->include.kf, &sub_handle, expand_includes);

      if (sub_entry) {
	kf->cur_entry = handle;
	kf->cur_sub_kf = this_entry->include.kf;
	*handle_p = sub_handle;
	return sub_entry;
      }
    }

    *handle_p = (kill_file_iter_handle) handle;
    return kf_iter(kf, handle_p, expand_includes);
  }

  *handle_p = (kill_file_iter_handle) handle;
  return this_entry;
}

kill_entry *kill_file_iter(newsgroup, mode, handle)
     struct newsgroup *newsgroup;
     int mode;
     kill_file_iter_handle *handle;
{
  kill_file *kf;

  if (mode == KILL_LOCAL)
    kf = (kill_file *) newsgroup->kill_file;
  else
    kf = GlobalKillFile;

  clear_seen();

  return kf_iter(kf, handle, True);
}

#define CHECK_WRITE(cmd) if (! (cmd)) { \
       mesgPane(XRN_SERIOUS, mesg_name, ERROR_WRITING_FILE_MSG, temp_file, \
		errmsg(errno)); \
       (void) fclose(fp); \
       (void) unlink(temp_file); \
       goto done; \
}

static void write_kf(kf)
     kill_file *kf;
{
  char *temp_file = 0;
  FILE *fp;
  char buf[MAX_KILL_ENTRY_LENGTH];
  kill_entry *entry;
  kill_file_iter_handle handle = 0;
  time_t now = time(0);
  int mesg_name = newMesgPaneName();

  assert (kf);

  if (kf->flags & KF_CHANGED) {
    struct stat statbuf;

    if (kf->mod_time) {
      if ((stat(kf->file_name, &statbuf) < 0) && (errno != ENOENT)) {
	/* Not a completely accurate error, but close enough. */
	mesgPane(XRN_SERIOUS, mesg_name, CANT_OPEN_KILL_MSG, kf->file_name,
		 errmsg(errno));
	goto done;
      }

      if (kf->mod_time != statbuf.st_mtime) {
	(void) sprintf(error_buffer, ASK_FILE_MODIFIED_MSG, "Kill",
		       kf->file_name);
	if (ConfirmationBox(TopLevel, error_buffer, 0, 0, False) ==
	    XRN_CB_ABORT)
	  goto done;
      }
    }

    temp_file = utTempFile(kf->file_name);

    if (! (fp = fopen(temp_file, "w"))) {
      mesgPane(XRN_SERIOUS, mesg_name, CANT_CREATE_TEMP_MSG, temp_file,
	       errmsg(errno));
      goto done;
    }

    if (kf->thru) {
      CHECK_WRITE(fprintf(fp, "THRU %ld\n", kf->thru) != EOF);
    }

    while ((entry = kf_iter(kf, &handle, False))) {
      if (entry_expired(entry, now))
	continue;
      unparse_kill_entry(entry, buf);
      CHECK_WRITE(fputs(buf, fp) != EOF);
    }

    if (fclose(fp) == EOF) {
      mesgPane(XRN_SERIOUS, mesg_name, ERROR_WRITING_FILE_MSG, temp_file,
	       errmsg(errno));
      (void) unlink(temp_file);
      goto done;
    }

    if (rename(temp_file, kf->file_name)) {
      mesgPane(XRN_SERIOUS, mesg_name, ERROR_RENAMING_MSG, temp_file,
	       kf->file_name, errmsg(errno));
      (void) unlink(temp_file);
    }

done:
    XtFree(temp_file);
  }

  handle = 0;
  while ((entry = kf_iter(kf, &handle, False)))
    free_entry_contents(entry);
  XtFree((char *)kf->entries);
  XtFree((char *)kf);
}

void write_kill_file(newsgroup, mode)
     struct newsgroup *newsgroup;
     int mode;
{
  kill_file **kf_ptr, *kf;

  if (mode == KILL_LOCAL)
    kf_ptr = (kill_file **) &newsgroup->kill_file;
  else
    kf_ptr = &GlobalKillFile;
  
  kf = *kf_ptr;

  if (kf) {
    char *old_file_name = kf->file_name;
    write_kf(kf);
    XtFree(old_file_name);
  }

  *kf_ptr = 0;
}

static void free_entry_contents(entry)
     kill_entry *entry;
{
  XtFree(entry->any.value);

  switch (entry->type) {
  case KILL_INCLUDE:
    XtFree(entry->include.operand);
    break;
  case KILL_ENTRY:
    XtFree(entry->entry.pattern);
#ifdef POSIX_REGEX
    regfree(&entry->entry.reStruct);
#else /* SYSV_REGEX or BSD regexps */
# ifdef SYSV_REGEX
    free(entry->entry.reStruct);
# else /* BSD regexps */
    XtFree(entry->entry.reStruct);
# endif /* SYSV_REGEX */
#endif /* POSIX_REGEX */
    break;
  }
}

Boolean has_kill_files(newsgroup)
     struct newsgroup *newsgroup;
{
  if (newsgroup && newsgroup->kill_file)
    return True;

  if (GlobalKillFile)
    return True;

  return False;
}

void add_kill_entry(newsgroup, mode, field, regexp)
     struct newsgroup *newsgroup;
     int mode;
     char *field, *regexp;
{
  kill_file **kf_ptr, *kf;
  char buf[MAX_KILL_ENTRY_LENGTH];
  FILE *fp;
  char *file;
  kill_entry my_entry;
  int mesg_name = newMesgPaneName();
  unsigned char *fetch_ptr;
  struct stat statbuf;

  if (mode == KILL_LOCAL) {
    file = localKillFile(newsgroup, 1);
    kf_ptr = (kill_file **) &newsgroup->kill_file;
    fetch_ptr = &newsgroup->fetch;
  } else {
    file = globalKillFile();
    kf_ptr = &GlobalKillFile;
    fetch_ptr = &GlobalFetchFlags;
  }

  kf = *kf_ptr;

  memset((char *) &my_entry, 0, sizeof(my_entry));

  my_entry.type = KILL_ENTRY;
  my_entry.entry.value = 0;

  if (field) {
    assert(strlen(regexp) <= MAX_KILL_PATTERN_VALUE_LENGTH);
    (void) sprintf(buf, "^%s: .*%s", field, regexp);
    my_entry.entry.pattern = XtNewString(buf);
    my_entry.entry.check_flags = field_to_check_flag(field, strlen(field));
    if (! my_entry.entry.check_flags)
      my_entry.entry.check_flags = ALL_CHECK_FLAGS;
  }
  else {
    assert(strlen(regexp) <= MAX_KILL_PATTERN_LENGTH);
    my_entry.entry.pattern = XtNewString(regexp);
    my_entry.entry.check_flags = DEF_CHECK_FLAGS;
  }
  
  my_entry.entry.action_flags = KILL_JUNK;
  my_entry.entry.timeout = app_resources.killTimeout;
  my_entry.entry.last_used = time(0);

  unparse_kill_entry(&my_entry, buf);

  free_entry_contents(&my_entry);

  if (stat(file, &statbuf) < 0)
    statbuf.st_mtime = 0;

  /*
    XXX Check mod time of file and update it in the kf structure.
    */
  
  if ((fp = fopen(file, "a")) == NULL) {
    mesgPane(XRN_SERIOUS, mesg_name, CANT_OPEN_KILL_MSG,
	     file, errmsg(errno));
  }
  else if ((fputs(buf, fp) == EOF) || (fclose(fp) == EOF)) {
    mesgPane(XRN_SERIOUS, mesg_name, ERROR_WRITING_FILE_MSG, file,
	     errmsg(errno));
  }

  if (kf) {
    if (statbuf.st_mtime && kf->mod_time &&
	(statbuf.st_mtime == kf->mod_time) && (stat(file, &statbuf) >= 0))
      kf->mod_time = statbuf.st_mtime;

    kf->count++;
    kf->entries = (kill_entry *) XtRealloc((char *)kf->entries,
					   kf->count *
					   sizeof(*kf->entries));
    memset((char *) &kf->entries[kf->count-1], 0, sizeof(*kf->entries));
    parse_kill_entry("internal", buf, kf,
		     &kf->entries[kf->count-1], fetch_ptr,
		     mesg_name);
    assert(kf->entries[kf->count-1].type == KILL_ENTRY);
  }
}

#define CHECK_FIELD(value, flag) \
  if (! strncmp(field_name, (value), MAX(field_length, sizeof(value)-1))) { \
    return(flag); \
  }

static char field_to_check_flag(field_name, field_length)
     char *field_name;
     int field_length;
{
  CHECK_FIELD("Subject", KILL_SUBJECT);
  CHECK_FIELD("From", KILL_AUTHOR);
  CHECK_FIELD("Newsgroups", KILL_NEWSGROUPS);
  CHECK_FIELD("Date", KILL_DATE);
  CHECK_FIELD("Message-ID", KILL_ID);
  CHECK_FIELD("References", KILL_REFERENCES);
  CHECK_FIELD("Xref", KILL_XREF);

  return 0;
}

#undef CHECK_FIELD

static Boolean entry_expired(entry, now)
     kill_entry *entry;
     time_t now;
{
  if (entry->type != KILL_ENTRY)
    return False;

  if (! entry->entry.timeout)
    return False;

  if ((now - entry->entry.last_used) >
      (entry->entry.timeout * 24 * 60 * 60))
    return True;

  return False;
}

void kill_update_last_used(file, entry)
     kill_file *file;
     kill_entry *entry;
{
  if (entry->type != KILL_ENTRY)
    return;

  if (! entry->entry.timeout)
    return;

  entry->entry.last_used = time(0);

  if (file->cur_sub_kf)
    kill_update_last_used(file->cur_sub_kf, entry);
  else
    file->flags |= KF_CHANGED;
}
