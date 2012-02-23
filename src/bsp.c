/*
    BSPonMPI. This is an implementation of the BSPlib standard on top of MPI
    Copyright (C) 2006  Wijnand J. Suijlen
                                                                                
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
                                                                                
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
                                                                                
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
                                                                                
    See the AUTHORS file distributed with this library for author contact
    information.
*/

/** @mainpage BSPonMPI
 * BSPonMPI is an implementation of the BSPlib standard on top of MPI 1.1
 * 
 * \section Introduction
 * You should read this document if you want to makes changes to BSPonMPI or
 * if you want to write your own BSPlib implementation and you need some
 * inspiration. If you just want to use BSPonMPI, you should read the README
 * file (included with the BSPonMPI package) first. In all cases I assume that
 * the reader is familiar with the  
 * <A HREF="http://www.bsp-worldwide.org/standard/bsplib_C_examples.ps.Z">BSPlib</A>
 * standard 
 * 
 *  This document contains
 *  - A brief overview of the architecture of BSPonMPI
 *  - Short descriptions of every function and data structure
 *  - Detailed explanation of some non-standard (weird) solutions
 *  - A lot of pictures and cross references
 *
 *  If you want to change something in BSPonMPI, you should read the
 *  \ref Architecture description and the detailed descriptions
 *  of the code which will be affected by your changes. If you just need
 *  inspiration, you only need to read the \ref Architecture  description
 * 
 * \section Architecture
 
 * \subsection Requirements
 * The mission is to make a BSPlib implementation which is fast and platform
 * independent. It will be written in C and uses MPI as communication
 * library which are both available for almost any platform.
 *
 * \subsection genidea General Idea
 * The BSPlib standard is designed to make programming according to the Bulk
 * Synchronous Parallel (BSP) model easier. The BSP model prescribes that you
 * cut up your program in \e computation and \e communication supersteps.
 * BSPlib helps you by providing a function bsp_sync() to execute an entire
 * communication superstep. Communication is gathered during a computation
 * superstep using three functions: bsp_put(), bsp_get() and bsp_send().
 * To put it very simple: A BSPlib implementation is a 
 * - buffer   
 * - having three functions to fill it: bsp_put(), bsp_get() and bsp_send(), and
 * - one function to empty it: bsp_sync()
 *
 * Actually it is not that simple: A bsp_get() needs some action to be taken
 * on the remote processor and expects some data to be returned. This very
 * simple model can be repaired by seeing that there are not only data
 * deliveries (bsp_put() and bsp_send()) but also data requests (bsp_get()).
 * Therefore we may conclude that any BSPlib implementation consists of
 * two buffers: one request buffer and one delivery buffer. Note that I ignore
 * the two unbuffered communication routines bsp_hpput() and bsp_hpget().
 * These are currently implemented as their buffered counterparts.
 *
 * Let me now restate what a BSPlib implementation is:
 * - Two communication buffers: One data request and one data delivery buffer.
 * - Three functions to fill them: bsp_get() which adds a request to the data
 * request buffer, and bsp_send() and bsp_put() which add a delivery to the data
 * delivery buffer.
 * - One function to empty them: bsp_sync()
 *
 * \subsection designchoices Design Choices 
 * The idea is to use the bulk communication procedures of MPI. The two most
 * BSP like MPI functions are: 
 * <A HREF="http://www-unix.mcs.anl.gov/mpi/mpi-standard/mpi-report-1.1/node75.htm">
 * MPI_Alltoall()</A> and 
 * <A HREF="http://www-unix.mcs.anl.gov/mpi/mpi-standard/mpi-report-1.1/node75.htm">
 * MPI_Alltoallv()</A>. Nowadays computer manufacturers implement efficient
 * MPI 1.1 libraries. Any MPI enabled machine has these functions and it is
 * very probable that they are optimal. You may wonder why no use is
 * made of the DRMA procedures of MPI-2  . They may be very fast on DRMA enabled machines. 
 * However MPI-2 is not yet widely available and will therefore limit the use
 * of this library.
 * 
 * In order to effectively use these communication procedures we need that the
 * surrounding code is very fast and portable. The choices made are
 * -# Use ANSI C 99 as programming language in combination with the GNU
 * autotools. 
 * -# Use an object oriented programming style. 
 * -# Collect all communication and transmit them using the least possible
 * calls to MPI_Alltoallv. 
 * -# Use arrays to implement communication buffers. They can be used in a
 * number of contexts, e.g. as a queue and as parameter of the MPI_Alltoallv()
 * function.  
 * -# Allocate memory for these buffers only once (at the start of the
 * program) and double the allocated memory if the buffer proves to be too
 * small. Using this strategy the number of calls to malloc() remains very
 * small. 
 * -# Minimise the use of branch statements (if, switch). 
 * -# Try to make optimisation as easy as possible for a C compiler, but
 * only use ANSI C constructions. Examples: RESTRICT, inline,
 * static, memory alignment, rather dereference a pointer than use memcpy(),
 * etc...
 *
 * \subsection Implementation
 * \subsubsection obj Object Oriented Programming in ANSI C
 * Though ANSI C is used as programming language, the program is written in an object
 * oriented style. To translate classes and inheritance into C constructs, 
 * \c struct's, \c union's and \c enum's are used, e.g.: the C++ code
 * \code
  class A { int x, y;};
  class B: A { int a, b;};
  class C: A { int c, d;};

  void foo()
  {
    A a; a.x = 0; 
    B b; b.a = 1; b.x = 0;
    C c; c.d = 2; c.y = 1;
  }  
  \endcode 
 * translates into something like
  \code 
  typedef enum { B_type, C_type } class_type;
  typedef struct { int a, b;} B_info;
  typedef struct { int c, d;} C_info;
  typedef union { B_info b; C_info c;} class_info;
  typedef struct { int x, y; class_info info; class_type type;} A;

  void foo()
  {
    A a; a.x = 0;
    A b; b.type = B_type; b.b.a=1; b.x = 0;
    A c; c.type = C_type; c.c.d=2; c.y = 1;
  }  
  \endcode
 Member functions are translated into <tt>className_functionName( classref *,
 ...)</tt>. For example
 \code
   class A 
   { 
     int x;
     int foo() const
     {
       return x;
     };
   };  
 \endcode
 translates into
 \code
   typedef struct {int x;} A;
   int a_foo( const A * RESTRICT a)
   {
     return a->x;
   }
 \endcode
 *
 * \subsection The Communication Buffer
 * Because BSPlib is essentially a communication buffer, the performance of
 * the library heavily depends on its data structure.
 * For this purpose \ref ExpandableTable is designed. This is
 * essentially a one-dimensional array which is subdivided in equally sized
 * blocks. Blocks and processors are assigned one-to-one to each other.
 * When you look at these blocks as \e columns, you will get a
 * \e table where each column corresponds to a processor. Putting data in a
 * column, marks that data as 'received from' or 'to be sent to' the
 * corresponding processor. This way, the array can be passed as a parameter of
 * the \c MPI_Alltoallv() function. 
 * \image html exptable.png "Data layout of the ExpandableTable data structure"
 * \image latex exptable.eps "Data layout of the ExpandableTable data structure" width=10cm
 * Each column is again subdivided in \e slots. Accessing arbitrary bytes may
 * be very expensive on some architectures or impossible. Therefore aligned
 * addresses are used by subdividing each column in slots which are typically
 * of the same size as a struct, double or something similar. The
 * size of the entire array is equal to \c nprocs \c x \c rows \c x \c
 * slot_size, where \c rows is the number of slots in a column. When data is
 * added to a column the variable \c used_slot_count is incremented with the
 * number of slots occupied by the new data.  An example is shown below
 * \image html exptable-ex.png "Example data layout of an ExpandableTable data structure"
 * \image latex exptable-ex.eps "Example data layout of an ExpandableTable data structure" width=10cm
 * When one tries to add data to a column but there is not enough space
 * available, new space is allocated. The new space is two times the size of
 * the already allocated space. Again this array is subdivided in equally
 * sized blocks. The old data is copied to the new locations and the new data
 * is added. Note that the size of an \ref ExpandableTable can only increase. This
 * way only a very limited number of calls to malloc() are necessary. \ref
 * ExpandableTable and its member functions are declared in \ref
 * bsp_exptable.c and \ref bsp_exptable.h
 *
 * This data structure serves as a building block for the two communication
 * buffers: RequestTable and DeliveryTable.
 * They both have slightly different needs. RequestTable only has to handle
 * data requests from other processors. Each data request is of a fixed size.
 * Therefore implementing RequestTable will be rather straightforward. For
 * details see \ref bsp_reqtable.h and \ref bsp_reqtable.c.  On the
 * other hand DeliveryTable has to handle data deliveries which may differ in
 * size.  The implementation of DeliveryTable is a bit different; This
 * buffer is not only used for communication of bsp_put(), bsp_send() and the data
 * delivery part of a bsp_get(), but also for actions which have to be carried
 * out during a bsp_sync(), e.g.: bsp_set_tagsize(), etc... for details
 * see \ref bsp_delivtable.h and \ref bsp_delivtable.c
 *
 * BSPlib also provides a way to address remote memory locations and a queue
 * of received messages. These two are represented by the classes
 * MemoryRegister and MessageQueue, respectively. MemoryRegister can also be modelled as a fixed size
 * element array. Therefore the abstract class FixedElSizeTable is introduced.
 * MessageQueue holds information where messages can be found
 * in a received DeliveryTable. 
 * We get the following UML class diagram
   \dot
   
digraph G {
  // define standard classes 
  node [ shape = box ];
  ExpandableTable [URL = "\ref bsp_exptable.h"] ;
  FixedElSizeTable [URL= "\ref bsp_exptable.h"];
  MessageQueue    [URL = "\ref bsp_mesgqueue.h"];
  DeliveryTable   [URL = "\ref bsp_delivtable.h"];
  MemoryRegister  [URL = "\ref bsp_memreg.h"];
  RequestTable    [URL = "\ref bsp_reqtable.h"];
  BSP             [URL = "\ref bsp.c"];
 
  // defining inheritance relations 
  {
    edge [ arrowhead = onormal ];
    FixedElSizeTable -> ExpandableTable ;
    
    MemoryRegister -> FixedElSizeTable;
    RequestTable -> FixedElSizeTable;

    MessageQueue;
    DeliveryTable -> ExpandableTable;
  }
  // define composition relation 
  {
    edge [ arrowtail = diamond, arrowhead = none ];
    BSP -> { MemoryRegister; MessageQueue } [ headlabel = 1, taillabel = 1 ];
    BSP -> { DeliveryTable ; RequestTable } [ headlabel = 2, taillabel = 1 ] ;
  }
  // define aggregrate relation
  {
    edge [ arrowtail = odiamond, arrowhead = none ];
    MessageQueue -> DeliveryTable [ headlabel = 1, taillabel = 1] ;
  }  
}  
\enddot
 * A small UML legenda is shown below. 
 * \image html legenda.png "Legenda of UML class diagram and UML Sequence Diagram"
 * \image latex legenda.eps "Legenda of UML class diagram and UML Sequence Diagram" height=10cm,angle=90
 *
 * \subsection sequence A Sequence Diagram
 * Until now the explanation may seem to be still a bit abstract. Below the source
 * code and its sequence diagram of a simple BSP program are shown. It
 * shows how processors and objects collaborate over time. You may find that in this
 * picture the information density is very high. Do not examine the picture in
 * whole, but look at individual calls to bsplib functions.
 *
 * \code
 #include <bsp.h>
 #include <stdio.h>

 void spmd_part()
 {
   int a=1, b=2, c=3;
   bsp_begin(2);
   bsp_push_reg(&a, sizeof(int));
   bsp_sync();

   if (bsp_pid() == 0)
     {
       bsp_put(1, &b, &a, 0, sizeof(int));
       bsp_get(1, &a, 0, &c, sizeof(int));
       bsp_send(1, NULL, "some text", 10);
     }
   bsp_sync();

   bsp_pop_reg(&a);
   bsp_sync();

   bsp_end();
 }
 
 int main(int argc, char *argv)
 {
   bsp_init(&spmd_part, argc, argv);

   printf("First perform some sequential code\n");

   spmd_part();

   printf("Finally perform some sequential code\n");
   return 0;
 }
 \endcode
 * \image html sequence.png "Sequence diagram of the example above"
 * \image latex sequence.eps "Sequence diagram of the example above" width=14cm
 *
 * \subsection improv Room For Improvement
 * There are still things which are not being taken care of in an optimal way.
 * Ideas to improve performance are:
 * - Implement the memoryRegister_find such that it is O(1) in stead of O(n)
 * (e.g. hash on least significant bits)
 * - Every column in DeliveryTable has an index of 18 integers. Currently
 * the full index is communicated. Reducing the communication size of the
 * index may reduce the latency.  
 * - Reduce the overhead when sending individual bsp_put() or bsp_send().
 * 
 *
 * \section License
    BSPonMPI. This is an implementation of the BSPlib standard on top of MPI.
    Copyright &copy; 2006 
    <A HREF="mailto:wjsuijle@users.sourceforge.net">Wijnand J. Suijlen</A>
                                                                                
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
                                                                                
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
                                                                                
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "bsp.h"
#include "bsp_memreg.h"
#include "bsp_mesgqueue.h"
#include "bsp_delivtable.h"
#include "bsp_reqtable.h"
#include "bsp_private.h"
#include "bsp_alloc.h"
#include "bsp_abort.h"

#define DELIVTAB_SIZE 1
#define REQTAB_SIZE   1
#define MEMREG_SIZE   1	

/** @file bsp.c 
    Implements the BSPlib primitives.
    @author Wijnand Suijlen
*/    

