#include <stdio.h>
#include <stdlib.h>
#include <math.h>
int main(int argc, char** argv) {
	if(argc == 0) return -1;
	double f = atof(argv[1]);
	for(int i = -5; i <= 5; i++) {
		for(int j = -5; j <= 5; j++) {
			float x2 = sqrt(3.0) * f * (float)i + (j % 2 ? sqrt(3.0) / 2 * f : 0);
			float x1 = 1.5 * f * (float)j;
			printf("{(%f, %f)}, ", x1, x2);
		}

	}

	return 0;
}
