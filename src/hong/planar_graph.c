#include "params.h"
#include "boundary.h"
#include "pool.h"

typedef struct planar_graph_edge {
    int vertex1;
    int vertex2;
    int face1;
    int face2;
    struct planar_graph_edge* next;
    struct planar_graph_edge* prev;
} planar_graph_builder_edge_t;

#define INNER_FACE (0)
#define OUTER_FACE (-1)
typedef struct {
    boundary_t boundary;
    int face_count;
    int vertex_count;
    planar_graph_builder_edge_t* current_outer_edge;
    pool_t edges_pool;    
} planar_graph_builder_t;

static
void planar_graph_builder_from_boundary(planar_graph_builder_t* planar_graph, boundary_t boundary) {
    planar_graph->boundary = boundary;
    planar_graph->face_count = 1;
    planar_graph->vertex_count = boundary.size;
    pool_init(&planar_graph->edges_pool);
    planar_graph_builder_edge_t tmp_edge; 
    planar_graph_builder_edge_t* last_edge = &tmp_edge; 
    for(int i = 0; i < boundary.size; i++) {
        planar_graph_builder_edge_t* edge = (planar_graph_builder_edge_t*)pool_alloc(
            &planar_graph->edges_pool, sizeof(planar_graph_builder_edge_t));

        edge->vertex1 = i;
        edge->vertex2 = (i + 1) % boundary.size;
        edge->face1 = INNER_FACE;
        edge->face2 = OUTER_FACE;
        edge->prev = last_edge;
        last_edge->next = edge;
        last_edge = edge;
    }
    tmp_edge.next->prev = last_edge;
    last_edge->next = tmp_edge.next;
}

static inline
void planar_graph_builder_rotl(planar_graph_builder_t* planar_graph, int shift) {
    planar_graph->boundary = boundary_rotl(planar_graph->boundary, shift);
    for(int i = 0; i < shift; i++) {
        planar_graph->current_outer_edge = planar_graph->current_outer_edge->next;
    }
}

static
void planar_graph_builder_normalize(planar_graph_builder_t* planar_graph) {
    int max_rotation = 0;
    boundary_t max_boundary = planar_graph->boundary;
    for(int i = 0; i < planar_graph->boundary.size; i++) {
        boundary_t new_boundary = boundary_rotl(planar_graph->boundary, i);
        if(max_boundary.bits < new_boundary.bits) {
            max_boundary = new_boundary;
            max_rotation = i;
        }
    }
    planar_graph_builder_rotl(planar_graph, max_rotation);
}

static inline
void planar_graph_builder_insert(planar_graph_builder_t* planar_graph, int ngon, int allow_overlap) {
    boundary_t result = {.bits = 0, .size = 0};
    boundary_t boundary = planar_graph->boundary;
    if((boundary.bits >> (BITS * (boundary.size - 1))) < VALENCE - 2) {
        u32 s;
        if(VALENCE != 4) {
            s = __builtin_ctzll(~boundary.bits) / BITS;
        } else {
            s = __builtin_ctzll(~(boundary.bits | 0x5555555555555555)) / BITS;
        }
        if(s != boundary.size - 1) {
            if(s <= ngon - 2) {
                result.bits   = boundary.bits + (1ull << ((boundary.size - 1) * BITS));
                result.bits >>= s * BITS;
                result.bits  += 1ull;
                result.bits <<= (ngon - 2 - s) * BITS;
                result.size   = boundary.size + ngon - 2 - 2 * s;
            }
        } else if(allow_overlap) {
            if((boundary.bits >> (BITS * (boundary.size - 1))) < VALENCE - 3) {
                if(boundary.size + 1 <= ngon) {
                    result.bits   = (boundary.bits >> (BITS * (boundary.size - 1))) + 2;
                    result.bits <<= (ngon - boundary.size - 1) * BITS;
                    result.size   = ngon - boundary.size;
                }
            } else {
                if(boundary.size + 3 <= ngon) {
                    result.bits   = 1 << (ngon - boundary.size - 3) * BITS;
                    result.size   = ngon - boundary.size - 2;
                }
            }
        }        
    }
    return result;
}

static inline
void planar_graph_remove(planar_graph_builder_t* planar_graph, int ngon, int allow_overlap) {

}

static
void planar_graph_unfold(planar_graph_builder_t* planar_graph, int n) {

}
