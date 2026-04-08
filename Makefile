all: tetris

tetris: main.o
	gcc -fsanitize=address -g -O0 -o tetris main.o -lasan

clean:
	rm -f *.o tetris