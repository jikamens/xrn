#include <X11/Intrinsic.h>
#include <X11/Xfuncs.h>
#include <assert.h>

#include "snapshot.h"
#include "newsrcfile.h"
#include "news.h"
#include "utils.h"

static char *	group;
static art_num	first;
static art_num	last;
static char *	statuses;

/*
  Save a snapshot of which articles are read and unread in the
  specified newsgroup.  Assumes that the articles array in the
  newsgroup is allocated and valid.
  */
void groupSnapshotSave(newsgroup)
    struct newsgroup *newsgroup;
{
    int i, byte, bit, mask;

    FREE(group);
    FREE(statuses);

    group = XtNewString(newsgroup->name);
    first = newsgroup->first;
    last = newsgroup->last;
    /* Use XtCalloc instead of XtMalloc because it zeroes. */
    statuses = XtCalloc(1, (last - first + 1) / 8 + 1);

    for (i = first; i <= last; i++) {
	byte = (i - first) / 8;
	bit = (i - first) % 8;
	mask = 1 << bit;
	if (IS_READ(newsgroup->articles[INDEX(i)]))
	    statuses[byte] |= mask;
    }
}

/*
  Free a previously saved snapshot of the specified newsgroup, if
  there is one.
  */
void groupSnapshotFree(newsgroup)
    struct newsgroup *newsgroup;
{
    if ((! group) || strcmp(group, newsgroup->name))
	return;

    FREE(group);
    FREE(statuses);
}

/*
  Restore the snapshot of read and unread articles in the specified
  newsgroup.  There *must be* a current snapshot when this function is
  called; the newsgroup specified *must be* the newsgroup of that
  snapshot; the first and last article numbers in the newsgroup *must
  not* be different from when the snapshot was taken.
  */
void groupSnapshotRestore(newsgroup)
    struct newsgroup *newsgroup;
{
    int i, byte, bit, mask;

    assert(group && !strcmp(group, newsgroup->name) &&
	   (first == newsgroup->first) && (last == newsgroup->last));

    for (i = first; i <= last; i++) {
	byte = (i - first) / 8;
	bit = (i - first) % 8;
	mask = 1 << bit;
	if (statuses[byte] & mask)
	    SET_READ(newsgroup->articles[INDEX(i)]);
	else
	    SET_UNREAD(newsgroup->articles[INDEX(i)]);
    }

    groupSnapshotFree(newsgroup);
}
