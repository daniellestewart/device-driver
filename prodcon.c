/* CSCI 5103 Fall 2016
 * Assignment # 7
 * name: Alexander Cina, Danielle Stewart
 * student id: 4515522, 4339650
 * x500 id: cinax020, dkstewar
 * CSELABS machine: csel-x07-umh
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include "prodcon.h"

// Define USER_LEVEL to gain access to item_t functions
// that make use of the standard library
#define USER_LEVEL
#include "item.h"

#define SLEEP_TIME 2000

/*
 * Write to the scullbuffer n_produce items
 * each of the format "<label><N>" where
 * <label> is replaced with the argument label
 * and <N> is replaced with a number from 0
 * to n_produce - 1, coresponding to the order
 * in which the item was produced.
 */
void producer(int n_produce, char *label) {
	// Open the scullbuffer to an appropriately named file descriptor
	int i;
	int n_bytes; // Number of bytes written
	item_t *itemPtr; // Pointer to hold an allocated item
	char str[32]; // String to hold item's text as it is generated.
	int scull = open("/dev/scullbuffer",O_WRONLY);
	if (scull == -1) {
		perror("Open failed in producer");
		exit(1);
	}
	// Sleep for a short amount of time to give the other process a chance to open the file.
	// A more robust solution would use semaphores to impiment a barrier.
	if (usleep(SLEEP_TIME) != 0) {
		PDEBUG("Error in '%s's' usleep.",label);
	}
	PDEBUG("Producer '%s' opened scullbuffer\n",label);
	// Loop to produce an item, then load it into the buffer
	// Repeat these actions n_produce times.
	for (i=0; i<n_produce; i++) {
		PDEBUG("Entered loop %d in producer %s\n",i,label);
		// Allocate memory for the new item.
		itemPtr = malloc(ITEM_SZ);
		// Build the text of the item.
		sprintf(str,"%s|%d",label,i);
		// Place the item in itemPtr.
		fillItem(itemPtr,str,strlen(str));
		PDEBUG("Producer '%s' produced item %d\n",label,i);
		// Write the item to scullbuffer
		n_bytes = write(scull,itemPtr,ITEM_SZ);
		if (n_bytes == -1) {
			// Error in write syscall
			perror("Write failed in producer");
			free(itemPtr);
			if (close(scull) == -1) {
				perror("Close failed in error handler for producer write");
			}
			exit(1);
		}
		else if (n_bytes == 0) {
			// buffer full and no readers connected t scullbuffer
			free(itemPtr);
			if (close(scull) == -1) {
				perror("Close failed in producer");
				exit(1);
			}
			return;
		}
		else {
			// successful write to scullbuffer
			PDEBUG("Producer '%s' wrote item %d to scull.\n"
				"It was %d bytes.\n",label,i,n_bytes);
		}
		// Release the memory of the item just written.
		free(itemPtr);
	}
	// Close the file descriptor
	if (close(scull) == -1) {
		perror("Close failed in producer");
		exit(1);
	}
	PDEBUG("Producer '%s' closed scullbuffer\n",label);
}

/*
 * Read from the scullbuffer n_produce items
 * and write them all to the log file given
 * by the argument logFile.
 */
void consumer(int n_consume, char *logFile) {
	int i;
	int n_bytes; // Number of bytes read.
	item_t *itemPtr; // pointer to hold the item read from scull
	int scull = open("/dev/scullbuffer",O_RDONLY);
	if (scull == -1) {
		perror("Open scullbuffer failed in consumer");
		exit(1);
	}
	PDEBUG("Consumer '%s' opened scullbuffer\n",logFile);
	// Sleep for a short amount of time to give the other process a chance to open the file.
	// A more robust solution would use semaphores to impliment a barrier.
	if (usleep(SLEEP_TIME) != 0) {
		PDEBUG("Error in '%s's' usleep.",logFile);
	}
	FILE *log = fopen(logFile,"w");
	PDEBUG("Consumer '%s' opened logfile\n",logFile);
	// Loop to consume n_consume items from the scullbuffer.
	for (i=0; i<n_consume; i++) {
		PDEBUG("Consumer '%s' started the loop.\n",logFile);
		// Allocate memory for the pointer
		itemPtr = malloc(ITEM_SZ);
		// Read item from scullbuffer
		PDEBUG("Consumer about to read\n");
		n_bytes = read(scull,itemPtr,ITEM_SZ);
		PDEBUG("Consumer just read %d bytes\n",n_bytes);
		if (n_bytes == -1) {
			// Error in read
			perror("Read failed in consumer");
			free(itemPtr);
			if (close(scull) == -1) {
				perror("Failed to close scullbuffer in error handler of read in consumer");
			}
			exit(1);
		}
		else if (n_bytes == 0) {
			// No bytes to read and no producers to fill scull buffer
			free(itemPtr);
			if (close(scull) == -1) {
				perror("Close failed in producer");
				exit(1);
			}
			return;
		}
		else {
			// Read an item succesfully
			PDEBUG("Consumer '%s' read item %d from scull.\n"
				"It was %d bytes.\n",logFile,i,n_bytes);
		}
		// Write item to logfile
		printItem(itemPtr,log);
		PDEBUG("Consumer '%s' wrote item %d to logfile.\n",logFile,i);
		// Free the memory of the pointer
		free(itemPtr);
	}
	fclose(log);
	PDEBUG("Consumer '%s' closed logFile\n",logFile);
	if (close(scull) == -1) {
		perror("Consumer failed to close scullbuffer");
		exit(1);
	}
	PDEBUG("Consumer '%s' closed scullbuffer\n",logFile);
}

