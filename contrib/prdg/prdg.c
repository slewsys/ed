/* prdg.c: Entry point for directed graph printer.
 *
 * SPDX-FileCopyrightText: 1989,2024 Andrew L. Moore <slewsys@gmail.com>, SlewSys Research
 *
 * SPDX-License-Identifier: CC0
 */
#include "prdg.h"

name_node_t *vertex_list = NULL; /* List of vertex names. */

/* Option parameters. */
long maxdepth = DEPTH_MAX;       /* Maximum path depth. */
long width = PAPERWIDTH;         /* Paper width (in columns). */
int flags = 0;                   /* Command-line flags. */
int ntabs = NTABS;               /* Graph width in tabs. */

char *program_name = "prdg";

int
main (int argc, char **argv)
{
  char *arglist = "ad:himr:w:x"; /* Command-line options. */
  char *roots =  NULL;           /* Roots from which to print DG. */
  int roots_size = 0;            /* Root count. */
  int c;
  int status = 0;

  while ((c = getopt (argc, argv, arglist)) != EOF)
    switch (c)
      {
      case 'a':
        flags |= GRAPH_ALL;
        break;
      case 'd':
        if (((maxdepth = strtol (optarg, NULL, 10)) == 0 && errno == EINVAL)
            || ((maxdepth == LONG_MIN
                 || maxdepth == LONG_MAX) && errno == ERANGE))
          {
            fprintf (stderr, "%s\n", strerror (errno));
            exit (1);
          }
        else if (!(0 < maxdepth && maxdepth <= DEPTH_MAX))
          {
            fprintf (stderr, "graph depth: %ld: Not in range (0,%d].\n",
                     maxdepth, DEPTH_MAX);
            exit (1);
          }
        break;
      case 'h':
        usage ();
        exit (0);
      case 'i':
        flags |= EXPAND_ALL | GRAPH_ALL;
        maxdepth = 2;
        break;
      case 'm':
        flags |= ALLOW_REPEATED_ARCS;
        break;
      case 'r':
        if (!(roots = (char *) realloc (roots, ++roots_size * sizeof (char *)))
            || !(((char **) roots)[roots_size - 1] = strdup (optarg)))
          {
            fprintf (stderr, "%s\n", strerror (errno));
            exit (1);
          }
        break;
      case 'w':
        if (((width = strtol (optarg, NULL, 10)) == 0 && errno == EINVAL)
            || ((width == LONG_MIN
                 || width == LONG_MAX) && errno == ERANGE))
          {
            fprintf (stderr, "%s\n", strerror (errno));
            exit (1);
          }
        else if (!(0 < width && width <= DEPTH_MAX * TABSIZE + MARGIN))
          {
            fprintf (stderr, "graph depth: %ld: Not in range (0,%d].\n",
                     width, DEPTH_MAX * TABSIZE + MARGIN);
            exit (1);
          }
        break;
      case 'x':
        flags |= EXPAND_ALL;
        break;
      case '?':
      default:
        usage ();
        exit (1);
      }
  ntabs = (width - MARGIN) / TABSIZE;

  if (!(status = build_graph ())
      || !(status = roots_size
           ? print_selected_graphs ((char **) roots, roots_size)
           : print_topological_graphs ()))
    exit (!status);
  clean_up ();
  exit (0);
}

void
usage ()
{

  printf ("Usage: %s [OPTION ...]\n\
Options:\n\
  -a        Print a separate graph for each name (default: no).\n\
  -d N      Print graphs to at most depth N (default: 200).\n\
  -h        Print (this) help, then exit.\n\
  -i        Print inverted graph. Implies `-d 2' (default: no).\n\
  -m        Allow multiple instances of the same arc (default: no).\n\
  -r ROOT   Print graph only for ROOT. (default: topological root).\n\
  -x        Print each subgraph in full (default: no).\n\
  -w N      Print graph to fit in N columns (default: 132).\n", program_name);
}

