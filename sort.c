/*

  This file contains routines for sorting a list of articles from a
  newsgroup, and for producting a textual representation of the sorted
  article list for displaying in the newsgroup index.

  The method for using the routines in this file is as follows:

  1) Call art_sort_init() to initialize a sort and get back an opaque
  object for future sort manipulations.

  2) Call the various art_sort_by_*() to do the sorts.  These routines
  should be called from minor to major sorting order.  That is, if you
  want the articles to be sorted by thread, and within thread by date,
  you would call sort_by_date() first and then sort_by_thread().

  3) Call art_sort_done() to free the opaque object and get back a
  string representing the finished product of the sort.

  You can also call art_sort_cancel() at any time after
  art_sort_init() to abort a sort and free the memory associated with
  it without getting back the string subject list.

  The newsgroup must be prefetched completely before sort_init() is
  called.

  No other manipulation of the newsgroup's article list should be done
  while sorting is being done.
  */

#include <ctype.h>
#include <assert.h>

#include "config.h"
#include "utils.h"
#include "news.h"
#include "sort.h"
#include "artstruct.h"
#include "internals.h"
#include "mesg.h"
#include "mesg_strings.h"
#include "hash.h"
#include "resources.h"

extern time_t get_date _ARGUMENTS((char *));

#define SUB_SORT_WIDTH 24


struct sort_article {
  void *sort_key;
  art_num num;
  struct article *art;
};

struct sort_data {
  struct newsgroup *newsgroup;
  art_num count;
  struct sort_article *articles;
  void (*last_sort) _ARGUMENTS((void *));
};


static int (*compare_function) _ARGUMENTS((const void *, const void *));
static void (*sort_list[3]) _ARGUMENTS((void *));

void art_sort_parse_sortlist(orig_sort_spec)
     char *orig_sort_spec;
{
  char *token;
  int num_sorts = (sizeof(sort_list)/sizeof(sort_list[0]));
  int i = num_sorts - 1;
  char *sort_spec;

  if (!orig_sort_spec)
    return;

  sort_spec = orig_sort_spec = XtNewString(orig_sort_spec);

  while ((token = strtok(sort_spec, ", \t\n")) && (i >= 0)) {
    sort_spec = 0;
    if (! strcasecmp(token, "false") ||
	! strcasecmp(token, "off") ||
	! strcasecmp(token, "0")) {
      for (i = 0; i < num_sorts; i++)
	sort_list[i] = 0;
      XtFree(orig_sort_spec);
      return;
    }
    else if (! strcasecmp(token, "subject") ||
	     ! strcasecmp(token, "true") ||
	     ! strcasecmp(token, "on") ||
	     ! strcasecmp(token, "1"))
      sort_list[i--] = art_sort_by_subject;
    else if (! strcasecmp(token, "date"))
      sort_list[i--] = art_sort_by_date;
    else if (! strcasecmp(token, "thread"))
      sort_list[i--] = art_sort_by_thread;
    else
      mesgPane(XRN_SERIOUS, 0, UNKNOWN_SORT_TYPE_MSG, token);
  }

  if (token)
    mesgPane(XRN_SERIOUS, 0, TOO_MANY_SORT_TYPES_MSG);

  XtFree(orig_sort_spec);
  return;
}

int art_sort_need_dates()
{
  return((sort_list[0] == art_sort_by_date) ||
	 (sort_list[1] == art_sort_by_date) ||
	 (sort_list[2] == art_sort_by_date));
}


int art_sort_need_threads()
{
  return((sort_list[0] == art_sort_by_thread) ||
	 (sort_list[1] == art_sort_by_thread) ||
	 (sort_list[2] == art_sort_by_thread));
}


void *art_sort_init(
		    _ANSIDECL(struct newsgroup *,	newsgroup),
		    _ANSIDECL(art_num,			first),
		    _ANSIDECL(art_num,			last)
		    )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			first)
     _KNRDECL(art_num,			last)
{
  struct sort_data *data = (struct sort_data *)
    XtMalloc(sizeof(struct sort_data));
  art_num i, count;

  data->newsgroup = newsgroup;

  /*
    Figure out how many articles there are.
    */
  for (i = first, count = 0; i <= last; i++) {
    struct article *art = artStructGet(newsgroup, i, False);
    if (IS_LISTED(art))
      count++;
    ART_STRUCT_UNLOCK;
  }

  data->articles = (struct sort_article *)
    XtCalloc(count, sizeof(*data->articles));
  data->count = count;
  data->last_sort = 0;

  for (i = first, count = 0; i <= last; i++) {
    struct article *art = artStructGet(newsgroup, i, False);
    if (IS_LISTED(art)) {
      data->articles[count].num = i;
      data->articles[count++].art = art;
    }
    ART_STRUCT_UNLOCK;
  }

  return (void *) data;
}



