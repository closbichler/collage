all: compile

compile:
	gcc -o collage collage.c -Wall -lm
	
clean:
	rm collage