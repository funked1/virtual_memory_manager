CC = gcc

default: all

all:
	$(CC) -o main main.c
	$(CC) -o main_pr main_pr.c

clean:
	rm main
	rm main_pr
