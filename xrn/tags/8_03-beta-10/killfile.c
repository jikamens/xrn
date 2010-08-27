#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <errno.h>

#include "config.h"
#include "utils.h"
#include "news.h"
#include "killfile.h"
#include "internals.h"
#include "mesg.h"
#include "mesg_strings.h"
#include "resources.h"
#include "error_hnds.h"

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

static void parse_kill_entry _ARGUMENTS((struct newsgroup *, char *,
					 char *, kill_file *,
					 kill_entry *, int));

/*
  Parse a KILL-file entry.  If there's a parse error, this routine
  will display  it.  The resulting parsed entry is put into entry,
  or it's unmodified if the line should not be added to the kill-entry
  list.

  The kill_entry structure contains a union whose value is dependent
  on what type of entry has been parsed.  Right now, only "entry" and
  "other" (i.e., strings which are ignored by XRN) are supposed.  The
  "entry" type contains a pointer to the compiled regexp (if
  necessary), a set of flags indicating which fields to check, and a
  set of flags indicating the action (junk the article, mark it read,
  or save it).

  Lines beginning with "&", lines beginning with "#", and blank lines
  (or lines containing only whitespace) are ignored.  "THRU" lines are
  put into the kill-file structure.

  If there's a trailing newline in the line being parsed, it will be
  replaced in-place with a null.  Otherwise, the input string will not
  be modified.

  If this function determines that the "Newsgroups" line of the
  article is going to have to be checked while processing this entry,
  and newsgroup->fetch & FETCH_NEWSGROUPS is false, it will add
  FETCH_NEWSGROUPS to newsgroup->fetch.  The caller should detect this
  and react appropriately. Ditto for the other FETCH_* fields.
  */
