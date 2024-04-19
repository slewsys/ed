/* prdg.c: Directed Graph Printer. */
/*
 * SPDX-FileCopyrightText: 1989,2024 Andrew L. Moore, SlewSys Research
 *
 * SPDX-License-Identifier: CCO
 */
#include "prdg.h"

#define DEPTH_MAX 200                    /* Function stack limit.  */
#define TABSIZE 8                        /* Tab width. */
#define MARGIN 20                        /* Margin width. */
#define PAPERWIDTH (14*TABSIZE + MARGIN) /* Graph width limit. */

name_node_t *name_list;
int maxdepth = DEPTH_MAX;       /* print to at most depth `maxdepth' */
int graph_all = 0;              /* Print a graph for each name. */
int expand_all = 0;             /* Print each sub-graph in full. */
int select_roots = 0;           /* Print graph for selected names. */
int ntabs = ((PAPERWIDTH - MARGIN) / TABSIZE); /* Graph width. */

char *pgm = "prdg";
char *arglist = "ad:r:ixw:"; /* valid options */

extern char *optarg;
extern int optind;

int
main (int argc, char **argv)
{
  int c, i;
  int width = PAPERWIDTH;
  int status = 0;

  while ((c = getopt (argc, argv, arglist)) != EOF)
    switch (c)
      {
      case 'a':
        graph_all = 1;
        break;
      case 'd':
        if ((maxdepth = atoi (optarg)) > DEPTH_MAX)
          maxdepth = DEPTH_MAX;
        break;
      case 'i':
        expand_all = 1;
        graph_all = 1;
        maxdepth = 2;
        break;
      case 'r':
        select_roots = 1;
        break;
      case 'w':
        if ((width = atoi (optarg)) <= 0)
          width = PAPERWIDTH;
        break;
      case 'x':
        expand_all = 1;
        break;
      case '?':
      default:
        usage ();
        exit (1);
      }
  ntabs = (width - MARGIN) / TABSIZE;

  if ((status = build_graph ()) != 0
      || (status = print_graph (argc, argv) != 0))
    exit (status);
  exit (0);
}

void
usage ()
{

  printf ("Usage: %s [OPTION ...]\n\
Options:\n\
  -a        Print a separate call graph for each name (default: no).\n\
  -d N      Print graphs to at most depth N (default: 20).\n\
  -r ROOT   Print graph only for ROOT (default: N/A).\n\
  -x        Print each subgraph in full (default: no).\n\
  -w N      Print graph to fit in W columns (default: 132).\n", pgm);
}

/*
 * Name pairs in the input are expected in blocks of the form:
 *
 * name1a --> name2aa
 * name1a --> name2ab
 * name1a --> name2ac
 * .
 * .
 * .
 * name1b --> name2ba
 * name1b --> name2bb
 * name1b --> name2bc
 * .
 * .
 * .
 *
 * For a distinct name1, only the first block of name pairs is valid.  A
 * graph can be inverted by reversing the relation between name pairs,
 * i.e., by putting name2 first:
 *
 * name2 --> name1
 *
 * Unless a block contains only a single name pair, then an initial
 * name1-name1 pair is effectively ignored.  A name1-name1 pair after the
 * first represents a cycle - i.e., a node which points to itself.  A
 * block consisting of a single name1-name1 pair represents a non-cyclic,
 * possibly disconnected, node.
 */

adjacency_node_t *adjacency_tail; /* Pointer to end of adjacency list. */

/*
 * build_graph: Get name pairs from standard input, and insert them as
 *     arcs in a directed graph (DG). An arc tail is the head of a
 *     singly linked list (the adjacency list) to which it is
 *     connected (logically).
 */
int
build_graph (void)
{
  char buf[LINE_MAX];
  char *arc_tail;               /* Name of arc tail-vertex. */
  char *arc_head;               /* Name of arc head-vertex. */
  char *arc_label;              /* Tail vertex label. */
  int is_arc;

  while (fgets (buf, LINE_MAX, stdin) != NULL)
    {
      if (parse_adjacency_line (buf, &arc_tail, &arc_head, &arc_label) == -1
          || (is_arc = get_arc (arc_tail, arc_head, arc_label)) == -1)
        return 1;

      if (!is_arc)
        adjacency_tail = create_arc_node (arc_tail, arc_label);
      else if (is_arc && adjacency_tail)
        adjacency_tail = link_arc_node (arc_head, adjacency_tail);
      else
        fprintf (stderr, "%s: cannot redefine: %s\n", pgm, arc_tail);
    }

  if (ferror (stdin))
    fprintf (stderr, "%s\n", strerror (errno));

  return !feof (stdin);
}

