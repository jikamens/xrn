#include <math.h>
#include <string.h>
#include <stdio.h>

#ifdef XRN
#include "config.h"
#include "utils.h"
#else
#include <malloc.h>
#endif

#ifndef _ARGUMENTS
#define _ARGUMENTS(a) a
#endif

#include "hash.h"

#ifdef DEBUG
static int hash_debug		= 0;
#endif

#define SIZE_FACTOR		2.0
/*
  This is the preferred number of bits to shift left after each
  character is XORed into the hash of a string.  However, it will be
  changed in the hash function if it is a factor of the number of bits
  in the table size.
  */
#define CHAR_SIG_BITS		4

#ifdef DEBUG
float SIZE_MULTIPLIER		= SIZE_FACTOR;
#else
#define SIZE_MULTIPLIER		SIZE_FACTOR
#endif

#ifdef XRN
#define HASH_MALLOC(foo)	XtMalloc(foo)
#define HASH_FREE(foo)		XtFree((char *)foo)
#define HASH_CALLOC(foo,bar)	XtCalloc((foo),(bar))
#define HASH_REALLOC(foo,bar)	XtRealloc((char *)(foo),(bar))
#else
#define HASH_MALLOC(foo)	malloc(foo)
#define HASH_FREE(foo)		free(foo)
#define HASH_CALLOC(foo,bar)	calloc((foo),(bar))
#define HASH_REALLOC(foo,bar)	realloc((foo),(bar))
#endif /* XRN */

#define HASH_ADDED 1
#define HASH_DELETED 2

#define NEXT_HASH(hash,size) (((hash)+1)%(size))
#define PREV_HASH(hash,size) (((hash)+(size)-1)%(size))

struct hash_entry {
  void *key;
  void *value;
  char status;
};

struct hash_table {
  int size, size_bits;
  hash_calc_func calc;
  hash_compare_func key_compare, value_compare;
  hash_free_func free_key, free_value;
#ifdef DEBUG
  int items;
  int stores, store_collisions;
  int retrieves, retrieve_collisions;
  int deletes, delete_collisions;
#endif
  struct hash_entry *entries;
};

/*
  Create a new hash table with room for at most nitems elements, with
  hash function calc and comparison functions key_compare and
  value_compare.

  Note that nitems *must* be correct, and that the behavior of the
  functions in this file is undefined if it isn't.  Note furthermore
  that even items that are deleted from the hash table may count
  against nitems, so the total number of items ever added to the table
  must be less than nitems.

  For generic string hashes, the functions hash_string_calc and
  hash_string_compare are available.
  */
hash_table_object hash_table_create(nitems, calc, key_compare, value_compare,
				    free_key, free_value)
     int nitems;
     hash_calc_func calc;
     hash_compare_func key_compare, value_compare;
     hash_free_func free_key, free_value;
{
  int i, size_bits = 1;
  struct hash_table *table;

  /*
    Figure out how big to make the hash table.  Multiply it by our
    multiplier, and then round up to the nearest power of two, since
    modulus by powers of two is faster.

    The table needs to be at least one bigger than the maximum number
    of items, so that searches will always terminate eventually by
    reaching an empty cell in the table.
    */
  nitems = (int)((float)(nitems + 1) * SIZE_MULTIPLIER);
  for (i = 1; i < nitems; i *= 2) size_bits++;
  nitems = i;

  table = (struct hash_table *) HASH_MALLOC(sizeof(*table));

  table->size = nitems;
  table->size_bits = size_bits;
  table->calc = calc;
  table->key_compare = key_compare;
  table->value_compare = value_compare;
  table->free_key = free_key;
  table->free_value = free_value;
#ifdef DEBUG
  table->items = 0;
  table->stores = table->store_collisions = 0;
  table->retrieves = table->retrieve_collisions = 0;
  table->deletes = table->delete_collisions = 0;
#endif
  table->entries = (struct hash_entry *)
    HASH_CALLOC(nitems, sizeof(*table->entries));

  return (hash_table_object)table;
}

/*
  Add a new element to the table.

  If unique is true, then the element must be unique.  Otherwise,
  multiple elements which compare equal are allowed to be in the table
  (and are retrieved in the order they were inserted).

  This function will always succeed unless unique is true and the key
  is non-unique.

  The key and value arguments are not copied.  The caller should
  ensure that they point to persistent values.

  Returns true on success, false on failure.
  */