/** @name Initialisation */
/*@{*/

/** Initializes BSPonMPI. 
 * @bug Because several MPI implementations require that command line
 * arguments are supplied, a call to this function at the start of the program
 * is mandatory
 *
 * Example 
 * @code

    void do_something()
    {
      bsp_begin(bsp_nprocs())
      ... a parallel program ...
      bsp_end()
    }  
      
    int main(int argc, char *argv[])
    {
      bsp_init( &do_something, argc, argv);
      ... optional sequential code ...
      do_something();
      ... optional sequential code ...
    }  
   @endcode  
   @param spmd_part reference to the SPMD function
   @param argc obtained from function main() 
   @param argv obtained from function main()

   @see bsp_begin()
   @see bsp_end()
   @see bsp_nprocs()
*/
void BSP_CALLING
bsp_init (void (*spmd_part) (void), int argc, char *argv[])
{
#ifndef _SEQUENTIAL
  /* initialize MPI */
  MPI_Init(&argc, &argv);
  MPI_Comm_size( MPI_COMM_WORLD, &bsp.nprocs);
  MPI_Comm_rank( MPI_COMM_WORLD, &bsp.rank);
  
  if (bsp.rank == 0) 
    {
     /* do nothing */
    }
  else
    {
     /* else just run the spmd part */
      spmd_part();
      exit(0);
    }  
#else
	bsp.rank = 0;
#endif // BSP_SEQUENTIAL
}

