#include <X11/Intrinsic.h>
#include <assert.h>

#define ARTSTRUCT_C
#include "news.h"
#include "artstruct.h"

static Boolean artsame _ARGUMENTS((struct article *, struct article *));
static void freeartstruct _ARGUMENTS((struct article *));


/*
  Initialize a newsgroup structure from the point of view of the
  routines in this file.
  */
void artListInit(newsgroup)
    struct newsgroup *newsgroup;
{
    newsgroup->articles = newsgroup->ref_art = 0;
}


/*
  Make sure that the article list for a newsgroup exists.  Create it
  if it doesn't.
  */
void artListSet(newsgroup)
    struct newsgroup *newsgroup;
{
    struct article *old;

    if (newsgroup->articles)
	return;

    newsgroup->articles = (struct article *)
	XtCalloc(1, sizeof(struct article));
    newsgroup->articles->status = ART_CLEAR;
    newsgroup->articles->first = newsgroup->first;

    if (newsgroup->first > 1) {
	old = (struct article *)
	    XtCalloc(1, sizeof(struct article));
	old->first = 1;
	old->status = ART_CLEAR_READ;
	SET_UNAVAIL(old);
	old->next = newsgroup->articles;
	old->next->previous = old;
	newsgroup->articles = old;
    }
    newsgroup->ref_art = newsgroup->articles;
}


/*
  Free an article structure.
  */
static void freeartstruct(art)
    struct article *art;
{
    XtFree(art->subject);
    XtFree(art->author);
    XtFree(art->lines);
    CLEAR_FILE(art);
    XtFree((char *) art);
}


/*
  Free an article list for a newsgroup.
  */
void artListFree(newsgroup)
    struct newsgroup *newsgroup;
{
    struct article *next, *list = newsgroup->articles;

    while (list) {
	next = list->next;
	freeartstruct(list);
	list = next;
    }
    newsgroup->articles = newsgroup->ref_art = 0;
}

    
/*
  Get an article structure for a particular article in a newsgroup.

  The newsgroup article list must have been initialized previously
  (with artListSet(), followed by calls to populate it).

  The specified article must be between 1 and newsgroup->last.

  If "writeable" is true, than the returned structure can be modified
  by the caller.  Otherwise, it shouldn't be.
  */
struct article *artStructGet(newsgroup, artnum, writeable)
    struct newsgroup *newsgroup;
    art_num artnum;
    Boolean writeable;
{
    struct article *reference = newsgroup->ref_art;

    if (! reference)
	reference = newsgroup->articles;

    /*
      Find the structure containing the article we want.
      */

    while ((artnum < reference->first) ||
	   (reference->next && (reference->next->first <= artnum))) {
	if (artnum < reference->first)
	    reference = reference->previous;
	else
	    reference = reference->next;
    }

    if (! writeable) {
	newsgroup->ref_art = reference;
	return reference;
    }

#define COPY_FIELD(field) \
    if (reference->field) \
	new->field = XtNewString(reference->field);

    /*
      If there are articles in the structure after the one we want,
      create a new structure to hold them.
      */
    if ((reference->next && (reference->next->first > (artnum + 1))) ||
	(!reference->next && (artnum < newsgroup->last))) {
	struct article *new = (struct article *)
	    XtMalloc(sizeof(struct article));
	*new = *reference;
	COPY_FIELD(subject);
	COPY_FIELD(author);
	COPY_FIELD(lines);
	COPY_FIELD(filename);
	new->first = artnum + 1;
	new->previous = reference;
	if (reference->next)
	    reference->next->previous = new;
	reference->next = new;
    }

    /*
      If there are articles in the structure before the one we want,
      create a new structure for just the one we want.
      */
    if (reference->first < artnum) {
	struct article *new = (struct article *)
	    XtMalloc(sizeof(struct article));
	*new = *reference;
	COPY_FIELD(subject);
	COPY_FIELD(author);
	COPY_FIELD(lines);
	COPY_FIELD(filename);
	new->first = artnum;
	new->previous = reference;
	if (reference->next)
	    reference->next->previous = new;
	reference->next = new;
	reference = new;
    }

#undef COPY_FIELD

    newsgroup->ref_art = reference;
    return reference;
}


/*
  Given an article structure which was previously returned by
  artStructGet with "writeable" True and which may have been modified
  in the meantime, pack the article list containing the article
  structure as necessary.

  The "art" parameter is an input/output parameter; when the function
  returns, it will point at the current article structure for the
  article, but that structure should then be treated as read-only by
  the caller.
  */
void artStructSet(newsgroup, art)
    struct newsgroup *newsgroup;
    struct article **art;
{
    if (artsame(*art, (*art)->previous)) {
	struct article *tmp = *art;
	(*art)->previous->next = (*art)->next;
	if ((*art)->next)
	    (*art)->next->previous = (*art)->previous;
	*art = (*art)->previous;
	XtFree((char *) tmp);
    }