/* SKIP_WHITESPACE: Scan buffer for next non-space char. */
#define SKIP_WHITESPACE(cp)                                                   \
  do                                                                          \
    {                                                                         \
      while (isspace ((unsigned char) *(cp)))                                 \
        ++(cp);                                                               \
      if (*(cp) == '\0')                                                      \
        {                                                                     \
          fprintf (stderr, "%s: %s: Unexpected end of input.\n", pgm, buf);   \
          return -1;                                                          \
        }                                                                     \
    }                                                                         \
  while (0)

/* SKIP_TO_WHITESPACE: Scan buffer for next space char. */
#define SKIP_TO_WHITESPACE(cp, c)                                             \
  do                                                                          \
    {                                                                         \
      while (!isspace ((unsigned char) *(cp)) && *(cp) != '\n'                \
             && *(cp) != '\0')                                                \
        ++(cp);                                                               \
      if ((c) && (*(cp) == '\n' || *(cp) == '\0'))                            \
        {                                                                     \
          fprintf (stderr, "%s: %s: Unexpected end of input.\n", pgm, buf);   \
          return -1;                                                          \
        }                                                                     \
      c = *(cp);                                                              \
    }                                                                         \
  while (0)

/* SKIP_TO_WHITESPACE: Scan buffer for next space char. */
#define SKIP_TO_EOL(cp)                                                       \
  do                                                                          \
    {                                                                         \
      while (*(cp) != '\0' && *(cp) != '\n')                                  \
        ++(cp);                                                               \
    }                                                                         \
  while (0)

char tail_name[NAME_MAX] = "";	/* previous arc tail name */
char *nil = "";

/*
 * parse_adjacency_line: Read from stdin a line containing a
 *     space-separated name pair and optional label, i.e.,
 *       name1 name2 [label1]
 *     Replace spaces with nils and assign each string to its
 *     respective pointer.
 *     Return -1 on I/O error or EOF, otherwise 0.
 */
int
parse_adjacency_line (char *buf, char **name1, char **name2, char **label)
{
  char *cp = buf;
  char noeol = 1;               /* If set, fail on EOL. */

  SKIP_WHITESPACE (cp); *name1 = cp++; noeol = 1;
  SKIP_TO_WHITESPACE (cp, noeol); *cp++ = '\0';
  SKIP_WHITESPACE (cp); *name2 = cp++; noeol = 0;
  SKIP_TO_WHITESPACE (cp, noeol); *cp++ = '\0';
  if (noeol == '\n' || noeol == '\0')
    *label = nil;
  else
    {
      SKIP_WHITESPACE (cp); *label = cp++;
      SKIP_TO_EOL (cp); *cp = '\0';
    }
  return 0;
}

/*
 * get_arc: Copy name1 to tail_name if they differ.
 *     Return 1 if name1 is an arc tail, otherwise 0.
 */
int
get_arc (char *name1, char *name2, char *label)
{
  /* New name1. */
  if (strcmp (name1, tail_name))
    {
      strcpy (tail_name, name1);
      if (!strcmp (name1, name2))
        return 0;
      return (adjacency_tail = create_arc_node (name1, label)) != NULL ? 1 : -1;
    }
  return strcmp (name1, name2) ? 1 : 0;
}

/*
 * create_arc_node: Given a name (s), if it is not already on the name
 *     list, create a node for it there. Otherwise, retrieve the name
 *     list node. Create an arc tail (i.e., adjacency_list) node and link it
 *     to the new/retrieved name node (via the name node's adjacency_list
 *     pointer). Return a pointer to the arc tail node.
 */
