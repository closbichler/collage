all: compile

clean:
	rm collage

compile:
	gcc -o collage collage-cli.c collage.c -Wall -lm
	
test:
	./collage shrink fotos/nehammer.jpg test-shrink.jpg
	./collage fun fotos/nehammer.jpg test-fun.jpg
	./collage single fotos/nehammer.jpg test-single.jpg
	./collage multi fotos/nehammer.jpg fotos/nehammer/ test-multi.jpg 2000x2000 50