/** Marks the start of the SPMD code. The code following the call to this
    function will be executed in parallel with at most maxprocs processors.
    The SPMD code must end with a call to bsp_end()
    @param maxprocs Denotes the requested number of processors. The actual
                    allocated number of processors may be less.  Calling 
		    @code bsp_begin(bsp_nprocs()) @endcode allocates the maximum number of
		    processors.
    @see bsp_nprocs()		    
    @see bsp_end()
*/
void BSP_CALLING
bsp_begin (int maxprocs)
{
#ifndef _SEQUENTIAL
  int flag, i, *ranks;
  MPI_Group group, newgroup;
  /* initialize if necessary */
  if (MPI_Initialized(&flag), !flag)
    {
      int argc = 0;
      char **argv = NULL;
      fprintf(stderr, "Warning! bsp_init() is not called. Initialization of MPI may fail\n");
      MPI_Init (&argc, &argv);
      MPI_Comm_size (MPI_COMM_WORLD, &bsp.nprocs);
      MPI_Comm_rank (MPI_COMM_WORLD, &bsp.rank);
    }
  /* broadcast maxprocs to all other processors */  
  MPI_Bcast(&maxprocs, 1, MPI_INT, 0, MPI_COMM_WORLD);
  
  /* allocate at most maxproc processors:
     Form a new group of processors. Some processors will not be member of
     this group and will therefore be deallocated */
  if (maxprocs > 0) 
    {
      bsp.nprocs = MIN(maxprocs, bsp.nprocs);
    } /* else request maximum number of processors*/
  
  MPI_Comm_group( MPI_COMM_WORLD, &group);
  ranks = bsp_malloc( bsp.nprocs, sizeof(int));
  for (i = 0; i < bsp.nprocs; i++)
    ranks[i] = i;

  MPI_Group_incl(group, bsp.nprocs, ranks, &newgroup);
  MPI_Comm_create(MPI_COMM_WORLD, newgroup, &bsp.communicator);

  bsp_free(ranks);

  if (bsp.rank >= bsp.nprocs)
    { /* terminate all unnecessary processes */
      MPI_Finalize();
      exit(0);
    }  
#else
  bsp.nprocs = 1;
  bsp.rank = 0;
#endif // _SEQUENTIAL
  bsp.send_index = bsp_malloc(3 * bsp.nprocs, sizeof(unsigned int));
  bsp.recv_index = bsp_malloc(3 * bsp.nprocs, sizeof(unsigned int));

  /* initialize data structures */
  memoryRegister_initialize(&bsp.memory_register, bsp.nprocs, MEMREG_SIZE,
                            bsp.rank);
  messageQueue_initialize (&bsp.message_queue);
  deliveryTable_initialize(&bsp.delivery_table, bsp.nprocs, DELIVTAB_SIZE);
  requestTable_initialize(&bsp.request_table, bsp.nprocs, REQTAB_SIZE);
  deliveryTable_initialize(&bsp.delivery_received_table, bsp.nprocs, 
                           DELIVTAB_SIZE);
  requestTable_initialize(&bsp.request_received_table, bsp.nprocs,
                           REQTAB_SIZE);

  bsp.global_array_last = 0;
  bsp.global_overflow = 0;
  
  /* save starting time */
  bsp.begintime = 0; // bsp.begintime is used in bsp_time(), so must be initialized
  bsp.begintime = bsp_time();
}