int hash_table_insert(table_p, key, value, unique)
     hash_table_object table_p;
     void *key;
     void *value;
     int unique;
{
  struct hash_table *table = (struct hash_table *)table_p;
  int hash;

  for (hash = (*table->calc)(table->size, table->size_bits, key);
       table->entries[hash].status;
       hash = NEXT_HASH(hash,table->size)) {
    if (unique &&
	(table->entries[hash].status != HASH_DELETED) &&
	! (*table->key_compare)(key, table->entries[hash].key))
      return 0;
#ifdef DEBUG
    if ((*table->key_compare)(key, table->entries[hash].key))
      table->store_collisions++;
#endif
  }

  table->entries[hash].status = HASH_ADDED;
  table->entries[hash].key = key;
  table->entries[hash].value = value;
#ifdef DEBUG
  table->stores++;
  table->items++;
#endif

  return 1;
}

/*
  Retrieve an element from the table.

  If "reference" is non-null, then it is (a) used to determine where
  to start the search and (b) filled in with where the next search for
  the same key should be started.  This is used to retrieve multiple
  items with the same key from the table.  The first time this
  function is called for a particular value, the contents of reference
  should be HASH_NO_VALUE.

  Returns the value retrieved, or HASH_NO_VALUE there are none (or
  no more).
  */
void *hash_table_retrieve(table_p, key, reference)
     hash_table_object table_p;
     void *key;
     void **reference;
{
  struct hash_table *table = (struct hash_table *)table_p;
  void *dummy_reference = HASH_NO_VALUE;
  POINTER_NUM_TYPE hash;

  if (! reference)
    reference = &dummy_reference;

  if (*reference == HASH_NO_VALUE)
    hash = (*table->calc)(table->size, table->size_bits, key);
  else
    hash = NEXT_HASH((POINTER_NUM_TYPE)*reference, table->size);

  for ( ; table->entries[hash].status; hash = NEXT_HASH(hash,table->size)) {
    if ((table->entries[hash].status == HASH_DELETED) ||
	(*table->key_compare)(key, table->entries[hash].key)) {
#ifdef DEBUG
      table->retrieve_collisions++;
#endif
      continue;
    }
    *reference = (void *)hash;
#ifdef DEBUG
    table->retrieves++;
#endif
    return table->entries[hash].value;
  }

  return HASH_NO_VALUE;
}


/*
  Delete an entry from the hash table.

  Value is optional; if it is null, then all entries in the table with
  the given key are deleted.
  */
void hash_table_delete(table_p, key, value)
     hash_table_object table_p;
     void *key;
     void *value;
{
  struct hash_table *table = (struct hash_table *)table_p;
  int hash = (*table->calc)(table->size, table->size_bits, key);

  for ( ; table->entries[hash].status; hash = NEXT_HASH(hash,table->size)) {
    if (table->entries[hash].status == HASH_DELETED) {
#ifdef DEBUG
      table->delete_collisions++;
#endif
      continue;
    }
    if ((*table->key_compare)(key, table->entries[hash].key) ||
	((value != HASH_NO_VALUE) &&
	 (*table->value_compare)(value, table->entries[hash].value))) {
      int new_hash = (*table->calc)(table->size, table->size_bits,
				    table->entries[hash].key);
      if (table->entries[new_hash].status == HASH_DELETED) {
	table->entries[new_hash] = table->entries[hash];
	table->entries[hash].status = HASH_DELETED;
      }
#ifdef DEBUG
      table->delete_collisions++;
#endif
      continue;
    }
    table->entries[hash].status = HASH_DELETED;
    if (table->free_key)
      (*table->free_key)(table->entries[hash].key);
    if (table->free_value)
      (*table->free_value)(table->entries[hash].value);
#ifdef DEBUG
    table->deletes++;
    table->items--;
#endif
  }
  for (hash = PREV_HASH(hash, table->size);
       table->entries[hash].status == HASH_DELETED;
       hash = PREV_HASH(hash, table->size)) {
    table->entries[hash].status = 0;
  }
}


