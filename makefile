all: chord
chord.o: chord.c
	gcc -c chord.c
Sha1.o: Sha1.c
	gcc -c Sha1.c
chord: chord.o Sha1.o
	gcc -o chord chord.o Sha1.o -lm
clean: chord chord.o Sha1.o
	rm chord chord.o Sha1.o


