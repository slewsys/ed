/* prdcg.h: Structures and declarations for the Directed Graph Printer. */
/*
 * SPDX-FileCopyrightText: 1989,2024 Andrew L. Moore, SlewSys Research
 *
 * SPDX-License-Identifier: CCO
 */
#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
#else
void *malloc ();
void *realloc ();
#endif  /* !STDC_HEADERS */

#if defined (STDC_HEADERS) || defined (HAVE_STRING_H)
# include <string.h>
#endif

#ifndef HAVE_STRERROR
extern const char *sys_errlist[];
extern int sys_nerr;
# define strerror(n)                                                          \
  ((n) > 0 && (n) < sys_nerr ? sys_errlist[n] : "Unknown error")
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#else
extern int optind;
extern char *optarg;
#endif  /* !HAVE_UNISTD_H */

#ifndef errno
extern int errno;
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#define LINE_MAX (2 * PATH_MAX)  /* Input line limit. */

typedef struct name_node
{
  struct name_node *next;	/* Next node in name list. */
  struct adjacency_node *adjacency_list; /* Node's adjacency list. */
  long name_visited;		/* Set if node previously visited. */
  int is_arc_head;		/* Set if node is the head of an arc. */
  char *adjacency_name;		/* Name of first node in adjacency_list. */
  char *adjacency_label;        /* Label of first node in adjacency_list */
} name_node_t;

typedef struct adjacency_node
{
  struct adjacency_node *next;  /* Next node in adjacency list. */
  name_node_t *name_node_p;     /* Name-node pointer. */
} adjacency_node_t;

name_node_t *add_to_name_list(char *, char *);
int build_graph (void);
adjacency_node_t *create_arc_node (char *, char *);
adjacency_node_t *get_adjacency_node (void);
int get_arc (char *, char *, char *);
name_node_t *get_name_list_node (char *);
int is_active_node (name_node_t *);
adjacency_node_t *link_arc_node (char *, adjacency_node_t *);
adjacency_node_t *node_to_arc (name_node_t *, adjacency_node_t *);
int parse_adjacency_line (char *, char **, char **, char **);
int print_graph (int, char **);
void print_name (name_node_t *, int);
void pop_active_node (void);
int push_active_node (name_node_t *);
void usage (void);
