#ifndef _ACTIVECACHE_H_
#define _ACTIVECACHE_H_

#include "utils.h"
#include "news.h"

extern void active_cache_read _ARGUMENTS((char *));
extern int active_cache_write _ARGUMENTS((char *, struct newsgroup **,
					  /* ng_num */ int, /* Boolean */ int));

#endif /* _ACTIVECACHE_H_ */
