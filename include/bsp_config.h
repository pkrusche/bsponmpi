
/*
 * This file is generated automatically and will be overwritten.
 */ 
#ifndef __BSPCONFIG_H__
#define __BSPCONFIG_H__

#define RESTRICT 
#define BSP_CALLING 
#define STDCALL 

	
#ifdef UNITTESTING
#include "../tests/bsp_test.h"
#else  
#ifndef _SEQUENTIAL
#include <mpi.h>
#else 
#include "bsp_mpistub.h"
#endif // _SEQUENTIAL
#endif  



#endif /* __BSPCONFIG_H__ */

