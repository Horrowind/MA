WARNINGS := -Wall -Wno-unused-function

all:
	gcc -g $(WARNINGS) -std=c11 -O0 -lpthread build.c -lSDL2 -lm

release:
	gcc $(WARNINGS) -std=c11 -march=native -O3 -DNDEBUG -lpthread build.c -lSDL2 -lm

musl:
	musl-gcc $(WARNINGS) -std=c11 -O3 build.c

preprocess:
	gcc -E -P -std=c11 -O0 -E build.c | clang-format -style=Mozilla > preprocessed.c
	gcc -g $(WARNINGS) -std=c11 -O0 -lpthread preprocessed.c -lSDL2 -lm
