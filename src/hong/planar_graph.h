#ifndef PLANAR_GRAPH_H
#define PLANAR_GRAPH_H

#include "params.h"
#include "boundary.h"
#include "pool.h"

typedef struct planar_graph_builder_edge {
    int vertex1;
    int vertex2;
    int face1;
    int face2;
    int prev;
    int next;
} planar_graph_builder_edge_t;

#define INNER_FACE (0)
#define OUTER_FACE (-1)
typedef struct {
    boundary_t boundary;
    int face_count;
    int vertex_count;
    int current_outer_edge;
    pool_t edges_pool;    
} planar_graph_builder_t;


#define MAX_NGON 16

typedef struct planar_graph_vertex planar_graph_vertex_t;
typedef struct planar_graph_edge planar_graph_edge_t;
typedef struct planar_graph_face planar_graph_face_t;

struct planar_graph_vertex {
    planar_graph_edge_t* edges[VALENCE];
    b32 is_boundary;
    double x;
    double y;
};

struct planar_graph_edge {
    planar_graph_vertex_t* vertex1;
    planar_graph_vertex_t* vertex2;
    planar_graph_face_t* face1;
    planar_graph_face_t* face2;
};

struct planar_graph_face {
    int ngon;
    planar_graph_vertex_t* vertices[MAX_NGON];
    planar_graph_edge_t* edges[MAX_NGON];
};

typedef struct {
    boundary_t boundary;
    planar_graph_vertex_t* vertices;
    int vertex_count;
    planar_graph_edge_t* edges;
    int edge_count;
    planar_graph_face_t* faces;
    int face_count;
} planar_graph_t;

void planar_graph_to_dotty(planar_graph_builder_t g, int line_num);

static
void planar_graph_builder_from_boundary(planar_graph_builder_t* planar_graph, boundary_t boundary);

static inline
void planar_graph_builder_rotl(planar_graph_builder_t* planar_graph, int shift);

static inline
void planar_graph_builder_rotr(planar_graph_builder_t* planar_graph, int shift);

static
void planar_graph_builder_normalize(planar_graph_builder_t* planar_graph);

static
void planar_graph_builder_check_outer_edges(planar_graph_builder_t* planar_graph);

static inline
void planar_graph_builder_insert(planar_graph_builder_t* planar_graph, int ngon, int allow_overlap);

static inline
void planar_graph_remove(planar_graph_builder_t* planar_graph, int ngon, int allow_overlap);

planar_graph_t planar_graph_from_builder(planar_graph_builder_t builder);

void planar_graph_deinit(planar_graph_t graph);

void planar_graph_layout_step(planar_graph_t* graph, int step, double* force_x, double* force_y);

void planar_graph_layout(planar_graph_t* graph);

int planar_graph_output_sdl(planar_graph_t graph);
void planar_graph_output_tikz(planar_graph_t graph);

#endif //PLANAR_GRAPH_H
