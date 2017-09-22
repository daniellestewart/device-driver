# CSCI5103
A repository for the Device Drivers project for CSCI 5103

This project is a linux device driver where the device is a region of memory.
The memory is treated as bounded buffer containing 32 byte items, each read
and written one at a time.

## Compiling
To compile the device driver simply type:
make
or type
make -f Makefile 

## Installing
To install the module run
./install_device.sh
To specify a number of items in the buffer instead of using the default (20) run
./install_device nitems=N
where N is the desired size of the buffer.
NOTE: There must always be one empty space in the buffer, so please run with N greater than
or equal to 2.

## Testing
To run all 5 tests of the device driver run
./test_cases.sh
This will automatically compile the producer/consumer test code if it has not 
already been compiled. It will then run all 5 tests. To compile the test cases
without running the test_cases.sh script, run the command
make -f MakeTest.make
To compile with debug messages enabled, run the command
make -f MakeTest.make debug
To run an individual test case run
./prodcon.out n
where n is replaced with the test number you would like to run [1-5].
To remove log files and the prodcon.out executable, run the command
make -f MakeTest.make clean
To run tests several times (for checking for rare errors) you can run
./multiple_test.sh
which will run the test several times.

## Removing
To remove the module run
./remove_device.sh

## Files
Below is a list of files and a short description of each one.

- install_device.sh: Install script for installing the driver

- item.h: Describes a struct for representing items as 32 consecutive bytes
Also describes some useful functions for user-level programs to work with
items. To gain access to the user functions, #define USER_LEVEL right above 
the #include "item.h" line.

- Makefile: Makefile for compiling the scullbuffer device driver.

- MakeTest.make: Makefile for compiling the producer/consumer test code.

- multiple_test.sh: Simple script for running test_cases.sh several times in a row.

- prodcon.c: Source file for the producer/consumer test code.

- prodcon.h: Header file containing a usefull debug macro for user programs.

- README.md: Describes the files, as well as how to compile, use, and test the
device driver.

- remove_device.sh: Script for removing the device driver when it is no longer
 to be used.

- scullbuffer.c: Source file for the scullbuffer device driver.

- scullbuffer.h: Header file for the scullbuffer device driver.

- test_cases.sh: Script for running all 5 test cases with a single command.