/*
 * Ordered name pairs representing directed arcs between vertices of a
 * graph are expected on standard input in the form:
 *
 *     name1a --> name2aa
 *     name1a --> name2ab
 *     name1a --> name2ac
 *     .
 *     .
 *     .
 *     name1b --> name2ba
 *     name1b --> name2bb
 *     name1b --> name2bc
 *     .
 *     .
 *     .
 *
 * where the arrows (-->) indicate the direction of the arcs, but are
 * not themselves part of the input. By default, only the first
 * instance of an arc is printed (see option `-m').
 *
 * An initial name1-name1 pair represents a vertex (i.e., of type
 * name_node_t). A label can be assigned to a vertex with input
 * element of the form:
 *
 *     name1 --> name1 label1
 *
 * A cycle occurs when either multiple name1-name1 pairs occur or two
 * distinct vertices reference each other, either directly or
 * indirectly, e.g.,
 *
 *     name1 --> name1
 *     name1 --> name1
 *
 * or
 *
 *     name1 --> name2
 *     name2 --> name1
 *
 * or
 *
 *     name1 --> name2
 *     name2 --> name3
 *     name3 --> name1
 *
 * Since a cyclic graph may not have a natural topological root,
 * printing such a graph requires specifying an artbitrary root on the
 * command line (e.g., `-r name1').
 *
 * A graph can be inverted by reversing the relation between name
 * pairs, i.e., by putting name2 first:
 *
 *     name2 --> name1
 */

name_node_t *current_vertex = NULL;          /* Current graph node. */
adjacency_node_t *adjacency_tail = NULL;     /* Tail of adjacency list. */
char current_vertex_name[NAME_MAX + 1] = ""; /* Previous name1. */
char *nil_label = "{}";                      /* Default label. */

/*
 * build_graph: Get (vertex) name pairs from standard input, and
 *     insert them as arcs in a directed graph (DG). Each vertex
 *     contains a list of vertices (the adjacency list) with which it
 *     forms a directed arc.
 */
int
build_graph (void)
{
  char buf[BUFSIZ];             /* I/O buffer. */
  char *name1 = NULL;                  /* Vertex name. */
  char *name2 = NULL;                  /* Adjacenct vertex name. */
  char *label1 = NULL;                 /* Vertex label. */
  char eol_char;                /* I/O buffer character. */

  for (;fgets (buf, BUFSIZ, stdin); name1 = name2 = label1 = NULL)
    {
      if (!parse_adjacency (buf, &name1, &name2, &label1))
        return 0;
      else if (!name1)
        continue;

      if (is_new_vertex (name1, name2)
          && (!(current_vertex = create_vertex (name1, label1))
              || !(adjacency_tail = get_adjacency_tail (current_vertex))))
        return 0;
      else if (is_new_arc (name1, name2)
               && !(adjacency_tail = create_arc (name2)))
        return 0;
      if (!strcmp (name1, name2))

        /* Subsequent name1 --> name1 pairs are cycles. */
        current_vertex->registered |= 1;
    }

  if (ferror (stdin))
    fprintf (stderr, "%s\n", strerror (errno));

  return feof (stdin);
}

/*
 * parse_adjacency: Parse (and modify) a buffer containing a
 *     space-separated name pair and optional label, i.e.,
 *
 *         name1 name2 [label1]
 *
 *     Although name1, name2 and label are space-separated, label is
 *     delimited by newline, and so may include spaces. The
 *     null-terminated tokens are returned as pointers into the
 *     buffer. If successful, return 1, otherwise 0.
 */
int
parse_adjacency(char *buf, char **name1, char **name2, char **label)
{
  if (!(*name1 = strtok (buf, " \t")))
      return 1;
  if (!(*name2 = strtok (NULL, " \t\n")))
      return 0;
  if (!(*label = strtok (NULL, "\n")))
        *label = nil_label;
  return 1;
}

