
/*
 * This file is generated automatically and will be overwritten.
 */ 
#ifndef __BSPCONFIG_H__
#define __BSPCONFIG_H__


#ifndef __cplusplus
#define inline 
#endif

#ifndef _MSC_VER
#define __cdecl
#endif


#ifndef _WIN32
#include <stdint.h>

typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int64_t INT64;
typedef uint64_t UINT64;

typedef uint8_t BYTE;
typedef uint32_t DWORD;
typedef uint16_t WORD;

#else
#include <Windows.h>

#endif

#ifdef _DEBUG
#include <assert.h>
#define ASSERT assert
#else

#define ASSERT(x)

#endif

#define RESTRICT 
#define __func__ __FUNCTION__ 
#define BSP_CALLING __cdecl 
#define STDCALL __stdcall 

	
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