/** Ends the SPMD code. This function must be called after all SPMD code
    has been executed. The code after this call is executed by processor 0
    only. 
*/    
void BSP_CALLING
bsp_end ()
{
  /* clean up datastructures */
  memoryRegister_destruct (&bsp.memory_register);
  deliveryTable_destruct(&bsp.delivery_table);
  requestTable_destruct(&bsp.request_table);
  deliveryTable_destruct(&bsp.delivery_received_table);
  requestTable_destruct(&bsp.request_received_table);

#ifndef _SEQUENTIAL
  bsp_free(bsp.recv_index);
  bsp_free(bsp.send_index);

  /* and finalize */
  MPI_Finalize ();
#endif
}
/*@}*/


/** @name Halt */
/*@{*/

/** Aborts the program  printing a message to the standard error output.
 @param format uses the same format as printf()
 */
void BSP_CALLING
bsp_abort (const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  vfprintf (stderr, format, ap);
  va_end (ap);

  bsp_intern_abort (ERR_BSP_ABORT, "bsp_abort()", __FILE__, __LINE__);
}
/*@}*/

/** @name Enquiry */
/*@{*/

/** Returns the number of processors available / allocated. 
   @return 
           -  if bsp_init() is not yet called: -1 .
	        Note that it is allowed to call @code bsp_begin(bsp_nprocs()) @endcode
           -  if bsp_init() is called, but not bsp_begin(): number of processors available 
	   -  if bsp_init() and bsp_begin() are called: number of processors allocated
*/
int BSP_CALLING
bsp_nprocs ()
{
#ifndef _SEQUENTIAL
  int flag;
  MPI_Initialized(&flag);
  return flag?bsp.nprocs:-1;
#else
	return 1;
#endif
}