/*
 * is_new_vertex: If either 1) current vertex not set, or 2) name1
 *      differs from that of current vertex, or 3) name1 and name2 are
 *      the same, then return 1. Otherwise 0.
 */
 int
is_new_vertex (char *name1, char *name2)
{
  return (!vertex_list || strcmp (name1, current_vertex_name)
          || !strcmp (name1, name2));
}

/*
 * is_new_arc: If either 1) name1 is the current vertex name and
 *     differs from name2, or 2) name1 and name2 are the same and a
 *     vertex with this name was previously registered, then return 1.
 *     Otherwise 0.
 */
 int
is_new_arc (char *name1, char *name2)
{
  name_node_t *nn_p;

  return (!strcmp (name1, current_vertex_name) && strcmp (name1, name2)
          || (!strcmp (name1, name2) && (nn_p = get_vertex (name1))
              && nn_p->registered));
}

name_node_t *
create_vertex (char *name, char *label)
{
  name_node_t *nn_p = get_vertex (name);
  adjacency_node_t *an_p;

  if (nn_p &&
      !(an_p = (strcmp (label, nil_label)
                ? update_vertex (nn_p, label)
                : nn_p->adjacency_list)))
    return NULL;
  else if (!nn_p && !(an_p = insert_vertex (name, label)))
    return NULL;

  strcpy (current_vertex_name, name);
  return an_p->name_node;
}

/*
 * create_arc: If the given name is that of an existing (adjacency)
 *     vertex, then append it to the current vertex's adjacency list
 *     if either its not already there or multiple instances are okay.
 *     If the given name is not that of an existing vertex, then
 *     create a new vertex and add it to the current vertex's
 *     adjacency list.
 */
adjacency_node_t *
create_arc (char *name)
{
  name_node_t *nn_p;
  adjacency_node_t *an_p;

  if ((nn_p = get_vertex (name))
      && (flags & ALLOW_REPEATED_ARCS
          || !(an_p = get_adjacency (name, current_vertex->adjacency_list)))
      && !(an_p = insert_arc (nn_p, adjacency_tail)))
    return NULL;
  else if (!nn_p && (!(an_p = insert_vertex (name, nil_label))
                     || !(an_p = insert_arc (an_p->name_node,
                                             adjacency_tail))))
    return NULL;
  return an_p;
}

/*
 * insert_vertex: Allocate name and adjacency nodes. Initialize
 *     adjacency node to point to name node, and add it to head of the
 *     name node's adjacency list. Insert name node in vertex list. If
 *     successful, return a pointer to the adjacency node, otherwise
 *     NULL.
 */
adjacency_node_t *
insert_vertex (char *name, char *label)
{
  name_node_t *nn_p;

  if (!(nn_p = create_name_node (name, label))
      || !(nn_p->adjacency_list = create_adjacency_node ()))
    return NULL;

  nn_p->adjacency_list->name_node = nn_p;
  insert_name_node (nn_p);
  return nn_p->adjacency_list;
}

/*
 * update_vertex: Update given vertex's label and return pointer to
 *     tail of its adjacency list.
 */
adjacency_node_t *
update_vertex (name_node_t *np, char *label)
{
  adjacency_node_t *an_p = np->adjacency_list;

  free (np->label);
  if (!(np->label = strdup (label)))
    return NULL;

  return an_p;
}

/*
 * insert_arc: Allocate adjacency node, initialize it to point to the
 *     given name node, and link it to after the given adjacency node.
 *     If successful, return a pointer to the new adjacency, otherwise
 *     NULL.
 */
adjacency_node_t *
insert_arc (name_node_t *np, adjacency_node_t *ap)
{
  if (!(ap->next = create_adjacency_node ()))
    return NULL;

  ap->next->name_node = np;
  np->is_arc_head = 1;
  return ap->next;
}

/*
 * create_name_node: Allocate and initialize a name node. If
 *     successful, return a node pointer, otherwise NULL.
 */
