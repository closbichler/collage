all: compile

clean:
	rm collage

compile:
	gcc -o collage collage-cli.c collage.c -Wall -lm
	
test:
	./collage shrink photos/nehammer.jpg test-shrink.jpg
	./collage single photos/nehammer.jpg test-single.jpg 0
	./collage multi photos/nehammer.jpg photos/nehammer/ test-multi.jpg 2000x2000 50