adjacency_node_t *
create_arc_node (char *s, char *t)
{
  name_node_t *np;
  adjacency_node_t *ap;

  /* Name already on name list. */
  if ((np = get_name_list_node (s)))
    {
      if ((ap = node_to_arc (np, NULL)) != 0)
        {
          if ((np->adjacency_label = (char *) realloc (np->adjacency_label, strlen (t) + 1)) == NULL)
            {
              fprintf (stderr, "%s\n", strerror (errno));
              return NULL;
            }

          /* update arc reference */
          strcpy (np->adjacency_label, t);
          return ap;
        }
    }
  /* add name to name list and install arc tail node */
  return node_to_arc (add_to_name_list (s, t), NULL);
}


/*
 * link_arc_node: Given a name (s), if it is not already on the name
 *     list, create a node for it there. Otherwise, retrieve the name
 *     list node. Create an arc head (i.e., adjacency_list) node and link it
 *     to the tail of the current adjacency list. Return a pointer to
 *     the arc head node.
 */
adjacency_node_t *
link_arc_node (char *s, adjacency_node_t *tail)
{
  name_node_t *p;
  adjacency_node_t *ap = 0;

  /* (name already on name list or added name) and installed new arc
   * and name != arc tail's name */
  if (((p = get_name_list_node (s)) != 0 || (p = add_to_name_list (s, nil)) != 0)
      && (ap = node_to_arc (p, tail)) != 0 && strcmp (s, tail_name))
    p->is_arc_head = 1;		/* arc head node */
  return ap;
}

/*
 * add_to_name_list: Allocate memory for a name list node. Insert the
 *     node in the name list in lexicographical order by name. Return
 *     a pointer to the node.
 */
name_node_t *
add_to_name_list (char *s, char *t)
{
  name_node_t *np, *p;

  /* name structure, name and arc reference alloc'd */
  if ((np = (name_node_t *) malloc (sizeof (name_node_t))) != NULL
      && (np->adjacency_name = strdup(s)) != NULL
      && (np->adjacency_label = strdup(t)) != NULL)
    {
      /* initialize name structure */
      np->adjacency_list = NULL;
      np->is_arc_head = 0;
      np->name_visited = 0;

      /* no name list, or name less than head of name list */
      if (name_list == 0 || strcmp (name_list->adjacency_name, s) > 0)
        {
          /* add node to head of name list */
          np->next = name_list;
          name_list = np;
        }
      else
        {
          /* insert node in lexicographical order in name list */
          for (p = name_list; p->next; p = p->next)
            if (strcmp (p->next->adjacency_name, s) > 0)
              break;
          np->next = p->next;
          p->next = np;
        }
      return np;
    }
  return NULL;
}


/*
 * node_to_arc: Create an adjacency node (*adjacency_p) with a pointer
 *     (adjacency_p->name_node_p) to its name list node (np). If the given
 *     adjacency list pointer (ap) is NULL, the new adjacency node is
 *     an arc tail, and it is pointed to by its name list node.
 *     Otherwise, the new adjacency node is an arc head and is linked
 *     after the given adjacency list pointer. Return a pointer to the
 *     new adjacency node.
 */
adjacency_node_t *
node_to_arc (name_node_t *np, adjacency_node_t *ap)
{
  adjacency_node_t *adjacency_p;

  /* null name or (dummy && an adjacency exists) or null adjacency */
  if (np == NULL || (ap == NULL && np->adjacency_list) || (adjacency_p = get_adjacency_node ()) == NULL)
    return NULL;
  adjacency_p->name_node_p = np;
  return ap ? (ap->next = adjacency_p) : (np->adjacency_list = adjacency_p);
}

/*
 * get_adjacency_node: Return pointer to alloc'd and initialized adjacency
 *     node.
 */
adjacency_node_t *
get_adjacency_node (void)
{
  adjacency_node_t *rp;

  if ((rp = (adjacency_node_t *) malloc (sizeof (adjacency_node_t))) == NULL)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      return NULL;
    }
  rp->next = NULL;
  return rp;
}

/*
 * print_graph: Preorder traverse the DG, printing the names of nodes
 *     as they are visited.
 */
