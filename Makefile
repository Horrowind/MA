TEXMFOUTPUT:=build

build/ma_main.pdf: *.tex
	mkdir -p build
	# latexmk -bibtex build/ma_main
	latexmk -bibtex -pdf -jobname=build/output -pdflatex="pdflatex -interaction=nonstopmode -halt-on-error -file-line-error" \
		-use-make ma_main.tex

all: build/ma_main.pdf
	# pdflatex -halt-on-error -file-line-error -output-directory=build ma_main.tex
	# bibtex build/ma_main
	# pdflatex -halt-on-error -file-line-error -output-directory=build --synctex=1 ma_main.tex