char *art_sort_done(data_p, line_length)
     void *data_p;
     int line_length;
{
  struct sort_data *data = (struct sort_data *) data_p;
  int sub_width = subjectIndexLine(line_length, 0, data->newsgroup, 0,
				   data->last_sort == art_sort_by_thread);
  char *out_data = XtMalloc(sub_width * data->count + 1), *ptr = out_data;
  art_num i;

  for (i = 0; i < data->count; i++) {
    (void) subjectIndexLine(line_length, ptr, data->newsgroup,
			    data->articles[i].num,
			    art_sort_need_threads());
    ptr += sub_width;
  }

  if (! data->count)
    *ptr = '\0';

  art_sort_cancel(data_p);

  return out_data;
}



void art_sort_cancel(data_p)
     void *data_p;
{
  struct sort_data *data = (struct sort_data *) data_p;

  XtFree((char *) data->articles);
  XtFree((char *) data);
}



char *art_sort_doit(
		    _ANSIDECL(struct newsgroup *,	newsgroup),
		    _ANSIDECL(art_num,			first),
		    _ANSIDECL(art_num,			last),
		    _ANSIDECL(int,			line_length)
		    )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			first)
     _KNRDECL(art_num,			last)
     _KNRDECL(int,			line_length)
{
  void *data_p;
  int i, num_sorts = sizeof(sort_list)/sizeof(sort_list[0]);
  
  data_p = art_sort_init(newsgroup, first, last);
  for (i = 0; i < num_sorts; i++)
    if (sort_list[i])
      (*sort_list[i])(data_p);
  return art_sort_done(data_p, line_length);
}
   

static int key_compare _ARGUMENTS((const void *, const void *));

static int key_compare(a_p, b_p)
     const void *a_p, *b_p;
{
  struct sort_article *a = (struct sort_article *) a_p;
  struct sort_article *b = (struct sort_article *) b_p;
  return (*compare_function)(a->sort_key, b->sort_key);
}


static void generate_subject_keys _ARGUMENTS((struct sort_data *));

static void generate_subject_keys(data)
     struct sort_data *data;
{
  art_num i;
  char *ptr;

  for (i = 0; i < data->count; i++) {
    data->articles[i].sort_key = (void *) XtMalloc(SUB_SORT_WIDTH + 1);
    (void) strncpy((char *) data->articles[i].sort_key,
		   subjectStrip(data->articles[i].art->subject),
		   SUB_SORT_WIDTH);
    ((char *)data->articles[i].sort_key)[SUB_SORT_WIDTH] = '\0';
    for (ptr = data->articles[i].sort_key; *ptr; ptr++)
      if (isupper((unsigned char)*ptr))
	*ptr = tolower(*ptr);
  }
}

int subject_value_compare(key1, key2)
     void *key1, *key2;
{
  return (struct sort_article *)key1 - (struct sort_article *)key2;
}

static void free_subject_keys _ARGUMENTS((struct sort_data *));

static void free_subject_keys(data)
     struct sort_data *data;
{
  art_num i;

  for (i = 0; i < data->count; i++) {
    XtFree((char *) data->articles[i].sort_key);
  }
}

void art_sort_by_subject(data_p)
     void *data_p;
{
  struct sort_data *data = (struct sort_data *) data_p;
  int i, tmp_pos = 0;
  struct sort_article *tmp_articles;
  hash_table_object hash_table;
  void *hash_reference;
  struct sort_article *hash_return;
  
  tmp_articles = (struct sort_article *)
    XtCalloc(data->count, sizeof(*tmp_articles));

  generate_subject_keys(data);

  hash_table = hash_table_create(data->count,
				 hash_string_calc,
				 hash_string_compare, subject_value_compare,
				 0, 0);

  for (i = 0; i < data->count; i++) {
    hash_table_insert(hash_table, (void *)data->articles[i].sort_key,
		      (void *)&data->articles[i], 0);
  }

  for (i = 0; i < data->count; i++) {
    hash_reference = HASH_NO_VALUE;
    if ((void *)(hash_return =
	 hash_table_retrieve(hash_table,
			     (void *)data->articles[i].sort_key,
			     &hash_reference)) == HASH_NO_VALUE) {
      continue;
    }
    do {
      tmp_articles[tmp_pos++] = *hash_return;
    } while ((hash_return = (struct sort_article *)
	      hash_table_retrieve(hash_table,
				  (void *)data->articles[i].sort_key,
				  &hash_reference)) !=
	     (struct sort_article *)HASH_NO_VALUE);

    hash_table_delete(hash_table,
		      (void *)data->articles[i].sort_key,
		      HASH_NO_VALUE);
  }

  XtFree((char *)data->articles);
  data->articles = tmp_articles;

  hash_table_destroy(hash_table);

  free_subject_keys(data);

  data->last_sort = art_sort_by_subject;
}



