
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

#ifdef _HAVE_MPI
#include <mpi.h>

#include "bspx_comm_mpi.h"

#define _BSP_INIT BSP_INIT_MPI
#define _BSP_EXIT BSP_EXIT_MPI
#define _BSP_ABORT BSP_ABORT_MPI
#define _BSP_COMM0 BSP_MPI_ALLTOALL_COMM
#define _BSP_COMM1 BSP_MPI_ALLTOALLV_COMM

#else 

#include "bspx_comm_seq.h"

#define _BSP_INIT BSP_INIT_SEQ
#define _BSP_EXIT BSP_EXIT_SEQ
#define _BSP_ABORT BSP_ABORT_SEQ
#define _BSP_COMM0 BSP_SEQ_ALLTOALL_COMM
#define _BSP_COMM1 BSP_SEQ_ALLTOALLV_COMM

#endif 

#endif  



#endif /* __BSPCONFIG_H__ */

