
/*
 * This file is generated automatically and will be overwritten.
 */ 
#ifndef __BSPCONFIG_H__
#define __BSPCONFIG_H__

#define _X86_64 
#define RESTRICT 
#define BSP_CALLING 
#define STDCALL 

	
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

