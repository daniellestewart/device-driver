/* CSCI 5103 Fall 2016
 * Assignment # 7
 * name: Alexander Cina, Danielle Stewart
 * student id: 4515522, 4339650
 * x500 id: cinax020, dkstewar
 * CSELABS machine: csel-x07-umh
 */

// This header file defines the item struct
#ifndef _ITEM_H_
#define _ITEM_H_

typedef struct item_s {
	char data[32];
} item_t;

#define ITEM_SZ sizeof(struct item_s)

#ifdef USER_LEVEL

#include <stdio.h>

// Place the string "str" into the data slot of the item.
void fillItem(struct item_s *item, char *str, size_t len) {
	// Check for null pointers first.
	if (item == NULL) {
		fprintf(stderr,"Error in function fillItem, item is null\n");
		return;
	}
	if (str == NULL) {
		fprintf(stderr,"Error in function fillItem, str is null\n");
		return;
	}
	unsigned int i;
	// Continue while i is less than the len argument, less than
	// the strlen of str, and less than 31 (leave space for a null
	// byte!).
	for (i=0; i<len && str[i] != '\0' && i < 31; i++) {
		item -> data[i] = str[i];
	}
	// Fill the rest of data with '\0's
	for (/* leave i unchanged */; i<32; i++) {
		item -> data[i] = '\0';
	}
}

// print the data of item into a file
void printItem(struct item_s *item, FILE *f) {
	if (item == NULL) {
		fprintf(stderr,"Error in function printItem, item is null\n");
	}
	fprintf(f,"%s\n",item->data);
}

#endif//USER_LEVEL

#endif//_ITEM_H_
