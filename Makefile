all: findeq.c findeq.h
	gcc -o main -pthread findeq.c
