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

/** @file bsp_threadsafe.h
 
	Thread safety macros

	@author Peter Krusche
  */  

#ifndef __bsp_threadsafe_H__
#define __bsp_threadsafe_H__

#ifdef BSP_THREADSAFE

extern void BSP_CALLING bsp_thread_locking_init ();
extern void BSP_CALLING bsp_thread_locking_exit ();
extern void BSP_CALLING bsp_thread_lock();
extern void BSP_CALLING bsp_thread_unlock();

#define BSP_TS_INIT() do { bsp_thread_locking_init(); } while (0)
#define BSP_TS_EXIT() do { bsp_thread_locking_exit(); } while (0)
#define BSP_TS_LOCK() do { bsp_thread_lock(); } while (0)
#define BSP_TS_UNLOCK() do { bsp_thread_unlock(); } while (0)

#else

#define BSP_TS_INIT() 
#define BSP_TS_EXIT() 
#define BSP_TS_LOCK() 
#define BSP_TS_UNLOCK() 

#endif

#endif // __bsp_threadsafe_H__
