#ifndef _KILLFILE_H
#define _KILLFILE_H

#define MAX_KILL_ENTRY_LENGTH	512
#define MAX_KILL_PATTERN_LENGTH (MAX_KILL_ENTRY_LENGTH- \
				 1-			/* first slash */ \
				 1-			/* second slash */ \
				 1-			/* "h" option */ \
				 20-			/* "t" option and value */ \
				 20-			/* "u" option and value */ \
				 1-			/* colon */ \
				 1-			/* command */ \
				 2)			/* newline and null */
#define MAX_KILL_PATTERN_VALUE_LENGTH (MAX_KILL_PATTERN_LENGTH- \
				       13) /* "^Newsgroups: " */

#define KILL_SESSION	-1
#define KILL_GLOBAL	0
#define KILL_LOCAL	1

#define KILL_ENTRY	(1<<0)
#define KILL_INCLUDE	(1<<1)
#define KILL_OTHER	(1<<2)

#define KILL_SUBJECT 	(1<<0)
#define KILL_AUTHOR  	(1<<1)
#define KILL_NEWSGROUPS	(1<<2)
#define KILL_DATE	(1<<3)
#define KILL_ID		(1<<4)
#define KILL_REFERENCES	(1<<5)
#define KILL_XREF	(1<<6)

#define KILL_JUNK (1<<0)
#define KILL_MARK (1<<1)
#define KILL_SAVE (1<<2)

typedef union _kill_entry  {
  char type;
  struct {
    char type;
    char *value;
  } any;
  struct {
    char type;
    char *value;
    char *pattern;
#ifdef POSIX_REGEX
    regex_t reStruct;
#else
    char *reStruct;
#endif /* POSIX_REGEX */
    char check_flags;
    char action_flags;
    int timeout;
    time_t last_used;
  } entry;
  struct {
    char type;
    char *value;
    char *operand;
    char is_ngfile;
    struct _kill_file *kf;
  } include;
  struct {
    char type;
    char *value;
  } other;
} kill_entry;

typedef struct _kill_file {
  time_t mod_time;
  char *file_name;
  art_num thru;
  int count;
  kill_entry *entries;
  struct _kill_file *cur_sub_kf;
  kill_entry *cur_entry;
  char flags;
} kill_file;

#define KF_SEEN		(1<<0)
#define KF_CHANGED	(1<<1)

void read_global_kill_file _ARGUMENTS((struct newsgroup *));
void read_local_kill_file _ARGUMENTS((struct newsgroup *));
kill_entry *kill_file_iter _ARGUMENTS((struct newsgroup *, int mode,
				       kill_entry *));
void write_kill_file _ARGUMENTS((struct newsgroup *, int mode));
Boolean has_kill_files _ARGUMENTS((struct newsgroup *));
void add_kill_entry _ARGUMENTS((struct newsgroup *newsgroup, int mode,
				char *field, char *regexp));
void kill_update_last_used _ARGUMENTS((kill_file *, kill_entry *));

#endif /* ! _KILLFILE_H */