static void parse_kill_entry(newsgroup, file_name, in_str, file, entry,
			     mesg_name)
     struct newsgroup *newsgroup;
     char *file_name;
     char *in_str;
     kill_file *file;
     kill_entry *entry;
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
  char *str = in_str;

  if (! strncmp(str, "THRU ", 5)) {
    file->thru = atol(str + 5);
    return;
  }

  /*
    Ignore the trailing newline.
    */
  if ((ptr = index(str, '\n')))
    *ptr = '\0';

  memset(&my_entry, 0, sizeof(my_entry));

  /*
    Ignore whitespace at the beginning of the line.
    */
  for (ptr = str; *ptr && isspace(*ptr); ptr++)
    /* empty */;

  if ((*str == '&') || (*str == '#') || (! *str)) {
    my_entry.type = KILL_OTHER;
    goto done;
  }

  if (index(app_resources.verboseKill, 'l'))
    mesgPane(XRN_INFO, mesg_name, KILL_LINE_MSG, str, file_name);

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

  memset(&my_entry, 0, sizeof(my_entry));

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
	ptr3 = strchr(ptr + 1, ':');
	if (ptr3 && (ptr3 < ptr2)) {
	  my_entry.entry.check_flags =
	    field_to_check_flag(ptr + 1, ptr3 - (ptr + 1));
	}
      }

      if (my_entry.entry.check_flags) {
	ptr = ptr3 + 1;
	while (*ptr && isspace(*ptr))
	  ptr++;
	*--ptr = '^';
      }
      else
	my_entry.entry.check_flags = ALL_CHECK_FLAGS;
    }
    else if (*ptr2 == 't') {	/* kill timeout */
      for ( ; *(ptr2+1) && isdigit(*(ptr2 + 1)); ptr2++) {
	my_entry.entry.timeout *= 10;
	my_entry.entry.timeout += (*(ptr2+1) - '0');
      }
    }
    else if (*ptr2 == 'u') {	/* last used */
      for ( ; *(ptr2+1) && isdigit(*(ptr2 + 1)); ptr2++) {
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
    newsgroup->fetch |= FETCH_NEWSGROUPS;
  if (my_entry.entry.check_flags & KILL_DATE)
    newsgroup->fetch |= FETCH_DATES;
  if (my_entry.entry.check_flags & KILL_ID)
    newsgroup->fetch |= FETCH_IDS;
  if (my_entry.entry.check_flags & KILL_REFERENCES)
    newsgroup->fetch |= FETCH_REFS;

  my_entry.type = KILL_ENTRY;

done:
  if (my_entry.type != KILL_ENTRY) {
    free_entry_contents(&my_entry);
  }
  my_entry.any.value = XtNewString(in_str);

  *entry = my_entry;
}

static void unparse_kill_entry _ARGUMENTS((kill_entry *, char *));

static void unparse_kill_entry(entry, buffer)
     kill_entry *entry;
     char *buffer;
{
  char *ptr;

  if (entry->type == KILL_OTHER) {
    (void) sprintf(buffer, "%*.*s\n",
		   -(MAX_KILL_ENTRY_LENGTH-2),
		   MAX_KILL_ENTRY_LENGTH-2,
		   entry->any.value);
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

void read_kill_file(newsgroup, mode)
     struct newsgroup *newsgroup;
     int mode;
{
  int entries_size;
  FILE *fp;
  char *file_name;
  kill_file **kf_ptr, *kf;
  struct stat statbuf;
  char buf[MAX_KILL_ENTRY_LENGTH];
  int mesg_name = newMesgPaneName();

  if (mode == KILL_LOCAL) {
    kf_ptr = (kill_file **) &newsgroup->kill_file;
    file_name = localKillFile(newsgroup, 0);
  }
  else {
    kf_ptr = &GlobalKillFile;
    file_name = globalKillFile();
  }

  if (*kf_ptr)
    return;

  kf = (kill_file *) XtCalloc(1, sizeof(*kf));
  kf->file_name = XtNewString(file_name);

  if ((stat(file_name, &statbuf) < 0) ||
      (! (fp = fopen(file_name, "r")))) {
    if (errno != ENOENT) {
      if (newsgroup)
	mesgPane(XRN_SERIOUS, mesg_name, CANT_OPEN_KILL_MSG,
		 file_name, newsgroup->name, errmsg(errno));
      else
	mesgPane(XRN_SERIOUS, mesg_name, CANT_OPEN_GLOBAL_KILL_MSG,
		 file_name, errmsg(errno));
    }
    *kf_ptr = kf;
    return;
  }

#if 0 /* see comment in killfile.h */
  kf->mod_time = statbuf.st_mtime;
#endif

  entries_size = 1;
  kf->entries = (kill_entry *) XtCalloc(entries_size, sizeof(*kf->entries));
  
  while (fgets(buf, sizeof(buf), fp)) {
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
      memset(&kf->entries[kf->count], 0,
	     (entries_size / 2) * sizeof(*kf->entries));
    }
    parse_kill_entry(newsgroup, file_name, buf, kf, &kf->entries[kf->count],
		     mesg_name);
    if (kf->entries[kf->count].type)
      kf->count++;
  }

  (void) fclose(fp);

  *kf_ptr = kf;
}

kill_entry *kill_file_iter(newsgroup, mode, last_entry)
     struct newsgroup *newsgroup;
     int mode;
     kill_entry *last_entry;
{
  kill_file *kf;

  if (mode == KILL_LOCAL)
    kf = (kill_file *) newsgroup->kill_file;
  else
    kf = GlobalKillFile;

  assert(kf);

  if (! last_entry)
    last_entry = kf->entries;
  else
    last_entry++;

  if (last_entry - kf->entries == kf->count)
    return 0;

#if !defined(POSIX_REGEX) && !defined(SYSV_REGEX)
  if (last_entry->type == KILL_ENTRY)
    (void) re_comp(last_entry->entry->reStruct);
#endif

  return last_entry;
}

#define CHECK_WRITE(cmd) if (! (cmd)) { \
       mesgPane(XRN_SERIOUS, mesg_name, ERROR_WRITING_FILE_MSG, temp_file, \
		errmsg(errno)); \
       (void) fclose(fp); \
       (void) unlink(temp_file); \
       XtFree(temp_file); \
       return; \
}

void write_kill_file(newsgroup, mode)
     struct newsgroup *newsgroup;
     int mode;
{
  kill_file **kf_ptr, *kf;
  char *temp_file;
  FILE *fp;
  char buf[MAX_KILL_ENTRY_LENGTH];
  kill_entry *entry = 0;
  time_t now = time(0);
  int mesg_name = newMesgPaneName();

  if (mode == KILL_LOCAL)
    kf_ptr = (kill_file **) &newsgroup->kill_file;
  else
    kf_ptr = &GlobalKillFile;
  
  kf = *kf_ptr;

  if (! kf)
    return;

  if (kf->count) {
    temp_file = utTempFile(kf->file_name);

    if (! (fp = fopen(temp_file, "w"))) {
      mesgPane(XRN_SERIOUS, mesg_name, CANT_CREATE_TEMP_MSG, temp_file,
	       errmsg(errno));
      XtFree(temp_file);
      return;
    }

    if (kf->thru) {
      CHECK_WRITE(fprintf(fp, "THRU %ld\n", kf->thru) != EOF);
    }

    while ((entry = kill_file_iter(newsgroup, mode, entry))) {
      if (entry_expired(entry, now))
	continue;
      unparse_kill_entry(entry, buf);
      CHECK_WRITE(fputs(buf, fp) != EOF);
    }

    if (fclose(fp) == EOF) {
      mesgPane(XRN_SERIOUS, mesg_name, ERROR_WRITING_FILE_MSG, temp_file,
	       errmsg(errno));
      (void) unlink(temp_file);
      XtFree(temp_file);
      return;
    }

    if (rename(temp_file, kf->file_name)) {
      mesgPane(XRN_SERIOUS, mesg_name, ERROR_RENAMING_MSG, temp_file,
	       kf->file_name, errmsg(errno));
      (void) unlink(temp_file);
      XtFree(temp_file);
      return;
    }

    XtFree(temp_file);
  }

  while ((entry = kill_file_iter(newsgroup, mode, entry)))
    free_entry_contents(entry);
  XtFree(kf->file_name);
  XtFree((char *)kf->entries);
  XtFree((char *)kf);
  *kf_ptr = 0;
}

static void free_entry_contents(entry)
     kill_entry *entry;
{
  XtFree(entry->any.value);
  XtFree(entry->entry.pattern);
#ifdef POSIX_REGEX
  regfree(&entry->entry.reStruct);
#else
  XtFree(entry->entry.reStruct);
#endif /* POSIX_REGEX */
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

  if (mode == KILL_LOCAL) {
    file = localKillFile(newsgroup, 1);
    kf_ptr = (kill_file **) &newsgroup->kill_file;
  } else {
    file = globalKillFile();
    kf_ptr = &GlobalKillFile;
  }

  kf = *kf_ptr;

  memset(&my_entry, 0, sizeof(my_entry));

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

  /*
    XXX Check mod time of file and update it in the kf structure.
    */
  
  if ((fp = fopen(file, "a")) == NULL) {
    if (mode == KILL_LOCAL)
      mesgPane(XRN_SERIOUS, mesg_name, CANT_OPEN_KILL_MSG,
	       file, newsgroup->name, errmsg(errno));
    else
      mesgPane(XRN_SERIOUS, mesg_name, CANT_OPEN_GLOBAL_KILL_MSG,
	       file, errmsg(errno));
  }
  else if ((fputs(buf, fp) == EOF) || (fclose(fp) == EOF)) {
    mesgPane(XRN_SERIOUS, mesg_name, ERROR_WRITING_FILE_MSG, file,
	     errmsg(errno));
  }

  if (kf) {
    kf->count++;
    kf->entries = (kill_entry *) XtRealloc((char *)kf->entries,
					   kf->count *
					   sizeof(*kf->entries));
    memset(&kf->entries[kf->count-1], 0, sizeof(*kf->entries));
    parse_kill_entry(newsgroup, "internal", buf, kf,
		     &kf->entries[kf->count-1], mesg_name);
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