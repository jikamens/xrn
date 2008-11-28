#ifndef AVL_H
#define AVL_H

/*
 * $Header: /d/src/cvsroot/xrn/avl.h,v 1.3 1994-10-10 18:46:30 jik Exp $
 */

/*
 * avl package
 *
 * Copyright (c) 1988-1993, The Regents of the University of California.
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

typedef struct avl_node_struct avl_node;
struct avl_node_struct {
    avl_node *left, *right;
    char *key;
    char *value;
    int height;
};


typedef struct avl_tree_struct avl_tree;
struct avl_tree_struct {
    avl_node *root;
    int (*compar)();
    int num_entries;
    int modified;
};


typedef struct avl_generator_struct avl_generator;
struct avl_generator_struct {
    avl_tree *tree;
    avl_node **nodelist;
    int count;
};


#define AVL_FORWARD 	0
#define AVL_BACKWARD 	1


extern avl_tree *avl_init_table _ARGUMENTS((int (*)(const char*,const char*)));
extern int avl_delete _ARGUMENTS((avl_tree *,char **,char **));
extern int avl_insert _ARGUMENTS((avl_tree *,char *,char *));
extern int avl_lookup _ARGUMENTS((avl_tree *,char *,char **));
extern int avl_count _ARGUMENTS((avl_tree *));
extern int avl_numcmp _ARGUMENTS((char *,char *));
extern int avl_check_tree _ARGUMENTS((avl_tree *));
extern int avl_gen _ARGUMENTS((avl_generator *,char **,char **));
extern void avl_foreach _ARGUMENTS((avl_tree *,void (*)(char *,char *),int));
extern void avl_free_gen _ARGUMENTS((avl_generator *));
extern avl_generator *avl_init_gen _ARGUMENTS((avl_tree *,int));
extern void avl_free_table _ARGUMENTS((avl_tree *,void (*)(char *),
    void (*)(void *)));

#define avl_is_member(tree, key)	avl_lookup(tree, key, (char **) 0)

     /*
      * Warning!  This macro does not check if the call to
      * avl_init_gen succeeds, and it may fail if it runs out of
      * memory.  Therefore, I don't recommend you use this macro at
      * all.  I'm leaving it in only because it was here when I
      * started working on the code.
      */
#define avl_foreach_item(table, gen, dir, key_p, value_p) 	\
    for(gen = avl_init_gen(table, dir); 			\
	    avl_gen(gen, key_p, value_p) || (avl_free_gen(gen),0);)

#endif

