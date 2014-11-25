CFLAGS=-Wall -Wno-unused-function -O3 -c $(DEBUG)

LFLAGS=-lm

.PHONY: debug remake clean

run: main.o organism.o map.o
	gcc $(DEBUG) main.o organism.o map.o -o run $(LFLAGS)

visualizer: visualizer.c
	gcc -O3 -Wall $(DEBUG) visualizer.c -o visualizer -lGL -lglut

debug:
	$(MAKE) DEBUG="-g -O0"

remake:
	$(MAKE) clean
	$(MAKE)

main.o: main.c globals.h organism.h map.h
	gcc $(CFLAGS) main.c

organism.o: organism.c globals.h
	gcc $(CFLAGS) organism.c

map.o: map.c globals.h organism.h main.h map.h
	gcc $(CFLAGS) map.c

clean:
	rm -f *.o game game.exe
