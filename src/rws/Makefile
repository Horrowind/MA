all:
	gcc -O2 rws.c -o rws
debug:
	gcc -g3 -O0 rws.c -o rws
debug_macro:
	gcc -g3 -O0 -E rws.c | grep -v "# " | clang-format > rws.pp.c
	gcc -g3 -O0 -o rws rws.pp.c