    if (artsame(*art, (*art)->next)) {
	struct article *tmp = (*art)->next;
	(*art)->next->first = (*art)->first;
	if ((*art)->previous)
	    (*art)->previous->next = tmp;
	else
	    newsgroup->articles = tmp;
	(*art)->next->previous = (*art)->previous;
	XtFree((char *) *art);
	*art = tmp;
    }

    newsgroup->ref_art = *art;
}


/*
  Like artStructSet, but an original article structure, which may have
  been retrieved with "writeable" False, and a copy are specified.  
  Only actually does a replacement if something was changed.

  The "original" parameter is input/output and will contain a
  read-only pointer to the structure for the article when the function
  returns.
  */
void artStructReplace(newsgroup, original, copy, artnum)
    struct newsgroup *newsgroup;
    struct article **original;
    struct article *copy;
    art_num artnum;
{
    if (! artsame(*original, copy)) {
	*original = artStructGet(newsgroup, artnum, True);
	(*original)->status = copy->status;
#define CHECK(field) \
	if ((*original)->field != copy->field) { \
	    XtFree((*original)->field); \
	    (*original)->field = copy->field; \
	}
	CHECK(subject);
	CHECK(author);
	CHECK(lines);
	if ((*original)->filename != copy->filename) {
	    CLEAR_FILE(*original);
	    CHECK(filename);
	}
#undef CHECK
	artStructSet(newsgroup, original);
    }
}


/*
  Return the next article structure in an article list, as well as the
  first and last article numbers it represents.  Returns null when
  there are no more.

  The structure that's returned is read-only.
  */
struct article *artStructNext(newsgroup, art, first, last)
    struct newsgroup *newsgroup;
    struct article *art;
    art_num *first, *last;
{
    art = art->next;
    if (! art) {
	newsgroup->ref_art = 0;
	return 0;
    }

    if (first)
	if (art->first)
	    *first = art->first;
	else
	    *first = newsgroup->first;

    if (last)
	if (art->next)
	    *last = art->next->first - 1;
	else
	    *last = newsgroup->last;

    newsgroup->ref_art = art;
    return art;
}


/*
  Like artStructNext(), but returns the previous article structure
  instead of the next one.
  */
struct article *artStructPrevious(newsgroup, art, first, last)
    struct newsgroup *newsgroup;
    struct article *art;
    art_num *first, *last;
{
    art = art->previous;
    if (! art) {
	newsgroup->ref_art = 0;
	return 0;
    }

    if (first)
	if (art->first)
	    *first = art->first;
	else
	    *first = newsgroup->first;

    if (last)
	if (art->next)
	    *last = art->next->first - 1;
	else
	    *last = newsgroup->last;

    newsgroup->ref_art = art;
    return art;
}


/*
  Returns the first article structure in the article list for a
  newsgroup, as well as the first and last article numbers it
  represents.
  */
struct article *artListFirst(newsgroup, first, last)
    struct newsgroup *newsgroup;
    art_num *first, *last;
{
    struct article *art = newsgroup->articles;

    if (! art) {
	newsgroup->ref_art = 0;
	return 0;
    }

    if (first)
	*first = art->first;

    if (last)
	if (art->next)
	    *last = art->next->first - 1;
	else
	    *last = newsgroup->last;

    newsgroup->ref_art = art;
    return art;
}


/*
  Returns the last article structure in the article list for a
  newsgroup.
  */
struct article *artListLast(newsgroup, first, last)
    struct newsgroup *newsgroup;
    art_num *first, *last;
{
    struct article *art = newsgroup->articles;

    if (! art) {
	newsgroup->ref_art = 0;
	return 0;
    }

    while (art->next)
	art = art->next;

    if (first)
	if (art->first)
	    *first = art->first;
	else
	    *first = newsgroup->first;

    if (last)
	*last = newsgroup->last;

    newsgroup->ref_art = 0;
    return art;
}


/*
  Compare two article structures, returning True if they are equal in
  everything but pointers and ranges and False otherwise.  Returns
  False if either structure pointer is null.
  */
static Boolean artsame(art1, art2)
    struct article *art1, *art2;
{
    if (! (art1 && art2))
	return False;

    if (art1->status != art2->status)
	return False;

#define ARTSTRCMP(field) \
    if (art1->field) \
	if (art2->field) { \
	    if (strcmp(art1->field, art2->field)) \
		return False; \
	} \
	else \
	    return False; \
    else if (art2->field) \
	return False;

    ARTSTRCMP(subject);
    ARTSTRCMP(author);
    ARTSTRCMP(lines);
    ARTSTRCMP(filename);

#undef ARTSTRCMP

    return True;
}

