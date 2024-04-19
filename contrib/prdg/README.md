# Call-graph Printer

The prdg program reads from standard input an adjacency list describing a
directed graph (DG) and prints a textual representation that may
include cycles. Algorithm-wise, it performs a
[topological sort](https://en.wikipedia.org/wiki/Topological_sorting#:~:text=In%20computer%20science%2C%20a%20topological,before%20v%20in%20the%20ordering.)
by means of a depth-first or preorder traversal of the DG.

Standard input is expected to contain space-separated name pairs, one
per line, interpreted as the vertices of an arc. The first name on a
line is taken as the tail of the arc, and any text following the
second forms a label for the first.

For instance, if the names are those of functions in a program, an arc
between two functions, A -> B, might represent a function call, i.e.,
function A invokes B.
