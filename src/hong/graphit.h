#ifndef GRAPHIT_H
#define GRAPHIT_H

typedef struct {
	int number_of_nodes; //number of nodes
	int deg; //degree
	int* A; // entries d*(n-1)+1 to dn return the nodes, which are connected to node n
} d_graph;


typedef struct {
	d_graph d_graph; //
  double* X; // cartesian coordinates of every node n
  double* Y; // cartesian coordinates of every node n
} c_graph;


c_graph c_graph_init(d_graph g, int k);
void c_graph_deinit(c_graph g);

void do_layout(c_graph g);

void output_pdf(c_graph g);
void output_sdl(c_graph g);

#endif //GRAPHIT_H