/** Returns the rank of the processor 
  @return The rank of the processor
  */
int BSP_CALLING
bsp_pid ()
{
  return bsp.rank;
}

/*@}*/

/** @name Superstep */
/*@{*/

/** Seperates two supersteps. */ 
void BSP_CALLING
bsp_sync ()
{
  unsigned int maxreqrows = 0, maxdelrows = 0, p;
  unsigned int any_gets = 0; /* any_gets is a boolean value, whether there are
                               any gets to performed. If there are no gets,
			       then one MPI_Alltoall doesn't have to be
			       executed */
  /* reset message buffer */
  messageQueue_sync(&bsp.message_queue);
  requestTable_reset(&bsp.request_received_table);
  deliveryTable_reset(&bsp.delivery_received_table);
 
  /* communicate information */
  for (p = 0; p < (unsigned)bsp.nprocs; p++)
    any_gets |= bsp.request_table.used_slot_count[p];

  for (p = 0; p < (unsigned)bsp.nprocs; p++)
    {
      bsp.send_index[3*p    ] = bsp.request_table.used_slot_count[p];
      bsp.send_index[3*p + 1] = bsp.delivery_table.used_slot_count[p];
      bsp.send_index[3*p + 2] = any_gets;
    }  

  MPI_Alltoall( bsp.send_index, 3 , MPI_UNSIGNED, 
                bsp.recv_index, 3 , MPI_UNSIGNED, bsp.communicator);

  /* expand buffers if necessary */
  maxreqrows = array_max(bsp.recv_index, 3*bsp.nprocs, 3);
  for (p = 0; p < (unsigned)bsp.nprocs; p++)
    maxdelrows = MAX( bsp.recv_index[1 + 3*p] + 
                     bsp.request_table.info.req.data_sizes[p], maxdelrows);

  if ( bsp.request_received_table.rows < maxreqrows )
    {
      maxreqrows = MAX(bsp.request_received_table.rows, maxreqrows);
      requestTable_expand(&bsp.request_received_table, maxreqrows);
    }  
  
  if (bsp.delivery_received_table.rows < maxdelrows )
    {
      maxdelrows = MAX(bsp.delivery_received_table.rows, maxdelrows);
      deliveryTable_expand(&bsp.delivery_received_table, maxdelrows );
    }  

  /* copy necessary indices to received_tables */
  for (p = 0; p < (unsigned)bsp.nprocs; p++) 
    {
      bsp.request_received_table.used_slot_count[p] = bsp.recv_index[3*p];
      bsp.delivery_received_table.used_slot_count[p] =
        bsp.recv_index[1 + 3*p] + bsp.request_table.info.req.data_sizes[p] ;
    }	
  
  /* Now we may conclude something about the communcation pattern */
  any_gets = 0;
  for (p = 0; p < (unsigned)bsp.nprocs; p++)   
    any_gets |= bsp.recv_index[3*p + 2];

  /* communicate & execute */
  if (any_gets) 
    {
      expandableTable_comm(&bsp.request_table, &bsp.request_received_table,
                    bsp.communicator);
      requestTable_execute(&bsp.request_received_table, &bsp.delivery_table);
    }

  expandableTable_comm(&bsp.delivery_table, &bsp.delivery_received_table,
                     bsp.communicator);
  deliveryTable_execute(&bsp.delivery_received_table, 
		        &bsp.memory_register, &bsp.message_queue, bsp.rank);
  /* clear the buffers */			
  requestTable_reset(&bsp.request_table);
  deliveryTable_reset(&bsp.delivery_table);
 
  /* pack the memoryRegister */
  memoryRegister_pack(&bsp.memory_register);
}
/*@}*/

