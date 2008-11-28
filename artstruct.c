#include <X11/Intrinsic.h>
#include <assert.h>

#define ARTSTRUCT_C
#include "config.h"
#include "news.h"
#include "artstruct.h"
#include "file_cache.h"

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


int artStructNumChildren(art)
     struct article *art;
{
  int i;
  art_num *ptr;

  if (! art->children)
    return 0;

  for (ptr = art->children, i = 0; *ptr; ptr++)
    i++;

  return i;
}

void artStructAddChild(art, child)
     struct article *art;
     art_num child;
{
  int i = artStructNumChildren(art);

  art->children = (art_num *) XtRealloc((char *) art->children,
					(i + 2) * sizeof(*art->children));

  art->children[i++] = child;
  art->children[i] = 0;
}

void artStructRemoveChild(art, child)
     struct article *art;
     art_num child;
{
  art_num *ptr, *ptr2;

  if (! art->children)
    return;

  for (ptr = art->children; *ptr; ptr++) {
    if (*ptr == child) {
      for (ptr2 = ptr + 1; *ptr2; ptr++, ptr2++)
	*ptr = *ptr2;
      *ptr = 0;
    }
  }

  return;
}
      
  
static art_num *copy_art_list _ARGUMENTS((struct article *));

static art_num *copy_art_list(art)
     struct article *art;
{
  int i;
  art_num *new;

  if (! art->children)
    return 0;

  i = artStructNumChildren(art);

  new = (art_num *) XtCalloc(i + 1, sizeof(*new));

  (void) memcpy((char *) new, (char *) art->children, (i + 1) * sizeof(*new));

  return new;
}

/*
  Free an article structure.
  */
static void freeartstruct(art)
    struct article *art;
{
  CLEAR_ALL(art);
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
struct article *artStructGet(
			     _ANSIDECL(struct newsgroup *,	newsgroup),
			     _ANSIDECL(art_num,			artnum),
			     _ANSIDECL(Boolean,			writeable)
			     )
     _KNRDECL(struct newsgroup *,	newsgroup)
     _KNRDECL(art_num,			artnum)
     _KNRDECL(Boolean,			writeable)
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
	COPY_FIELD(from);
	COPY_FIELD(author);
	COPY_FIELD(lines);
	COPY_FIELD(newsgroups);
	COPY_FIELD(date);
	COPY_FIELD(id);
	COPY_FIELD(xref);
	COPY_FIELD(references);
	new->file = reference->file;
	new->base_file = reference->base_file;
	new->parent = reference->parent;
	new->children = copy_art_list(reference);
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
	COPY_FIELD(from);
	COPY_FIELD(author);
	COPY_FIELD(lines);
	COPY_FIELD(newsgroups);
	COPY_FIELD(date);
	COPY_FIELD(id);
	COPY_FIELD(xref);
	COPY_FIELD(references);
	new->file = reference->file;
	new->base_file = reference->base_file;
	new->parent = reference->parent;
	new->children = copy_art_list(reference);
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


#define CHECK(field, staying, going) \
	if ((staying)->field != (going)->field) { \
	    XtFree((char *)(going)->field); \
	    (going)->field = (staying)->field; \
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

      CHECK(subject, (*art)->previous, *art);
      CHECK(from, (*art)->previous, *art);
      CHECK(author, (*art)->previous, *art);
      CHECK(lines, (*art)->previous, *art);
      CHECK(newsgroups, (*art)->previous, *art);
      CHECK(date, (*art)->previous, *art);
      CHECK(children, (*art)->previous, *art);
      /* "file" doesn't need checking because artsame() says it's the
	 same in both structures. */
      CHECK(id, (*art)->previous, *art);
      CHECK(xref, (*art)->previous, *art);
      CHECK(references, (*art)->previous, *art);

      (*art)->previous->next = (*art)->next;
      if ((*art)->next)
	(*art)->next->previous = (*art)->previous;
      *art = (*art)->previous;
      XtFree((char *) tmp);
    }

    if (artsame(*art, (*art)->next)) {
      struct article *tmp = (*art)->next;

      CHECK(subject, (*art)->next, *art);
      CHECK(from, (*art)->next, *art);
      CHECK(author, (*art)->next, *art);
      CHECK(lines, (*art)->next, *art);
      CHECK(newsgroups, (*art)->next, *art);
      CHECK(date, (*art)->next, *art);
      CHECK(children, (*art)->next, *art);
      /* "file" doesn't need checking because artsame() says it's the
	 same in both structures. */
      CHECK(id, (*art)->next, *art);
      CHECK(xref, (*art)->next, *art);
      CHECK(references, (*art)->next, *art);

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
	CHECK(subject, copy, *original);
	CHECK(from, copy, *original);
	CHECK(author, copy, *original);
	CHECK(lines, copy, *original);
	CHECK(newsgroups, copy, *original);
	CHECK(date, copy, *original);
	(*original)->parent = copy->parent;
	CHECK(children, copy, *original);
	if ((*original)->file != copy->file) {
	    CLEAR_FILE(*original);
	    CHECK(file, copy, *original);
	}
	if ((*original)->base_file != copy->base_file) {
	    CLEAR_BASE_FILE(*original);
	    CHECK(base_file, copy, *original);
	}
	CHECK(id, copy, *original);
	CHECK(xref, copy, *original);
	CHECK(references, copy, *original);
	artStructSet(newsgroup, original);
    }
}

#undef CHECK

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
    int num_children;

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
    ARTSTRCMP(from);
    ARTSTRCMP(author);
    ARTSTRCMP(lines);
    ARTSTRCMP(newsgroups);
    ARTSTRCMP(date);
    ARTSTRCMP(id);
    ARTSTRCMP(xref);
    ARTSTRCMP(references);

    if (art1->file != art2->file)
      return False;

    if (art1->base_file != art2->base_file)
      return False;

    if (art1->parent != art2->parent)
      return False;

    if ((art1->children != art2->children) ||
	((num_children = artStructNumChildren(art1)) !=
	 artStructNumChildren(art2)) ||
	(art1->children &&
	 memcmp((void *)art1->children, (void *)art2->children,
		(num_children + 1) * sizeof(*art1->children))))
      return False;

#undef ARTSTRCMP

    return True;
}
