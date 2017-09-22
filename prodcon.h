/* CSCI 5103 Fall 2016
 * Assignment # 7
 * name: Alexander Cina, Danielle Stewart
 * student id: 4515522, 4339650
 * x500 id: cinax020, dkstewar
 * CSELABS machine: csel-x07-umh
 */

#ifndef _PRODCON_H_
#define _PRODCON_H_

#undef PDEBUG

#ifdef SCULL_DEBUG
#define PDEBUG( ...) fprintf(stderr,__VA_ARGS__)
#else
#define PDEBUG(...)
#endif

#define MAX_WRITE_FAIL 1000
#define MAX_READ_FAIL 1000

#endif//_PRODCON_H_