/** @name DRMA */
/*@{*/
/** Makes the memory location with specified size available for DRMA
 * operations at the next and additional supersteps. 
 * @param ident pointer to memory location
 * @param size of memory block
 * @note In this version of BSPonMPI the parameter \a size is ignored
 * @see bsp_pop_reg()
 */
void BSP_CALLING
bsp_push_reg (const void *ident, size_t size)
{
  int i;
  DelivElement element;
  element.size = 0;
  element.info.push.address = ident;
  for (i=0 ; i < bsp.nprocs; i++)
    deliveryTable_push(&bsp.delivery_table, i, &element, it_pushreg);
}

/** Deregisters the memory location 
  @param ident pointer to memory location
  @see bsp_push_reg()
  */
void BSP_CALLING
bsp_pop_reg (const void *ident)
{
  DelivElement element;
  element.size = 0;
  element.info.pop.address = ident;
  deliveryTable_push(&bsp.delivery_table, bsp.rank, &element, it_popreg);
}  

/** Puts a block of data in the memory of some other processor at the next
 * superstep. This function is buffered, i.e.: the contents of \a src
 * is copied to a buffer and transmitted at the next bsp_sync() 
 * @param pid rank of destination (remote) processor
 * @param src pointer to source location on source (local) processor
 * @param dst pointer to destination location on source processor. Translation
              of addresses is performed with help of earlier calls to bsp_push_reg()
   @param offset offset from \a dst in bytes (comes in handy when working with arrays)
   @param nbytes number of bytes to be copied
   @see bsp_push_reg()
*/
void BSP_CALLING
bsp_put (int pid, const void *src, void *dst, long int offset, size_t nbytes)
{
  /* place put command in buffer */
  char * RESTRICT pointer;
  DelivElement element;
  element.size = (unsigned int) nbytes;
  element.info.put.dst = 
    memoryRegister_memoized_find(&bsp.memory_register, pid, dst) + offset;
  pointer = deliveryTable_push(&bsp.delivery_table, pid, &element, it_put);
  memcpy(pointer, src, nbytes);
}


