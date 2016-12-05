/********************************************************
* DEBUG Head
* Compile:	gcc
* Author:  	grey	
* Last Modify:  		2009.05.19
*********************************************************/

#ifndef __DEBUG_H__
#define	__DEBUG_H__

#include <stdio.h>

/* if you need output debug information, define "_USE_DEBUG_" */
#ifdef _USE_DEBUG_
#define	DBG(X)	printf X
#else
#define	DBG(X)
#endif

#endif/* __DEBUG_H__ */

