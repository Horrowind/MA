#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "SDL.h"

#include "graphit.h"

double pi = 3.14159265358979323; 
double cool(int i, int n);
double min(double x, double y);
d_graph d_minigraph();
c_graph c_minigraph();
void move(c_graph g, int n, double* force_x, double* force_y);
double dist(double x1, double x2, double y1, double y2);

void do_layout(c_graph g) {

	double* force_x;  // force_x[i] returns the force in x-direction for node i, see algorithm.pdf
	double* force_y;  // force_y[i] returns the force in y-direction for node i, see algorithm.pdf
	
	force_x = malloc(g.d_graph.number_of_nodes*sizeof(double));
	force_y = malloc(g.d_graph.number_of_nodes*sizeof(double));
	
	for (int i = 1; i<2; i++){
		move(g, i, force_x, force_y);
	}
	
	free(force_x);
	free(force_y);
}

// single move routine -------------------------------------------------

void move(c_graph g, int n, double* force_x, double* force_y){
	int degree = g.d_graph.deg;
	int node_length = g.d_graph.number_of_nodes;
	double constant = pow(node_length/pi, 0.5);

	// Step I:
	memset(force_x, 0, node_length * sizeof(double));
	memset(force_y, 0, node_length * sizeof(double)); 

	// Step II:
	for (int i = 0; i < node_length; i++){
		for (int j = 0; j < degree; j++){
			
			double distance = dist(g.X[i], g.Y[i], g.X[g.d_graph.A[degree*i+j]], g.Y[g.d_graph.A[degree*i+j]]);
			//					printf("%i, %i, %f , %f, %f, loop, distance: %f \n", i, j, constant, force_x[i], force_y[i], distance);
			//			printf("%f, %f \n", g.X[i], g.Y[i]);
			force_x[i] -= constant * (g.X[i]- g.X[g.d_graph.A[degree*i+j]]) * distance * distance;
			force_y[i] -= constant * (g.Y[i]- g.Y[g.d_graph.A[degree*i+j]]) * distance * distance;
		}
	}

	// Step III:

	double c = cool(n, node_length);
	// 3 is the number of boundary nodes
	for (int i = 3; i < node_length; i++){

		double norm = dist(0, 0, force_x[i], force_y[i]);
		if (norm == 0) {
			g.X[i] = 0;
			g.Y[i] = 0;
		} else {
			g.X[i] +=  min(norm, c) * force_x[i]/norm; 
			g.Y[i] +=  min(norm, c) * force_y[i]/norm; 
			//printf("Iteration %i \n", i);
		}
		//printf("%f, %f \n", g.X[i], g.Y[i]);
	}

	return;
}

// initializing and deinitialization -------------------------------------
// k: number of nodes on the unit circle (boundary nodes)
c_graph c_graph_init(d_graph g, int k){
	c_graph h;
	h.X = malloc(g.number_of_nodes*sizeof(double));
	h.Y = malloc(g.number_of_nodes*sizeof(double));	

	for (int i = 0; i < k; i++ ){
		h.X[i] = cos(2*pi*i/k);
		h.Y[i] = sin(2*pi*i/k);
	}
	for (int i = k; i < g.number_of_nodes; i++){
		h.X[i] = 0;
		h.Y[i] = 0;

	}
	h.d_graph = g;
	return h;
}

void c_graph_deinit(c_graph g){
	free(g.X);
	free(g.Y);
}

// utilities ------------------------------------------------------------

double min(double x, double y){
	if (x >= y) {
		return y;
	} else {
		return x;
	}
}

double dist(double x1, double x2, double y1, double y2){
	double d;
	d = sqrt((x1 - y1)*(x1 - y1) + (x2 - y2)*(x2 - y2));
	return d;
}

double cool(int i, int n){
	double result;
	result = pow(pi/n, 0.5)/(1 + pi/n*pow(i, 1.5));
	return result;
}

// Example ---------------------------------------------------------------------------

d_graph d_minigraph(){ 
	d_graph g;
	g.number_of_nodes = 11;
	//g.number_of_nodes = 5;
	g.deg = 4;
	// d_graph* h = &g; h->A is the same as (*g).A, h[i] is the same as *(h + i)
	g.A = malloc(g.deg*g.number_of_nodes*sizeof(int)); // allocate(A[d*n]), if A(:) :: int
	for (int i = 0; i < g.number_of_nodes; i++) {	
		g.A[g.deg*i+0] = (i+1)%(g.number_of_nodes);
		g.A[g.deg*i+1] = (i+2)%(g.number_of_nodes);
		g.A[g.deg*i+2] = (i+3)%(g.number_of_nodes);		
	}
	/* g.A[0] = 1; */
	/* g.A[1] = 2; */
	/* g.A[2] = 3; */
	/* g.A[3] = 0; */
	/* g.A[4] = 2; */
	/* g.A[5] = 4; */
	/* g.A[6] = 0; */
	/* g.A[7] = 1; */
	/* g.A[8] = 5; */
	/* g.A[9] = 0; */
	/* g.A[10] = 4; */
	/* g.A[11] = 5; */
	/* g.A[12] = 1; */
	/* g.A[13] = 3; */
	/* g.A[14] = 5; */
	/* g.A[15] = 3; */
	/* g.A[16] = 4; */
	/* g.A[17] = 2; */
	
	g.A[0] = 1;
	g.A[1] = 2;
	g.A[2] = 8;
	g.A[3] = 7;
	g.A[4] = 0;
	g.A[5] = 2;
	g.A[6] = 3;
	g.A[7] = 4;
	g.A[8] = 0;
	g.A[9] = 1;
	g.A[10] = 6;
	g.A[11] = 5;

	g.A[12] = 9;
	g.A[13] = 8;
	g.A[14] = 4;
	g.A[15] = 1;

	g.A[16] = 3;
	g.A[17] = 10;
	g.A[18] = 5;
	g.A[19] = 1;
	
	g.A[20] = 4;
	g.A[21] = 10;
	g.A[22] = 6;
	g.A[23] = 2;

	g.A[24] = 7;
	g.A[25] = 10;
	g.A[26] = 5;
	g.A[27] = 2;

	g.A[28] = 0;
	g.A[29] = 8;
	g.A[30] = 9;
	g.A[31] = 6;

	g.A[32] = 0;
	g.A[33] = 7;
	g.A[34] = 9;
	g.A[35] = 3;

	g.A[36] = 7;
	g.A[37] = 10;
	g.A[38] = 3;
	g.A[39] = 8;

	g.A[40] = 9;
	g.A[41] = 4;
	g.A[42] = 5;
	g.A[43] = 6;

	return g;
}

