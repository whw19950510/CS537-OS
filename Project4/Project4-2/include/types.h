#ifndef _TYPES_H_
#define _TYPES_H_

// Type definitions
//define condition variables
typedef struct __cond_t{
	struct proc *thqueue[8];
    uint head;
    uint tail;
    uint flag;
}cond_t;

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;
#ifndef NULL
#define NULL (0)
#endif

#endif //_TYPES_H_
