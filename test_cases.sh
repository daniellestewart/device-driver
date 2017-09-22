#!/bin/bash
# CSCI 5103 Fall 2016
# Assignment # 7
# name: Alexander Cina, Danielle Stewart
# student id: 4515522, <Add ID Here> 
# x500 id: cinax020, dkstewar
# CSELABS machine: csel-x07-umh

# This is a script for running the test cases
# in producer.c

# If executable doesn't exist, make it
if [ ! -e prodcon.out ]; then
	make -f MakeTest.make debug
fi

# Run each test
for n in {1..5}
do
	echo "Running ./prodcon.out $n"
	sleep .1
	./prodcon.out $n
	echo "Finished running ./prodcon.out $n"
done
