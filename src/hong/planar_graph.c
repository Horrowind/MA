#include <SDL2/SDL.h>
#include "planar_graph.h"

#define PI 3.14159265358979
#define TAU (2 * PI)
#define EPS (1.0d / 4096.0d)
#define MIN(x, y) ((x < y) ? (x) : (y))

void planar_graph_to_dotty(planar_graph_builder_t g, int line_num) {
    
    int edge_count = g.edges_pool.fill / sizeof(planar_graph_builder_edge_t);
    planar_graph_builder_edge_t* edges = (planar_graph_builder_edge_t*)g.edges_pool.data;

    FILE* fp = fopen("/tmp/t.dot", "w");
    fprintf(fp, "digraph g {\n");
    fprintf(fp, "  L%i\n", line_num);
    fprintf(fp, "  B");
    for(int i = g.boundary.size - 1; i >= 0; i--) {
        fprintf(fp, "%i", (int)((g.boundary.bits >> i) & 1));
    }
    fprintf(fp, "\n");

    for(int i = 0; i < edge_count; i++) {
        if(i == g.current_outer_edge) {
            fprintf(fp, "  E%u[color=blue, shape=box, height=0.3, width=0.3]\n", i);
        } else {
            fprintf(fp, "  E%u[shape=box, height=0.3, width=0.3]\n", i);
        }
        fprintf(fp, "  E%u ->  V%u[weight=5]\n", i, edges[i].vertex1);
        fprintf(fp, "  E%u ->  V%u[weight=5]\n", i, edges[i].vertex2);
        fprintf(fp, "  E%u ->  E%u[color=blue]\n", i, edges[i].prev);
        fprintf(fp, "  E%u ->  E%u\n", i, edges[i].next);
        
    }
    
    fprintf(fp, "}\n");
    
    fclose(fp);
    system2("cat /tmp/t.dot | sfdp -Txdot  | dotty -");
}

static
void planar_graph_builder_from_boundary(planar_graph_builder_t* planar_graph, boundary_t boundary) {
    planar_graph->boundary = boundary;
    planar_graph->face_count = 1;
    planar_graph->vertex_count = boundary.size;
    pool_init(&planar_graph->edges_pool);
    planar_graph_builder_edge_t* edges = (planar_graph_builder_edge_t*)pool_alloc(
        &planar_graph->edges_pool, boundary.size * sizeof(planar_graph_builder_edge_t));
    planar_graph->current_outer_edge = 0;
    for(int i = 0; i < boundary.size; i++) {
        edges[i].vertex1 = i;
        edges[i].vertex2 = (i + 1) % boundary.size;
        edges[i].face1 = INNER_FACE;
        edges[i].face2 = OUTER_FACE;
        edges[i].prev = (boundary.size + i - 1) % boundary.size;
        edges[i].next = (boundary.size + i + 1) % boundary.size;
    }
}

static inline
void planar_graph_builder_rotl(planar_graph_builder_t* planar_graph, int shift) {
    planar_graph->boundary = boundary_rotl(planar_graph->boundary, shift);
    planar_graph_builder_edge_t* edges = (planar_graph_builder_edge_t*)planar_graph->edges_pool.data;
    for(int i = 0; i < shift; i++) {
        planar_graph->current_outer_edge = edges[planar_graph->current_outer_edge].prev;
    }
}

