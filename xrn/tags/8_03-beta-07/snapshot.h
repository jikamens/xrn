#ifndef _SNAPSHOT_H
#define _SNAPSHOT_H

#include "news.h"

extern void groupSnapshotSave _ARGUMENTS((struct newsgroup *));
extern void groupSnapshotFree _ARGUMENTS((struct newsgroup *));
extern void groupSnapshotRestore _ARGUMENTS((struct newsgroup *));

#endif /* _SNAPSHOT_H */