int
print_graph (int argc, char **argv)
{
  int c;
  name_node_t *root_p;

  /* root node(s) specified */
  if (select_roots)
    {
    /* restart argument list; print only specified names */
    for (optind = 1; (c = getopt (argc, argv, arglist)) != EOF;)
      if (c == 'r')
        {
          if ((root_p = get_name_list_node (optarg)))
            print_name (root_p, 1);
          else
            {
              (void) fprintf (stderr, "%s: not found: %s\n", pgm, optarg);
              return -1;
            }
        }
      }
  else
    {
      /* Print everything. */
      for (root_p = name_list; root_p; root_p = root_p->next)
        if (graph_all || !root_p->is_arc_head)
          print_name (root_p, 1);
    }
  return 0;
}


/*
 * print_name: Print one tab for each level of recursion, then the
 *     name of an unvisited adjacency. Go to the adjacency's adjacency
 *     list and, recursively, print the name of an unvisited adjacency.
 *     When the current path is exhausted, back track to an adjacency list
 *     with unvisited nodes and continue with the next unvisited
 *     adjacency.
 *
 *     While travering the DG, maintain an active list of nodes in the
 *     current path. If an active node is revisited, terminate the
 *     path and print a ... cycle ... indicator.
 */
void
print_name (name_node_t *np, int tabc)
{
  static long line = 0;		/* Line number. */

  char *dashes;                 /* Separators for deep nestings. */
  int i, tabd, tabstar, tflag;
  adjacency_node_t *ap;

  if (tabc > maxdepth)
    return;
  printf ("%ld", ++line);
  if (!push_active_node (np))
    printf ("   * nesting is too deep\n");
  else
    {
      tabstar = 0;
      tabd = tabc;
      for (; tabd > ntabs; tabstar++)
        tabd = tabd - ntabs;
      if (tabstar > 0)
        {
          printf (" ");
          for (i = 0; i < tabstar; i++)
            printf ("<");
        }
      if (tabd == 0)
        printf ("   ");
      else
        for (i = 0; i < tabd; i++)
          printf ("\t");

      /* cycle found */
      if (is_active_node (np))
        printf ("... %s ... {%ld}\n", np->adjacency_name, np->name_visited);
      else
        {
          if (np->adjacency_list)
            {
              printf ("%s", np->adjacency_name);
              ap = np->adjacency_list->next;
              if (expand_all || !np->name_visited)
                {
                  printf (" %s\n", np->adjacency_label);
                  ++tabc;
                  if (!np->name_visited)
                    np->name_visited = line;
                  /* if (tabc > ntabs && tabc % ntabs == 1 && ap) */
                  /*   { */
                  /*     printf("%s\n", dashes); */
                  /*     tflag = 1; */
                  /*   } */
                  /* else */
                  /*   tflag = 0; */
                  for (; ap; ap = ap->next)
                    print_name (ap->name_node_p, tabc);
                  /*                  if (tflag)
                   * {
                   * printf("%s\n", dashes);
                   * tflag = 0;
                   * }
                   */
                }
              else
                printf (" ... {%ld}\n", np->name_visited);
            }

          /* library or external call */
          else
            printf ("%s {}\n", np->adjacency_name);
        }
      pop_active_node ();
    }
  return;
}

name_node_t *active_node[DEPTH_MAX];	/* current path */
int active_p = 0;                       /* current path size */

/*
 * push_active_node: Push the given pointer onto the active_node stack. If
 *     the stack is full, return 0. Otherwise 1.
 */
int
push_active_node (name_node_t *np)
{
  if (active_p < DEPTH_MAX)
    {
      active_node[active_p] = np;
      active_p++;
      return 1;
    }
  return 0;
}

/*
 * pop_active_node: Pop the active_node stack.
 */
void
pop_active_node (void)
{
  if (active_p)
    active_node[active_p--] = 0;
}

/*
 * is_active_node: Search for the given pointer on the active_node stack.
 *     Return 1 if found, otherwise 0.
 */
int
is_active_node (name_node_t *np)
{
  int i;

  for (i = 0; i < active_p - 1; i++)
    if (np == active_node[i])
      return 1;
  return 0;
}

/*
 * get_name_list_node: Search for the given name on the name list. If
 *     found, return the matching name node. Otherwise return NULL.
 */

name_node_t *
get_name_list_node (char *s)
{
  name_node_t *np = name_list;

  if (strlen (s) > NAME_MAX - 1)
    s[NAME_MAX - 1] = '\0';

  for (np = name_list; np; np = np->next)
    if (strcmp (s, np->adjacency_name) == 0)
      return np;
  return 0;
}
