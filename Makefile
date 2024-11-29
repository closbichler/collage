all: compile run

compile:
	gcc -o collage collage.c -Wall -lm

run:
	./collage
	code -r fotos/collage_*.jpg