/** Gets a block of data from the memory of some other processor at the next
 * superstep. This function is buffered, i.e.: The data is retrieved from the
 * destionation processor at the start of the next bsp_sync(). Translation of
 * the \a src pointer is performed with help of earlier calls to
 * bsp_push_reg()
 * @param pid Ranks of the source (remote) processor 
 * @param src Pointer to source location using a pointer to a local memory
 *            region 
 * @param offset offset from \a src in bytes
 * @param dst Pointer to destination location 
 * @param nbytes Number of bytes to be received
 * @see bsp_push_reg()
*/
void BSP_CALLING
bsp_get (int pid, const void *src, long int offset, void *dst, size_t nbytes)
{
  ReqElement elem;
  elem.size = (unsigned int )nbytes;
  elem.src = 
     memoryRegister_memoized_find(&bsp.memory_register, pid, src);
  elem.dst = dst;
  elem.offset = offset;
  
  /* place get command in buffer */
  requestTable_push(&bsp.request_table, pid, &elem);
}
/*@}*/

/** @name BSMP */
/*@{*/

/** Sends message to a processor. You may supply a tag and and a payload. The
 * default size of the tag is 0. To change the tag size you can use
 * bsp_set_tagsize().
 @param pid Rank of the destination processor
 @param tag pointer to the tag
 @param payload pointer to the payload
 @param payload_nbytes size of the payload
 */
void BSP_CALLING
bsp_send (int pid, const void *tag, const void *payload, size_t payload_nbytes)
{
  DelivElement element;
  char * RESTRICT pointer;
  element.size = (unsigned int )payload_nbytes + bsp.message_queue.send_tag_size;
  element.info.send.payload_size = (unsigned int )payload_nbytes;
  pointer = deliveryTable_push(&bsp.delivery_table, pid, &element, it_send);
  memcpy( pointer, tag, bsp.message_queue.send_tag_size);
  memcpy( pointer + bsp.message_queue.send_tag_size, payload, payload_nbytes);
}