void generate_date_keys(data)
     struct sort_data *data;
{
  art_num i;

  for (i = 0; i < data->count; i++) {
    time_t date = get_date(data->articles[i].art->date);
    if (date == (time_t)-1) {
      if (app_resources.complainAboutBadDates)
	mesgPane(XRN_SERIOUS, 0, UNPARSEABLE_DATE_MSG,
		 data->articles[i].num, data->newsgroup->name,
		 data->articles[i].art->date);
      date = (time_t) 0;
    }
    data->articles[i].sort_key = (void *)date;
  }
}

int date_compare(a, b)
     const void *a, *b;
{
  /*
    Casting to char * because pointer arithmetic on void * causes a
    warning.  Casting to a pointer instead of integer value because
    there are some architectures on which pointers are larger than
    integers.
    */
  return (char *)a - (char *)b;
}

      
void art_sort_by_date(data_p)
     void *data_p;
{
  struct sort_data *data = (struct sort_data *) data_p;
  
  generate_date_keys(data);

  compare_function = date_compare;
  qsort((void *)data->articles, data->count, sizeof(*data->articles),
	key_compare);
  data->last_sort = art_sort_by_date;
}

static void do_art_thread _ARGUMENTS((struct sort_data *,
				      hash_table_object,
				      art_num,
				      struct sort_article *,
				      int *));

static void do_art_thread(data, table, this_art, artlist, artpos)
     struct sort_data *data;
     hash_table_object table;
     art_num this_art;
     struct sort_article *artlist;
     int *artpos;
{
  int i;
  art_num *ptr;

  i = (POINTER_NUM_TYPE)hash_table_retrieve(table, (void *)this_art, 0);
  assert(i != (POINTER_NUM_TYPE)HASH_NO_VALUE);

  /* Circular article references are bogus, but unfortunately possible */
  if (! data->articles[i].sort_key)
    return;

  artlist[(*artpos)++] = data->articles[i];
  data->articles[i].sort_key = (void *)0;

  if (! data->articles[i].art->children)
    return;

  for (ptr = data->articles[i].art->children; *ptr; ptr++) {
    do_art_thread(data, table, *ptr, artlist, artpos);
  }
}
    
void art_sort_by_thread(data_p)
     void *data_p;
{
  struct sort_data *data = (struct sort_data *)data_p;
  struct article *art;
  POINTER_NUM_TYPE i, ret;
  hash_table_object table, done_table;
  struct sort_article *tmp_articles;
  int tmp_pos = 0;
  art_num this_art;

  tmp_articles = (struct sort_article *)
    XtCalloc(data->count, sizeof(*tmp_articles));

  table = hash_table_create(data->count, hash_int_calc,
			    hash_int_compare, hash_int_compare,
			    0, 0);
  done_table = hash_table_create(data->count, hash_int_calc,
				 hash_int_compare, hash_int_compare, 0, 0);

  for (i = 0; i < data->count; i++) {
    int ret;

    data->articles[i].sort_key = (void *)1;
    ret = hash_table_insert(table, (void *) data->articles[i].num,
			    (void *) i, 1);
    assert(ret);
  }

  for (i = 0; i < data->count; i++) {
    if (! data->articles[i].sort_key)
      continue;
    art = data->articles[i].art;
    this_art = data->articles[i].num;
    /* We are keeping track of which articles we've already done so
       that we can detect (and avoid) circular "References"
       dependencies. */
    ret = hash_table_insert(done_table, (void *)this_art, (void *)1, 1);
    assert(ret);
    while (art->parent) {
      if (! hash_table_insert(done_table, (void *) art->parent, (void *)1, 1))
	break;
      this_art = art->parent;
      art = artStructGet(data->newsgroup, this_art, False);
      ART_STRUCT_UNLOCK;
      assert(art);
    }
    do_art_thread(data, table, this_art, tmp_articles, &tmp_pos);
  }

  hash_table_destroy(table);
  hash_table_destroy(done_table);
  XtFree((char *)data->articles);
  data->articles = tmp_articles;
  data->last_sort = art_sort_by_thread;
}
