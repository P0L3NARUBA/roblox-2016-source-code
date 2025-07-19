/*							       	     HTMemory.c
**	DYNAMIC MEMORY MANAGER
**
**	(c) COPYRIGHT MIT 1995.
**	Please first read the full copyright statement in the file COPYRIGH.
**	@(#) $Id: HTMemory.c,v 2.10 1999/02/22 22:10:11 frystyk Exp $
**
** History:
**	 8 Feb 95	Written by Eric and Henrik
**  Changed by yungDoom
*/

/* Library include files */
#include "wwwsys.h"
#include "HTUtils.h"
#include "HTList.h"
#include "HTMemory.h"					 /* Implemented here */

PRIVATE HTList * HTMemCall = NULL;		    /* List of memory freers */
PRIVATE HTMemory_exitCallback * PExit = NULL;	  /* panic and exit function */
PRIVATE size_t LastAllocSize = 0;		  /* size of last allocation */ 

/* ------------------------------------------------------------------------- */

/*	HTMemoryCall_add
**	----------------
**	Register a call back function that is to be called if we need more
**	memory. Several call back functions can be registered in which case
**	all of them are called in the order of which they were registered.
*/
PUBLIC BOOL HTMemoryCall_add (HTMemoryCallback * cbf)
{
    if (!HTMemCall) HTMemCall = HTList_new();
    return cbf ? HTList_addObject(HTMemCall, (void *) cbf) : NO;
}

/*	HTMemoryCall_delete
**	-------------------
**	Unregister a call back function from a list
*/
PUBLIC BOOL HTMemoryCall_delete (HTMemoryCallback * cbf)
{
    return (HTMemCall && cbf) ? HTList_removeObject(HTMemCall,(void*)cbf) : NO;
}

/*	HTMemoryCall_deleteAll
**	----------------------
**	Unregisters all call back functions
*/
PUBLIC BOOL HTMemoryCall_deleteAll (void)
{
    if (HTMemCall) {
	HTList_delete(HTMemCall);
	HTMemCall = NULL;
	return YES;
    }
    return NO;
}

/*
**	Allocates memory using malloc
*/
PUBLIC void * HTMemory_malloc (size_t size)
{
    void * ptr;
    ptr = malloc(LastAllocSize = size);
    if (ptr != NULL) return ptr;
    if (HTMemCall) {
	HTMemoryCallback * pres;
	while ((pres = (HTMemoryCallback *) HTList_nextObject(HTMemCall))) {
	    (*pres)(size);
	    if ((ptr = malloc(size)) != NULL) return ptr;
	}
    }
    return NULL;
}

/*
**	Allocates memory using calloc
*/
PUBLIC void * HTMemory_calloc (size_t nobj, size_t size)
{
    void * ptr;
    ptr = calloc(nobj, LastAllocSize = size);
    if (ptr != NULL) return ptr;
    if (HTMemCall) {
	HTMemoryCallback * pres;
	size_t total = size * nobj;
	while ((pres = (HTMemoryCallback *) HTList_nextObject(HTMemCall))) {
	    (*pres)(total);
	    if ((ptr = calloc(nobj, size)) != NULL) return ptr;
	}
    }
    return NULL;
}

/*
**	Reallocates memory using realloc
*/
PUBLIC void * HTMemory_realloc (void * p, size_t size)
{
    void * ptr;
    ptr = realloc(p, LastAllocSize = size);
    if (ptr != NULL) return ptr;
    if (HTMemCall) {
	HTMemoryCallback * pres;
	while ((pres = (HTMemoryCallback *) HTList_nextObject(HTMemCall))) {
	    (*pres)(size);
	    if ((ptr = realloc(p, size)) != NULL) return ptr;
	}
    }
    return NULL;
}

/*
**	Frees memory
*/
PUBLIC void HTMemory_free (void * ptr)
{
    if (ptr) {
	free(ptr);
    }
}

/*	HTMemory_setExit
**	----------------
**	Register the memory exit function. This function notifies the user that
**	it is all over. If this function returns or is undefined, 
**	HTMemory_outofmem calls abort.
*/
PUBLIC void HTMemory_setExit (HTMemory_exitCallback * pExit)
{
    PExit = pExit;
}

/*	HTMemory_exit
**	-------------
**	Get the current exit function.
*/
PUBLIC HTMemory_exitCallback * HTMemory_exit (void)
{
    return PExit;
}

/*	HTMemory_outofmem
**	-----------------
**	Call app defined exit function. If that returns, exit anyway.
*/
PUBLIC void HTMemory_outofmem (char * name, char * file, unsigned long line)
{
    if (PExit)
	(*PExit)(name, file, line);
    abort();
}