static inline
void planar_graph_builder_rotr(planar_graph_builder_t* planar_graph, int shift) {
    planar_graph->boundary = boundary_rotr(planar_graph->boundary, shift);
    planar_graph_builder_edge_t* edges = (planar_graph_builder_edge_t*)planar_graph->edges_pool.data;
    for(int i = 0; i < shift; i++) {
        planar_graph->current_outer_edge = edges[planar_graph->current_outer_edge].next;
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

static
void planar_graph_builder_check_outer_edges(planar_graph_builder_t* planar_graph) {
    int outer_edge = planar_graph->current_outer_edge;
    planar_graph_builder_edge_t* edges = (planar_graph_builder_edge_t*)planar_graph->edges_pool.data;
    do {
        /* printf("(%i: %i %i %i %i %i %i) ", outer_edge, */
        /*        edges[outer_edge].vertex1, edges[outer_edge].vertex2, edges[outer_edge].face1, edges[outer_edge].face2, */
        /*        edges[outer_edge].prev, edges[outer_edge].next); */
        assert(edges[outer_edge].vertex1 == edges[edges[outer_edge].prev].vertex2);
        assert(edges[outer_edge].vertex2 == edges[edges[outer_edge].next].vertex1);
        assert(outer_edge == edges[edges[outer_edge].prev].next);
        assert(outer_edge == edges[edges[outer_edge].next].prev);
        outer_edge = edges[outer_edge].next;
    } while(outer_edge != planar_graph->current_outer_edge);
    /* printf("\n"); */
}

//Todo:
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
                int old_edge_count = planar_graph->edges_pool.fill / sizeof(planar_graph_builder_edge_t);
                planar_graph_builder_edge_t* new_edges = (planar_graph_builder_edge_t*)pool_alloc(
                    &planar_graph->edges_pool, (ngon - 1 - s) * sizeof(planar_graph_builder_edge_t));
                planar_graph_builder_edge_t* edges = (planar_graph_builder_edge_t*)planar_graph->edges_pool.data;

                result.bits   = boundary.bits + (1ull << ((boundary.size - 1) * BITS));
                int vertex1 = edges[planar_graph->current_outer_edge].vertex1;

                result.bits >>= s * BITS;
                int new_face = planar_graph->face_count++;
                int outer_edge = planar_graph->current_outer_edge;
                for(int i = 0; i < s; i++) {
                    assert(edges[outer_edge].face2 == OUTER_FACE);
                    edges[outer_edge].face2 = new_face;
                    outer_edge = edges[outer_edge].next;
                }
                int vertex2 = edges[outer_edge].vertex2;

                for(int j = 0; j < ngon - 1 - s; j++) {
                    new_edges[j].vertex1 = planar_graph->vertex_count - 1 + j;
                    new_edges[j].vertex2 = planar_graph->vertex_count     + j;
                    new_edges[j].face1 = new_face;
                    new_edges[j].face2 = OUTER_FACE;
                    new_edges[j].next = old_edge_count + j + 1;
                    new_edges[j].prev = old_edge_count + j - 1;

                }
                planar_graph->vertex_count += ngon - 2 - s;

                new_edges[0].prev = edges[planar_graph->current_outer_edge].prev;
                new_edges[0].vertex1 = vertex1;

                new_edges[ngon - 2 - s].next = edges[outer_edge].next;
                new_edges[ngon - 2 - s].vertex2 = vertex2;
                
                result.bits  += 1ull;
                result.bits <<= (ngon - 2 - s) * BITS;
                result.size   = boundary.size + ngon - 2 - 2 * s;
                edges[edges[planar_graph->current_outer_edge].prev].next = old_edge_count;
                edges[edges[outer_edge].next].prev = old_edge_count + ngon - 2 - s;
                planar_graph->current_outer_edge = old_edge_count;
                planar_graph->boundary = result;



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
}

static inline
void planar_graph_remove(planar_graph_builder_t* planar_graph, int ngon, int allow_overlap) {

}

static
void planar_graph_unfold(planar_graph_builder_t* planar_graph, int n) {
    
}

planar_graph_t planar_graph_from_builder(planar_graph_builder_t builder) {
    planar_graph_t result;
    result.boundary = builder.boundary;
    int edge_count = builder.edges_pool.fill / sizeof(planar_graph_builder_edge_t);
    planar_graph_builder_edge_t* edges = (planar_graph_builder_edge_t*)builder.edges_pool.data;

    result.vertices = malloc(builder.vertex_count * sizeof(planar_graph_vertex_t));
    result.edges = malloc(edge_count * sizeof(planar_graph_edge_t));
    result.faces = malloc((builder.face_count + 1) * sizeof(planar_graph_face_t));

    memset(result.vertices, 0, builder.vertex_count * sizeof(planar_graph_vertex_t));
    memset(result.edges, 0, edge_count * sizeof(planar_graph_edge_t));
    memset(result.faces, 0, (builder.face_count + 1) * sizeof(planar_graph_face_t));

    result.vertex_count = builder.vertex_count;
    result.edge_count = edge_count;
    result.face_count = builder.face_count;

    
    for(int edge = 0; edge < edge_count; edge++) {
        int vertex1 = edges[edge].vertex1;
        int vertex2 = edges[edge].vertex2;
        int face1 = edges[edge].face1 + 1;
        int face2 = edges[edge].face2 + 1;
        
        // Edges
        result.edges[edge].vertex1 = &result.vertices[vertex1];
        result.edges[edge].vertex2 = &result.vertices[vertex2];
        result.edges[edge].face1 = &result.faces[face1];
        result.edges[edge].face2 = &result.faces[face2];

        // Vertices
        for(int j = 0; j < VALENCE; j++) {
            if(result.vertices[vertex1].edges[j] == NULL) {
                result.vertices[vertex1].edges[j] = &result.edges[edge];
                break;
            }
        }
        for(int j = 0; j < VALENCE; j++) {
            if(result.vertices[vertex2].edges[j] == NULL) {
                result.vertices[vertex2].edges[j] = &result.edges[edge];
                break;
            }
        }

        // Faces
        for(int j = 0; j < MAX_NGON; j++) {
            if(result.faces[face1].edges[j] == NULL) {
                result.faces[face1].edges[j] = &result.edges[edge];
                result.faces[face1].ngon++;
                break;
            }
        }
        for(int j = 0; j < MAX_NGON; j++) {
            if(result.faces[face2].edges[j] == NULL) {
                result.faces[face2].edges[j] = &result.edges[edge];
                result.faces[face2].ngon++;
                break;
            }
        }
    }

    for(int i = 0; i < builder.vertex_count; i++) {
        result.vertices[i].x = 0.2 * cos(TAU * (double)i / builder.boundary.size + PI / builder.boundary.size);
        result.vertices[i].y = 0.2 * sin(TAU * (double)i / builder.boundary.size + PI / builder.boundary.size);
    }
    
    int outer_edge = builder.current_outer_edge;
    for(int i = 0; i < builder.boundary.size; i++) {
        int vertex = edges[outer_edge].vertex1;
        result.vertices[vertex].x = cos(TAU * (double)i / builder.boundary.size + PI / builder.boundary.size);
        result.vertices[vertex].y = sin(TAU * (double)i / builder.boundary.size + PI / builder.boundary.size);
        result.vertices[vertex].is_boundary = 1;
        outer_edge = edges[outer_edge].next;
	}

    return result;
}

void planar_graph_deinit(planar_graph_t graph) {
    free(graph.vertices);
    free(graph.edges);
    free(graph.faces);
}


void planar_graph_layout_step(planar_graph_t* graph, int step, double* force_x, double* force_y) {
    double constant = sqrt(graph->vertex_count / PI);
    for(int i = 0; i < graph->vertex_count; i++) {
        force_x[i] = 0.0d;
        force_y[i] = 0.0d;
        planar_graph_vertex_t* current_vertex = &graph->vertices[i];
        for(int j = 0; j < VALENCE; j++) {
            planar_graph_edge_t* current_edge = graph->vertices[i].edges[j];
            if(current_edge != NULL) {
                planar_graph_vertex_t* other_vertex = current_edge->vertex1 == current_vertex
                    ? current_edge->vertex2
                    : current_edge->vertex1;
                
                double distance_squared =
                    (current_vertex->x - other_vertex->x) * (current_vertex->x - other_vertex->x) +
                    (current_vertex->y - other_vertex->y) * (current_vertex->y - other_vertex->y);
                force_x[i] -= constant * (current_vertex->x - other_vertex->x) * distance_squared;
                force_y[i] -= constant * (current_vertex->y - other_vertex->y) * distance_squared;
            } 
        }
        /* for(int j = 0; j < graph->vertex_count; j++) { */
        /*     planar_graph_vertex_t* current_vertex = &graph->vertices[i]; */
        /*     planar_graph_vertex_t* other_vertex = &graph->vertices[j]; */
        /*     double distance_squared = */
        /*         (current_vertex->x - other_vertex->x) * (current_vertex->x - other_vertex->x) + */
        /*         (current_vertex->y - other_vertex->y) * (current_vertex->y - other_vertex->y); */
        /*     if(distance_squared < 0.01) { */
        /*         force_x[i] += constant * (current_vertex->x - other_vertex->x) * 0.1; */
        /*         force_y[i] += constant * (current_vertex->y - other_vertex->y) * 0.1; */
        /*     } */
        /* }    */
    }

    double c = sqrt(PI / (double)step) / (1.0d + PI / (double)graph->vertex_count * pow((double)step, 1.5d));

    for (int i = 0; i < graph->vertex_count; i++) {
        planar_graph_vertex_t* current_vertex = &graph->vertices[i];
        double force_norm = sqrt(force_x[i] * force_x[i] + force_y[i] * force_y[i]);
        if(!current_vertex->is_boundary && force_norm > EPS) {
            double dx = MIN(force_norm, c) * force_x[i] / force_norm;
            double dy = MIN(force_norm, c) * force_y[i] / force_norm;
            current_vertex->x += dx;
            current_vertex->y += dy;
        }
    }
}

void planar_graph_layout(planar_graph_t* graph) {
    double* force_array = malloc(graph->vertex_count * sizeof(double) * 2);
    double* force_x = &force_array[0];
    double* force_y = &force_array[graph->vertex_count];

    for(int step = 1; step < 200; step++) {
        planar_graph_layout_step(graph, step, force_x, force_y);
    }
    free(force_array);
}


int planar_graph_output_sdl(planar_graph_t graph) {
	SDL_Window *win = NULL;
	SDL_Renderer *renderer = NULL;
	int posX = 100, posY = 100, width = 600, height = 600;

	SDL_Init(SDL_INIT_VIDEO);

	win = SDL_CreateWindow("Planar graph", posX, posY, width, height, 0);

	renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    double* force_array = malloc(graph.vertex_count * sizeof(double) * 2);
    double* force_x = &force_array[0];
    double* force_y = &force_array[graph.vertex_count];
    
    int step = 1;
    int result = 0;
	while (1) {
		SDL_Event e;
		if (SDL_PollEvent(&e)) {
			if(e.type == SDL_QUIT) {
			    exit(0);
			}
            if(e.type == SDL_KEYDOWN) {
			    if(e.key.keysym.sym == SDLK_ESCAPE) {
                    result = 0;
                    break;
                }
                if(e.key.keysym.sym == SDLK_RETURN) {
                    result = 1;
                    break;
                }

			}

		}

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 255, 73, 123, 255);

        planar_graph_layout_step(&graph, step, force_x, force_y);
        step++;
        
		for(int i = 0; i < graph.vertex_count; i++){
            planar_graph_vertex_t* current_vertex = &graph.vertices[i];
            for(int j = 0; j < VALENCE; j++) {
                planar_graph_edge_t* current_edge = graph.vertices[i].edges[j];
                if(current_edge != NULL) {
                    planar_graph_vertex_t* other_vertex = current_edge->vertex1 == current_vertex
                        ? current_edge->vertex2
                        : current_edge->vertex1;

                    double x1 = current_vertex->x * (double)(width / 2) + (double)(width / 2);
                    double y1 = current_vertex->y * (double)(height / 2) + (double)(height / 2);
                    double x2 = other_vertex->x * (double)(width / 2) + (double)(width / 2);
                    double y2 = other_vertex->y * (double)(height / 2) + (double)(height / 2);
                    SDL_RenderDrawLine(renderer, (int)x1, (int)y1, (int)x2, (int)y2);
                    SDL_Rect rect = { .x = (int)x1 - 1, .y = (int)y1 - 1, .w = 3, .h = 3 };
                    SDL_RenderFillRect(renderer, &rect);
                }
			}
		}

		/* SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); */
        /* for(int i = 0; i < graph.boundary.size; i++) { */
        /*     int vertex_valence = boundary_rotl(graph.boundary, i).bits & LAST_NODE_MASK; */
        /*     for(int j = 1; j < vertex_valence + 1; j++) { */
        /*         double c = cos(-TAU * (double)i / graph.boundary.size + 3 * PI / graph.boundary.size); */
        /*         double s = sin(-TAU * (double)i / graph.boundary.size + 3 * PI / graph.boundary.size); */
        /*         double f = 1.0d - (0.02 * (double)j); */
        /*         double x = f * c * (double)(width / 2) + (double)(width / 2); */
        /*         double y = f * s * (double)(height / 2) + (double)(height / 2); */
        /*         SDL_Rect rect = { .x = (int)x - 1, .y = (int)y - 1, .w = 3, .h = 3 }; */
        /*         SDL_RenderFillRect(renderer, &rect); */
        /*     } */
        /* } */
				
		SDL_RenderPresent(renderer);
	}

    free(force_array);
    
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(win);

	SDL_Quit();
    return result;
}