/*
 * These test functions dispatch the appropriate
 * number of producer and consumer processes.
 * Testcase #1: 
 * Start producer which will produce 50 items before exiting
 * Start consumer which will try and consume 50 items before exiting
 * Both producer and consumer should exit normally after prod/con all items
 */
void test1() {
	printf("Running test 1\n");
	// Fork a producer
	pid_t pid = fork();
	if (pid == 0) {
		// Child Process
		PDEBUG("Producer BLACK has pid %d\n",getpid());
		// Run producer
		producer(50,"BLACK");
		exit(0);
	}
	else if (pid < 0) {
		// ERROR IN FORK
		perror("Error in producer fork");
		exit(1);
	}
	// Fork a consumer
	pid = fork();
	if (pid == 0) {
		// Child Process
		PDEBUG("Consumer test1_log has pid %d\n",getpid());
		// Run consumer
		consumer(50,"test1_log.txt");
		exit(0);
	}
	else if (pid < 0) {
		// ERROR IN FORK
		perror("Error in consumer fork");
		exit(1);
	}
	// Wait for producer and consumer child processes
	if (wait(NULL) < 0) {
		perror("Error in wait");
	}
	if (wait(NULL) < 0) {
		perror("Error in wait");
	}
}

/*
* Testcase #2: 
* Start producer which will produce 50 items before exiting
* Start consumer which will consume 10 items before exiting
* After consumer calls release, the producer should not go to sleep
* when trying to write items into the already full buffer, 
* and should instead get a return value of 0 from the
* write() call
*
*/
void test2() {
	printf("Running test 2\n");
	// Fork a producer
	pid_t pid = fork();
	if (pid == 0) {
		// Child Process
		// Run producer
		producer(50,"BLACK");
		exit(0);
	}
	else if (pid < 0) {
		// ERROR IN FORK
		perror("Error in producer fork");
		exit(1);
	}
	// Fork a consumer
	pid = fork();
	if (pid == 0) {
		// Child Process
		// Run consumer
		consumer(10,"test2_log.txt");
		exit(0);
	}
	else if (pid < 0) {
		// ERROR IN FORK
		perror("Error in consumer fork");
		exit(1);
	}
	// Wait for producer and consumer child processes
	if (wait(NULL) < 0) {
		perror("Error in wait");
	}
	if (wait(NULL) < 0) {
		perror("Error in wait");
	}
}

