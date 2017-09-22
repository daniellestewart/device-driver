# CSCI 5103 Fall 2016
# Assignment # 7
# name: Alexander Cina, Danielle Stewart
# student id: 4515522, <Add ID Here>
# x500 id: cinax020, dkstewar
# CSELABS machine: csel-x07-umh
# 
# This is a makefile for compiling the producer and
# consumer processes to test the scullbuffer device.

all: prodcon.c
	gcc -Wall -Wextra -Wpedantic prodcon.c -o prodcon.out
debug: prodcon.c
	gcc -DSCULL_DEBUG -Wall -Wpedantic -Wextra prodcon.c -o prodcon.out
clean:
	rm prodcon.out
	rm test[0-9]*_log.txt