c_graph c_minigraph(){
	c_graph tetrahedron;
	tetrahedron.d_graph = d_minigraph();
	tetrahedron.X = malloc(tetrahedron.d_graph.number_of_nodes*sizeof(double));
	tetrahedron.Y = malloc(tetrahedron.d_graph.number_of_nodes*sizeof(double));

	/* for (int i = 0; i < tetrahedron.d_graph.number_of_nodes; i++) {	 */
	/* 	tetrahedron.X[i] = ((double)i)/((double)tetrahedron.d_graph.number_of_nodes ); */
	/* 	tetrahedron.Y[i] = (double)(i+1)/(double)(tetrahedron.d_graph.number_of_nodes*i+1); */
	/* } */
	
	tetrahedron.X[0] = -0.5;
	tetrahedron.Y[0] = -0.5*sqrt(3.0);
	tetrahedron.X[1] = 1;
	tetrahedron.Y[1] = 0;
	tetrahedron.X[2] = -0.5;
	tetrahedron.Y[2] = 0.5*sqrt(3.0);

	double eps;
	eps = -0.1;
	
	tetrahedron.X[3] = -0.5*eps;
	tetrahedron.Y[3] = -0.5*sqrt(3.0)*eps;
	tetrahedron.X[4] = 1*eps;
	tetrahedron.Y[4] = 0*eps;
	tetrahedron.X[5] = -0.5*eps;
	tetrahedron.Y[5] = 0.5*sqrt(3.0)*eps;

	tetrahedron = c_graph_init(tetrahedron.d_graph, 3);
	return tetrahedron;
}

// Window output routine -----------------------------------------------

void output_sdl(c_graph g) {
	SDL_Window *win = NULL;
	SDL_Renderer *renderer = NULL;
	int posX = 100, posY = 100, width = 320, height = 320;

	SDL_Init(SDL_INIT_VIDEO);

	win = SDL_CreateWindow("Hello World", posX, posY, width, height, 0);

	renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

	while (1) {
		SDL_Event e;
		if (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				break;
			}
		}

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 255, 73, 123, 255);

		for (int i = 0; i<g.d_graph.number_of_nodes; i++){
			for (int j = 0; j < g.d_graph.deg; j++) {
				float x1 = g.X[i]*(width/2) +width/2;
				float y1 = g.Y[i]*(-height/2) +height/2;
						
				float x2 = g.X[g.d_graph.A[g.d_graph.deg*i+j]]*(width/2) +width/2;
				float y2 = g.Y[g.d_graph.A[g.d_graph.deg*i+j]]*(-height/2)+height/2;
						
				SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
			}
		}
				
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(win);

	SDL_Quit();

}



// PDF Output routine --------------------------------------------------

void output_pdf(c_graph g ){
	FILE *f = fopen("output_graph.tex", "w");
	if (f == NULL)
		{
			printf("Error opening file output_graph!\n");
			exit(1);
		}
	// print latex-code 
	fprintf(f,
					"\\documentclass[a4paper]{article}\n"
					"\\usepackage{tikz}\n"
					"\\begin{document}\n"
					"\\begin{figure}[h!]\n"
					"\\centering\n"
					"\\begin{tikzpicture}[scale = 4]\n"
					);
	// print tikz-code for nodes
	for (int i = 0; i<g.d_graph.number_of_nodes; i++){
		fprintf(f, "\\coordinate (x%i) at (%f, %f);\n", i, g.X[i], g.Y[i]);
	}
	// print tikz-code for connection lines
	for (int i = 0; i<g.d_graph.number_of_nodes; i++){
		for (int j = 0; j < g.d_graph.deg; j++) {
			fprintf(f,"\\draw (x%i) -- (x%i);\n", i, g.d_graph.A[g.d_graph.deg*i + j]);
		}		
	}
	// print latex-code 
	fprintf(f, "\\end{tikzpicture}\n"
					"\\caption{Graph}\n"
					"\n"
					"\\end{figure}\n"
					"\\end{document}\n"
					);

	fclose(f);
	// create pdf
	//system("pdflatex output_graph.tex");
}
