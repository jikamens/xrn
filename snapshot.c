/*
 * xrn - an X-based NNTP news reader
 *
 * Copyright (c) 1994-2023, Jonathan Kamens.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of California not
 * be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The University
 * of California makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE UNIVERSITY OF CALIFORNIA DISCLAIMS ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <X11/Intrinsic.h>
#if XtSpecificationRelease > 4	/* X11R4 didn't have this? */
#include <X11/Xfuncs.h>
#endif
#include <assert.h>

#include "config.h"
#include "snapshot.h"
#include "newsrcfile.h"
#include "news.h"
#include "artstruct.h"
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
    struct article *art;

    FREE(group);
    FREE(statuses);

    group = XtNewString(newsgroup->name);
    first = newsgroup->first;
    last = newsgroup->last;
    /* Use XtCalloc instead of XtMalloc because it zeroes. */
    statuses = XtCalloc(1, (last - first + 1) / 8 + 1);

    for (i = first; i <= last; i++) {
	art = artStructGet(newsgroup, i, False);
	byte = (i - first) / 8;
	bit = (i - first) % 8;
	mask = 1 << bit;
	if (IS_READ(art))
	    statuses[byte] |= mask;
	ART_STRUCT_UNLOCK;
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
    struct article *art, copy;

    assert(group && !strcmp(group, newsgroup->name) && (first == newsgroup->first) && (last == newsgroup->last));

    for (i = first; i <= last; i++) {
	art = artStructGet(newsgroup, i, False);
	copy = *art;
	byte = (i - first) / 8;
	bit = (i - first) % 8;
	mask = 1 << bit;
	if (statuses[byte] & mask)
	    SET_READ(&copy);
	else
	    SET_UNREAD(&copy);
	artStructReplace(newsgroup, &art, &copy, i);
    }

    groupSnapshotFree(newsgroup);
}
