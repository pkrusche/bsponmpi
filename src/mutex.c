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

#include <stdlib.h>

#ifdef _WIN32

#include <windows.h>

void * create_mutex() {
    HANDLE * m = malloc(sizeof(HANDLE));
	*m = CreateMutex(NULL, FALSE, NULL);
    return m;
}

void destroy_mutex ( void * m ) {
    HANDLE * _m = ( HANDLE* ) m;
    CloseHandle ( *_m );
	free(_m);
}

void lock_mutex ( void * m ) {
    HANDLE * _m = ( HANDLE* ) m;
    WaitForSingleObject ( *_m, INFINITE );
}

void free_mutex ( void * m) {
    HANDLE * _m = ( HANDLE* ) m;
    ReleaseMutex ( *_m );
}

#else

#include "pthread.h"

void * create_mutex() {
    pthread_mutex_t * m = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init ( m, NULL );
    return m;
}

void destroy_mutex ( void * m ) {
    pthread_mutex_t * _m = ( pthread_mutex_t* ) m;
    pthread_mutex_destroy ( _m );
    free(m);
}

void lock_mutex ( void * m ) {
    pthread_mutex_t * _m = ( pthread_mutex_t* ) m;
    pthread_mutex_lock ( _m );
}

void free_mutex ( void *m ) {
    pthread_mutex_t * _m = ( pthread_mutex_t* ) m;
    pthread_mutex_unlock ( _m );
}

#endif