name_node_t *
create_name_node (char *name, char *label)
{
  name_node_t *nn_p;

  if (!(nn_p = (name_node_t *) malloc (sizeof (name_node_t)))
      || !(nn_p->name = strdup(name))
      || !(nn_p->label = strdup(label)))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      return NULL;
    }

  nn_p->next = NULL;
  nn_p->adjacency_list = NULL;
  nn_p->is_arc_head = 0;
  nn_p->name_visited = 0;
  return nn_p;
}


/*
 * create_adjacency_node: Allocate and initialize an adjacency node. If
 *     successful, return a node pointer, otherwise NULL.
 */
adjacency_node_t *
create_adjacency_node (void)
{
  adjacency_node_t *an_p;

  if (!(an_p = (adjacency_node_t *) malloc (sizeof (adjacency_node_t))))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      return NULL;
    }
  an_p->next = NULL;
  return an_p;
}

/*
 * insert_name_node: Initialize vertex_list and insert given node in
 *    lexicographical order by name.
 */
void
insert_name_node (name_node_t *np)
{
  name_node_t *nl_p = vertex_list;

  if (nl_p)
    {
      while (nl_p->next && strcmp (np->name, nl_p->next->name) > 0)
        nl_p = nl_p->next;
      np->next = nl_p->next;
      nl_p->next = np;
    }
  else
    vertex_list = np;
}

/*
 * print_graph: Preorder traverse the DG from each topological root.
 */
int
print_topological_graphs (void)
{
  name_node_t *root_p;

  for (root_p = vertex_list; root_p; root_p = root_p->next)
    if ((flags & GRAPH_ALL || !root_p->is_arc_head) &&
        !print_graph (root_p))
      return 0;
  return 1;
}

/*
 * print_root: Preorder traverse the DG starting from each given root.
 */
int
print_selected_graphs (char **roots, int roots_size)
{
  name_node_t *root_p;
  int i = 0;

  for (i = 0; i < roots_size; ++i)
    {
      if (!(root_p = get_vertex (roots[i])))
        {
          (void) fprintf (stderr, "%s: %s: Node/vertex not in graph\n",
                          program_name, roots[i]);
          return -1;
        }
      if (!print_graph (root_p))
        return 0;
    }
  return 1;
}

/* Formatting strings. */
char tabs[DEPTH_MAX + 1] = {[0 ... DEPTH_MAX - 1] = '\t', '\0'};

#define SHIFT_MAX (DEPTH_MAX / NTABS)

char shifts[SHIFT_MAX + 1] = {[0 ... SHIFT_MAX - 1] = '<', '\0'};
char *spaces = "        ";

/*
 * PRINT_VERTEX: Print vertex name indented to its path depth, up
 *     paper to width. To indent further, prefix with shift marks (<).
 */
#define PRINT_VERTEX(format, _depth, name, label)                             \
  printf ("%-5ld%-3.*s%.*s" format, line_no, (_depth) / ntabs, shifts,        \
          (_depth) % ntabs, tabs, (name), (label))

/*
 * print_root: Preorder traverse the DG from the given vertex by
 *     iteratively following the top-level adjacency of that vertex to
 *     the adjacency vertex's top-level adjacency, and so on, pushing
 *     onto the stack adjacency pointers for back-tracking to an
 *     unvisited vertex. The path depth is number of adjacencies
 *     followed. As each vertex is visited, its name is printed,
 *     indented to the current path depth. if a vertex that's already
 *     on the stack is revisited, then a ... cycle ... mark is printed
 *     and the path terminated. Return 1 or 0 on error.
 */