/*
* Testcase #3: 
* Start producer which will produce 50 items before exiting
* Start consumer which will try to consume 100 items before exiting
* After producer calls release, the consumer should not go to sleep
* when trying to write items into the already full buffer, 
* and should instead get a return value of 0 from the
* read() call
*
*/
void test3() {
	int w_num;
	printf("Running test 3\n");
	// Fork a producer
	pid_t pid_p, pid_c;
	pid_p = fork();
	if (pid_p == 0) {
		// Child Process
		// Run producer
		producer(50,"BLACK");
		exit(0);
	}
	else if (pid_p < 0) {
		// ERROR IN FORK
		perror("Error in producer fork");
		exit(1);
	}
	// Fork a consumer
	pid_c = fork();
	if (pid_c == 0) {
		// Child Process
		// Run consumer
		consumer(100,"test3_log.txt");
		exit(0);
	}
	else if (pid_c < 0) {
		// ERROR IN FORK
		perror("Error in consumer fork");
		exit(1);
	}
	// Wait for producer and consumer child processes
	w_num = wait(NULL);
	if (w_num < 0) {
		perror("Error in wait");
	}
	else if (w_num == pid_p) {
		PDEBUG("Producer finished.\n");
	}
	else {
		PDEBUG("Consumer finished.\n");
	}
	w_num = wait(NULL);
	if (w_num < 0) {
		perror("Error in wait");
	}
	else if (w_num == pid_c) {
		PDEBUG("Consumer finished.\n");
	}
	else {
		PDEBUG("Producer finished.\n");
	}
}
/*
* Testcase #4: 
* Start two producers which will both produce 50 items each before exiting 
* Start a consumer which will try to consume 200 items before exiting
* All producers and consumers should exit normally after producing and consuming
* all the items
*/
void test4() {
	int w_num;
	printf("Running test 4\n");
	// Fork a producer
	pid_t pid_p1, pid_p2, pid_c;
	pid_p1 = fork();
	if (pid_p1 == 0) {
		// Child Process
		// Run producer
		producer(50,"BLACK");
		exit(0);
	}
	else if (pid_p1 < 0) {
		// ERROR IN FORK
		perror("Error in first producer fork");
		exit(1);
	}
	// Fork a second producer
	pid_p2 = fork();
	if (pid_p2 == 0) {
		// Child Process
		// Run producer
		producer(50,"WHITE");
		exit(0);
	}
	else if (pid_p2 < 0) {
		// ERROR IN FORK
		perror("Error in second producer fork");
		exit(1);
	}
	// Fork a consumer
	pid_c = fork();
	if (pid_c == 0) {
		// Child Process
		// Run consumer
		consumer(200,"test4_log.txt");
		exit(0);
	}
	else if (pid_c < 0) {
		// ERROR IN FORK
		perror("Error in consumer fork");
		exit(1);
	}
	// Wait for producer and consumer child processes
	w_num = wait(NULL);
	if (w_num < 0) {
		perror("Error in wait");
	}
	else if (w_num == pid_p1) {
		PDEBUG("Producer 1 finished.\n");
	}
	else if (w_num == pid_p2) {
		PDEBUG("Producer 2 finished.\n");
	}
	else {
		PDEBUG("Consumer finished.\n");
	}
	w_num = wait(NULL);
	if (w_num < 0) {
		perror("Error in wait");
	}
	else if (w_num == pid_p1) {
		PDEBUG("Producer 1 finished.\n");
	}
	else if (w_num == pid_p2) {
		PDEBUG("Producer 2 finished.\n");
	}
	else {
		PDEBUG("Consumer finished.\n");
	}
	w_num = wait(NULL);
	if (w_num < 0) {
		perror("Error in wait");
	}
	else if (w_num == pid_c) {
		PDEBUG("Consumer finished.\n");
	}
	else {
		PDEBUG("Producer finished.\n");
	}
}

/*
* Testcase #5: 
* Start a producer which will produce 50 items before exiting
* Start two consumers which will consume the 50 items
* The producer and consumers should exit normally after all 50 items
* are produced and consumed
*/
void test5() {
	printf("Running test 5\n");
	// Fork a producer
	pid_t pid = fork();
	if (pid == 0) {
		// Child Process
		// Run producer
		producer(50,"BLACK");
		exit(0);
	}
	else if (pid < 0) {
		// ERROR IN FORK
		perror("Error in producer fork");
		exit(1);
	}
	// Fork a consumer
	pid = fork();
	if (pid == 0) {
		// Child Process
		// Run consumer
		consumer(25,"test5a_log.txt");
		exit(0);
	}
	else if (pid < 0) {
		// ERROR IN FORK
		perror("Error in first consumer fork");
		exit(1);
	}
	// Fork a second consumer
	pid = fork();
	if (pid == 0) {
		// Child Process
		// Run consumer
		consumer(25,"test5b_log.txt");
		exit(0);
	}
	else if (pid < 0) {
		// ERROR IN FORK
		perror("Error in second consumer fork");
		exit(1);
	}
	// Wait for producer and consumer child processes
	if (wait(NULL) < 0) {
		perror("Error in wait");
	}
	if (wait(NULL) < 0) {
		perror("Error in wait");
	}
	if (wait(NULL) < 0) {
		perror("Error in wait");
	}
}

/*
 * The main program dispatches the appropriate 
 * test function, depending on the argument to
 * the program.
 */
int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("Usage: %s <test number>\n",argv[0]);
		return 0;
	}
	int testNum = atoi(argv[1]);
	switch (testNum) {
	case 1:
		test1();
		break;
	case 2:
		test2();
		break;
	case 3:
		test3();
		break;
	case 4:
		test4();
		break;
	case 5:
		test5();
		break;
	default:
		printf("Invalid test number\n");
		break;
	}
	return 0;
}
