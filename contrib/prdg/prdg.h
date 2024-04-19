/* prdcg.h: Structures and declarations for the directed graph printer.
 *
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
#include <string.h>
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

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifndef MIN
# define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef errno
extern int errno;
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#define DEPTH_MAX 500           /* Default path depth limit.  */
#define TABSIZE 8               /* Tab width. */
#define MARGIN 20               /* Margin width. */

/* Default paper width in tabs. */
#define NTABS 400

/*
 * Default graph width in columns.
 * For NTABS == 14, 132 columns.
 * For NTABS == 400, 3220 columns.
 */
#define PAPERWIDTH (NTABS * TABSIZE + MARGIN)

typedef struct name_node
{
  struct name_node *next;       /* Next node in name list. */
  struct adjacency_node *adjacency_list; /* Adjacency list. */
  off_t name_visited;           /* Set if node previously visited. */
  int is_arc_head;              /* Set if node is the head of an arc. */
  char *name;                   /* Node name. */
  char *label;                  /* Node label. */
  int registered;               /* Set to flag self-cycles.  */
} name_node_t;

typedef struct adjacency_node
{
  struct adjacency_node *next;  /* Next node in adjacency list. */
  name_node_t *name_node;       /* Adjacency's name node. */
} adjacency_node_t;

enum options
  {
    GRAPH_ALL           = 0x01, /* Print DG from each vertex. */
    EXPAND_ALL          = 0x02, /* Expand each . */
    PRINT_ROOTS         = 0x04, /* Print DG for given roots. */
    ALLOW_REPEATED_ARCS = 0x08  /* Allow multiple instances of an arc in DG. */
  };

int build_graph (void);
void clean_up (void);
adjacency_node_t *create_adjacency_node (void);
adjacency_node_t *create_arc (char *);
name_node_t *create_name_node (char *, char *);
name_node_t *create_vertex (char *, char *);
adjacency_node_t * get_adjacency (char *, adjacency_node_t *);
adjacency_node_t *get_adjacency_tail (name_node_t *);
name_node_t *get_vertex (char *);
void insert_name_node (name_node_t *);
adjacency_node_t *insert_vertex (char *, char *);
adjacency_node_t *insert_arc (name_node_t *, adjacency_node_t *);
int is_active_node (adjacency_node_t *);
int is_new_arc (char *, char *);
int is_new_vertex (char *, char *);
int parse_adjacency (char *, char **, char **, char **);
int print_topological_graphs (void);
int print_selected_graphs (char **, int);
int print_graph (name_node_t *);
adjacency_node_t *pop_active (void);
int push_active (adjacency_node_t *);
adjacency_node_t *update_vertex (name_node_t *, char *);
void usage (void);
