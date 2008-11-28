#ifndef _HASH_H
#define _HASH_H

#ifndef _ARGUMENTS
#ifdef __STDC__
#define _ARGUMENTS(a) a
#else
#define _ARGUMENTS(a) ()
#endif
#endif

typedef int (*hash_calc_func) _ARGUMENTS((int, int, void *));
typedef int (*hash_compare_func) _ARGUMENTS((void *, void *));
typedef void (*hash_free_func) _ARGUMENTS((void *));

typedef void *hash_table_object;

hash_table_object hash_table_create
	_ARGUMENTS((int, hash_calc_func,
		    hash_compare_func, hash_compare_func,
		    hash_free_func, hash_free_func));

int hash_table_insert _ARGUMENTS((hash_table_object, void *, void *, int));

void *hash_table_retrieve _ARGUMENTS((hash_table_object, void *, void **));

void hash_table_delete _ARGUMENTS((hash_table_object, void *, void *));

void hash_table_destroy _ARGUMENTS((hash_table_object));

int hash_string_calc _ARGUMENTS((int, int, void *));
int hash_string_compare _ARGUMENTS((void *, void *));

int hash_int_calc _ARGUMENTS((int, int, void *));
int hash_int_compare _ARGUMENTS((void *, void *));

#ifdef DEBUG
void hash_print_stats _ARGUMENTS((hash_table_object));

#define HASH_PRINT_HASHES	(1<<0)
#endif

#define HASH_NO_VALUE (void *)-1

#endif /* _HASH_H */
