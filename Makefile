TEXMFOUTPUT:=build

all: *.tex
	mkdir -p build
	pdflatex -halt-on-error -file-line-error -output-directory=build ma_main.tex
	bibtex build/ma_main
	pdflatex -halt-on-error -file-line-error -output-directory=build ma_main.tex