/** Gives the number of messages and the sum of the payload sizes in queue.
  @param nmessages pointer to an int. The value of this integer will be set to
  the number of messages in queue
  @param accum_nbytes pointer to an int. The value of this integer will be set
  to the sum of payload sizes in all messages. 
*/
void BSP_CALLING
bsp_qsize (int * RESTRICT nmessages, size_t * RESTRICT accum_nbytes)
{
  *nmessages = bsp.message_queue.n_mesg;
  *accum_nbytes = bsp.message_queue.accum_size;
}

/** Retrieves the tag and payload size of the current message in
 * queue.
   @param status the size of the payload of the current message, or when the
   queue is empty -1
   @param tag a pointer to a memory location big enough to contain a tag.
 */  
           
void BSP_CALLING
bsp_get_tag (int * RESTRICT status , void * RESTRICT tag)
{
  if (bsp.message_queue.n_mesg == 0)
    *status = -1;
  else
    {
      ALIGNED_TYPE * RESTRICT current_tag = 
        bsp.message_queue.head + 
	 no_slots( sizeof(DelivElement), sizeof(ALIGNED_TYPE));
      DelivElement * RESTRICT message = (DelivElement *) bsp.message_queue.head;	
      *status = message->size;
      memcpy(tag, current_tag, bsp.message_queue.recv_tag_size ); 
    }
}

/** Dequeue the current message.
  @param payload A pointer to a memory location big enough to contain the
  payload or \a reception_nbytes
  @param reception_nbytes The maximum number of bytes to copy
*/  
void BSP_CALLING
bsp_move (void *payload, size_t reception_nbytes)
{
  DelivElement * RESTRICT message = (DelivElement *) bsp.message_queue.head;	
  int copy_bytes = MIN((unsigned)reception_nbytes, message->size);
  char * RESTRICT current_payload =
    (char *) bsp.message_queue.head + 
      sizeof(ALIGNED_TYPE) * 
      no_slots( sizeof(DelivElement), sizeof(ALIGNED_TYPE)) +
    bsp.message_queue.recv_tag_size;
  memcpy(payload, current_payload, copy_bytes);
  
  bsp.message_queue.head += message->next;
  bsp.message_queue.n_mesg --;
  bsp.message_queue.accum_size -= message->size;
}

/** Sets the tag size at the next superstep.
  @param tag_nbytes pointer to an int which should contain the size of the tag
  in bytes. It becomes current tag size.
 */ 
void BSP_CALLING
bsp_set_tagsize (size_t *tag_nbytes)
{
  DelivElement element;
  element.info.settag.tag_size = (unsigned int )*tag_nbytes;
  element.size = 0;
 
  deliveryTable_push(&bsp.delivery_table, bsp.rank, &element, it_settag);
  *tag_nbytes = bsp.message_queue.send_tag_size;
}

/*@}*/


/** @name High Performace */
/*@{*/

/** Dequeue the current message in an unbuffered way. 
  @param tag_ptr a pointer to reference of memory location which will contain
                the tag 
  @param payload_ptr a pointer to a reference of a memory location which will
                contain the pyload
  @return the payload size of the dequeued message
*/  
int BSP_CALLING
bsp_hpmove (void **tag_ptr, void **payload_ptr)
{
  if (bsp.message_queue.n_mesg == 0)
    return -1;
  else
    {
       ALIGNED_TYPE * RESTRICT current_tag =
         (ALIGNED_TYPE *) bsp.message_queue.head + 
	   no_slots(sizeof(DelivElement), sizeof(ALIGNED_TYPE));
       char * RESTRICT current_payload =
         (char *) current_tag + bsp.message_queue.recv_tag_size;
       DelivElement * RESTRICT message =
         (DelivElement *) bsp.message_queue.head;
       int size = message->info.send.payload_size;	 
       *tag_ptr     = current_tag;
       *payload_ptr = current_payload;
       bsp.message_queue.head += message->next;
       bsp.message_queue.n_mesg--;
       bsp.message_queue.accum_size -= size;
       return size;
    }
}

/*@}*/
