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

#include "config.h"
#include "utils.h"
#include "news.h"
#include "sort.h"
#include "artstruct.h"
#include "internals.h"
#include "getdate.h"
#include "mesg.h"
#include "mesg_strings.h"

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
};


static int (*compare_function) _ARGUMENTS((qsort_arg_type, qsort_arg_type));
static void (*sort_list[2]) _ARGUMENTS((void *));

void art_sort_parse_sortlist(sort_spec)
     char *sort_spec;
{
  char *token;
  int num_sorts = (sizeof(sort_list)/sizeof(sort_list[0]));
  int i = num_sorts - 1;
  

  if (!sort_spec)
    return;

  while ((token = strtok(sort_spec, ", \t\n")) && (i >= 0)) {
    sort_spec = 0;
    if (! strcasecmp(token, "false") ||
	! strcasecmp(token, "off") ||
	! strcasecmp(token, "0")) {
      for (i = 0; i < num_sorts; i++)
	sort_list[i] = 0;
      return;
    }
    else if (! strcasecmp(token, "subject") ||
	     ! strcasecmp(token, "true") ||
	     ! strcasecmp(token, "on") ||
	     ! strcasecmp(token, "1"))
      sort_list[i--] = art_sort_by_subject;
    else if (! strcasecmp(token, "date"))
      sort_list[i--] = art_sort_by_date;
    else
      mesgPane(XRN_SERIOUS, 0, UNKNOWN_SORT_TYPE_MSG, token);
  }

  if (token)
    mesgPane(XRN_SERIOUS, 0, TOO_MANY_SORT_TYPES_MSG);

  return;
}

int art_sort_need_dates()
{
  return((sort_list[0] == art_sort_by_date) ||
	 (sort_list[1] == art_sort_by_date));
}


void *art_sort_init(newsgroup, first, last, unread_only)
     struct newsgroup *newsgroup;
     art_num first, last;
     unsigned char unread_only;
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
    if (IS_AVAIL(art) && (!unread_only || IS_UNREAD(art)) &&
	art->subject)
      count++;
  }

  data->articles = (struct sort_article *)
    XtCalloc(count, sizeof(*data->articles));
  data->count = count;
  
  for (i = first, count = 0; i <= last; i++) {
    struct article *art = artStructGet(newsgroup, i, False);
    if (IS_AVAIL(art) && (!unread_only || IS_UNREAD(art)) &&
	art->subject) {
      data->articles[count].num = i;
      data->articles[count++].art = art;
    }
  }

  return (void *) data;
}



char *art_sort_done(data_p, line_length)
     void *data_p;
     int line_length;
{
  struct sort_data *data = (struct sort_data *) data_p;
  int sub_width = subjectIndexLine(line_length, 0, data->newsgroup, 0);
  char *out_data = XtMalloc(sub_width * data->count + 1), *ptr = out_data;
  art_num i;

  for (i = 0; i < data->count; i++) {
    (void) subjectIndexLine(line_length, ptr, data->newsgroup,
			    data->articles[i].num);
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



char *art_sort_doit(newsgroup, first, last, unread_only, line_length)
     struct newsgroup *newsgroup;
     art_num first, last;
     unsigned char unread_only;
     int line_length;
{
  void *data_p;
  int i, num_sorts = sizeof(sort_list)/sizeof(sort_list[0]);
  
  data_p = art_sort_init(newsgroup, first, last, unread_only);
  for (i = 0; i < num_sorts; i++)
    if (sort_list[i])
      (*sort_list[i])(data_p);
  return art_sort_done(data_p, line_length);
}
   


static int key_compare(a_p, b_p)
     qsort_arg_type a_p, b_p;
{
  struct sort_article *a = (struct sort_article *) a_p;
  struct sort_article *b = (struct sort_article *) b_p;
  return (*compare_function)(a->sort_key, b->sort_key);
}



static void generate_subject_keys(data)
     struct sort_data *data;
{
  art_num i;

  for (i = 0; i < data->count; i++) {
    data->articles[i].sort_key = (void *) XtMalloc(SUB_SORT_WIDTH + 1);
    (void) strncpy((char *) data->articles[i].sort_key,
		   subjectStrip(data->articles[i].art->subject),
		   SUB_SORT_WIDTH);
    ((char *)data->articles[i].sort_key)[SUB_SORT_WIDTH] = '\0';
  }
}

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
  int i, j, tmp_pos = 0;
  struct sort_article *tmp_articles;

  tmp_articles = (struct sort_article *)
    XtCalloc(data->count, sizeof(*tmp_articles));

  generate_subject_keys(data);

  for (i = 0; i < data->count; i++) {
    if (! data->articles[i].sort_key)
      continue;
    tmp_articles[tmp_pos] = data->articles[i];
    data->articles[i].sort_key = 0;
    for (j = i + 1; j < data->count; j++) {
      if (data->articles[j].sort_key &&
	  ! strcasecmp((char *)tmp_articles[tmp_pos].sort_key,
		       (char *)data->articles[j].sort_key)) {
	tmp_articles[++tmp_pos] = data->articles[j];
	data->articles[j].sort_key = 0;
      }
    }
    tmp_pos++;
  }

  XtFree((char *)data->articles);
  data->articles = tmp_articles;

  free_subject_keys(data);
}



void generate_date_keys(data)
     struct sort_data *data;
{
  art_num i;

  for (i = 0; i < data->count; i++) {
    time_t date = get_date(data->articles[i].art->date);
    if (date == (time_t)-1) {
      fprintf(stderr, "Unparseable date: %s\n",
	      data->articles[i].art->date);
      date = (time_t) 0;
    }
    data->articles[i].sort_key = (void *)date;
  }
}

int date_compare(a, b)
     qsort_arg_type a, b;
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
}