int
print_graph (name_node_t *np)
{
  static off_t line_no = 1;

  name_node_t *nn_p = np;
  adjacency_node_t *an_p = nn_p->adjacency_list;
  int depth = 0;

  goto skip_first_increment;
  do
    {
      while ((an_p = an_p->next) && depth < maxdepth)
        {
        skip_first_increment:
          nn_p = an_p->name_node;
          if (is_active_node (an_p))
              PRINT_VERTEX ("    ... %s ... {%ld}\n",
                            depth - 1, nn_p->name, nn_p->name_visited);
          else if (nn_p->name_visited && !(flags & EXPAND_ALL))
              PRINT_VERTEX ("%s ... {%ld}\n",
                            depth, nn_p->name, nn_p->name_visited);
          else
            {
              PRINT_VERTEX ("%s %s\n", depth, nn_p->name, nn_p->label);
              nn_p->name_visited = line_no;
              if (!push_active (an_p))
                return 0;

              /* Update an_p only after pushing it onto the active_node stack. */
              an_p = nn_p->adjacency_list;
              ++depth;
            }
          ++line_no;
        }
      an_p = pop_active ();
      --depth;
    }
  while (an_p && an_p->name_node != np);
  return 1;
}

adjacency_node_t *active_node = NULL;     /* Path in DG. */
int active_node_size = 0;                 /* Size of path in DG. */

/*
 * push_active: Add the given pointer to the top of the active_node
 *     stack. Return the stack size, or 0 on error.
 */
int
push_active (adjacency_node_t *ap)
{
  if (!(active_node =
        (adjacency_node_t *) realloc (active_node,
                                      ++active_node_size
                                      * sizeof (adjacency_node_t *))))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      return 0;
    }
  ((adjacency_node_t **) active_node)[active_node_size - 1] = ap;
  return active_node_size;
}

/*
 * pop_active_node: Remove a pointer from the top of the active_node
 *     stack and return it. If the stack is empty, return NULL.
 */
adjacency_node_t *
pop_active (void)
{
  return active_node_size
    ? ((adjacency_node_t **) active_node)[--active_node_size]
    : NULL;
}

/*
 * is_active_node: Search for the given pointer on the active_node stack.
 *     Return 1 if found, otherwise 0.
 */
int
is_active_node (adjacency_node_t *ap)
{
  int i;

  for (i = 0; i < active_node_size; i++)
    if (ap->name_node == ((adjacency_node_t **) active_node)[i]->name_node)
      return 1;
  return 0;
}

/*
 * get_vertex: Search for the given name on the vertex list. Return
 *     a node pointer if found, otherwise NULL.
 */
name_node_t *
get_vertex (char *name)
{
  name_node_t *nl_p = vertex_list;

  if (strlen (name) > NAME_MAX)
    name[NAME_MAX] = '\0';

  while (nl_p && strcmp (name, nl_p->name))
    nl_p = nl_p->next;
  return nl_p;
}

/*
 * get_adjacency: Search for the given name on the adjacency list. Return
 *     a node pointer if found, otherwise NULL.
 */
adjacency_node_t *
get_adjacency (char *name, adjacency_node_t *ap)
{
  adjacency_node_t *al_p = ap->next;

  if (strlen (name) > NAME_MAX)
    name[NAME_MAX] = '\0';

  while (al_p && strcmp (name, al_p->name_node->name))
    al_p = al_p->next;
  return al_p;
}

adjacency_node_t *
get_adjacency_tail (name_node_t *np)
{
  adjacency_node_t *an_p = np->adjacency_list;

  while (an_p->next)
    an_p = an_p->next;
  return an_p;
}

void
clean_up (void)
{

  name_node_t *nl_p ;
  adjacency_node_t *an_p;
  name_node_t *nl_next;
  adjacency_node_t *an_next;

  for (nl_p = vertex_list; nl_p; nl_p = nl_next)
    {
      for (an_p = nl_p->adjacency_list; an_p; an_p = an_next)
        {
          an_next = an_p->next;
          free (an_p);
        }
      nl_next = nl_p->next;
      free (nl_p->name);
      free (nl_p->label);
      free (nl_p);
    }
}
