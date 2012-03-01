/***************************************************************************
 *   Copyright (C) 2007   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#ifndef __MY_SPAWN_H__
#define __MY_SPAWN_H__

#ifdef _WIN32
#include <process.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#else
#include <sys/time.h>
#include <sys/wait.h>

#ifdef _POSIX_SPAWNP
#include <spawn.h>

#define P_WAIT 0
extern char **environ;

static inline void _spawnvp(int, const char * file,  char * const* argv) {
	__pid_t temp;

	if(0 != posix_spawnp(&temp, file, NULL, NULL, argv, environ)) {
		std::cout << "Spawning failed." << std::endl;
	} else {
		waitpid(temp, NULL, 0);
	}
}
#else
#define P_WAIT 0
static inline int _spawnvp(int, const char * file,  char * const* argv) {
	// use system() otherwise
	std::string command;
	while (*argv != NULL) {
		command+= " ";
		command+= *argv;
		++argv;
	}
	return system(command.c_str());
}
#endif
#endif

#endif