/*
  Destroy a hash table.
  */
void hash_table_destroy(table_p)
     hash_table_object table_p;
{
  struct hash_table *table = (struct hash_table *)table_p;
  int hash;

  for (hash = 0; hash < table->size; hash++) {
    if (table->entries[hash].status == HASH_ADDED) {
      if (table->free_key)
	(*table->free_key)(table->entries[hash].key);
      if (table->free_value)
	(*table->free_value)(table->entries[hash].value);
    }
  }

  HASH_FREE(table->entries);
  HASH_FREE(table);
}
  
int hash_string_calc(size, size_bits, key)
     int size;
     int size_bits;
     void *key;
{
  static int stashed_size = 0;
  static int shift_bits;

  unsigned int hash = 0;
  unsigned char *ptr;
  int cur_pos = 0;

  if (size != stashed_size) {
    stashed_size = size;
    for (shift_bits = CHAR_SIG_BITS; shift_bits < CHAR_SIG_BITS+8;
	 shift_bits++) {
      if (! (shift_bits % 8))
	continue;
      if (size_bits % (shift_bits % 8))
	break;
    }
    shift_bits %= 8;
  }
  for (ptr = (unsigned char *)key; *ptr; ptr++) {
    hash = hash ^ (*ptr << cur_pos);
    cur_pos = (cur_pos + shift_bits) % size_bits;
  }

  hash %= size;

#ifdef DEBUG
  if (hash_debug & HASH_PRINT_HASHES)
    printf("Key:\t%s\tHash:\t%d\n", (char *)key, hash);
#endif /* DEBUG */

  return hash;
}

int hash_string_compare(key1, key2)
     void *key1, *key2;
{
  return strcmp((char *)key1, (char *)key2);
}

int hash_int_calc(size, size_bits, key)
     int size;
     int size_bits;
     void *key;
{
  return (POINTER_NUM_TYPE)key % size;
}

int hash_int_compare(key1, key2)
     void *key1, *key2;
{
  return (POINTER_NUM_TYPE)key1 - (POINTER_NUM_TYPE)key2;
}

#ifdef DEBUG
#define PERCENT(a,b) ((b)?(100*(a)/(b)):0)

void hash_print_stats(table_p)
     hash_table_object table_p;
{
  struct hash_table *table = (struct hash_table *)table_p;

  printf("Size:\t%d\tItems:\t%d\n", table->size, table->items);
  printf("Stores:\t%d\tCollisions:\t%d (%d%%)\n",
	 table->stores, table->store_collisions,
	 PERCENT(table->store_collisions, table->stores));
  printf("Retrieves:\t%d\tCollisions:\t%d (%d%%)\n", table->retrieves,
	 table->retrieve_collisions,
	 PERCENT(table->retrieve_collisions, table->retrieves));
  printf("Deletes:\t%d\tCollisions:\t%d (%d%%)\n", table->deletes,
	 table->delete_collisions,
	 PERCENT(table->delete_collisions, table->deletes));
}
#endif /* DEBUG */

#ifdef TEST
#include <unistd.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

int main(argc, argv)
     int argc;
     char *argv[];
{
  int c;
  int size = 0;
  hash_table_object table_p;
  char buf[BUFSIZ];
  char *value = 0;
  void *reference = HASH_NO_VALUE;

  int getopt();
  extern char *optarg;

  while ((c = getopt(argc, argv, "m:s:")) != EOF) {
    switch (c) {
    case 'm':
      SIZE_MULTIPLIER = atof(optarg);
      break;
    case 's':
      size = atoi(optarg);
      break;
    default:
      exit(1);
    }
  }

  if (! size) {
    fprintf(stderr, "%s; Must specify size with -s\n", argv[0]);
    exit(1);
  }

  table_p = hash_table_create(size, hash_string_calc,
			      hash_string_compare,
			      hash_string_compare,
			      (hash_free_func)free,
			      0);

  c = 0;
  while (fgets(buf, sizeof(buf), stdin)) {
    c++;
    hash_table_insert(table_p, (void *)strdup(buf), (void *)c, 0);
  }

  hash_print_stats(table_p);
  hash_table_destroy(table_p);

  exit(0);
}
#endif /* TEST */
