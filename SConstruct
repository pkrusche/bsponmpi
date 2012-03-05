###############################################################################
# Copyright (C) 2007   Peter Krusche, The University of Warwick         
# peter@dcs.warwick.ac.uk                                
###############################################################################

import os
import os.path
import glob
import string
import re
import sys
import platform;

from SCons.Defaults import *

###############################################################################
# read options and configure directories
###############################################################################

optfilename = 'opts.py'

# Try to find options file based on hostname
optfilename_local = 'opts_'+platform.uname()[1]+'_'+platform.uname()[0]+'_'+platform.uname()[4]+'.py'
if len(glob.glob(optfilename_local)) > 0:
    optfilename = optfilename_local
else:
    print 'To use specific options for this system, use options file "'+optfilename_local+'"'

print 'Using options from ' + optfilename

subarch = platform.uname()[4]
opts = Variables(optfilename)

# these are the options that can be specified through the command line
opts.AddVariables(
	EnumVariable('mode', 'Build mode: set to debug or release', 'debug',
                    allowed_values = ('debug', 'release'),
                    ignorecase = 1),
	BoolVariable('profile', 'Enable profiling. Also enables debug information.', 0),
	BoolVariable('runtests', 'Run tests.', 0),
	BoolVariable('debuginfo', 'Include debug information also in release version.', 1),
	BoolVariable('sequential', 'Compile sequential library that does not use MPI (bsp_nprocs() == 1).', 0),
	BoolVariable('threadsafe', 'Make bspwww library thread safe.', 1),
    ('win32_boostdir', 'Path to Boost library in Win32', 'C:\\Boost\\include'),
    ('win32_tbbdir', 'Path to tbb library in Win32', 'C:\\tbb'),
	('win32_ccpdir', 'Path to Microsoft Compute Cluster Pack in Win32', 'C:\\Program Files\\Microsoft Compute Cluster Pack'),
	('toolset', 'Specify compiler and linker tools: gcc|default', 'default'),
	('additional_lflags', 'Additional linker flags', ''),
	('additional_cflags', 'Additional compiler flags', ''),
	('MPICC', 'MPI c compiler wrapper (Unix only)', 'mpicc'),
	('MPICXX', 'MPI c++ compiler wrapper (Unix only)', 'mpicxx'),
	('MPILINK', 'MPI linker wrapper (Unix only)', 'mpicxx'),
	('mpiexec', 'MPI exec command for testing', 'mpiexec'),
	('mpiexec_params', 'MPI exec parameters for testing', '-n 3'),
	)

SCons.Defaults.DefaultEnvironment(tools = [])

# read options before creating root environment
readopts = Environment(tools = [], options = opts)

###############################################################################
# Set up the root environment
###############################################################################

# add qt4 here if necessary
platform_name = platform.uname()[0]
if platform_name == 'Windows':
	ttools = ['msvc', 'mslib', 'mslink']
#	ttools = ['gnulink', 'gcc', 'g++', 'ar']
elif platform_name == 'Linux':
	ttools = ['gnulink', 'gcc', 'g++', 'ar']
else:
	ttools = ['default']

root = Environment(
    tools = ttools,
    toolpath = ['site_scons/site_tools'],
    options = opts,
)
Help(opts.GenerateHelpText(root))

###############################################################################
# Setup compiling parameters
###############################################################################

root.Append(
	ENV = os.environ,
	BINDIR = "#bin",
	LIBDIR = "#lib",
	SRCDIR = "#src",
	)

# dependency optimization
root.SetOption('max_drift', 4)
root.SetOption('implicit_cache', 1)
root.SetOption('diskcheck', None)
root.Ignore('', '')

platform = ARGUMENTS.get('OS', root['PLATFORM'])
mode = root['mode']
toolset = root['toolset']
profile = root['profile']
sequential = root['sequential']
threadsafe = root['threadsafe']
debuginfo = root['debuginfo']
runtests = root['runtests']

win32_tbbdir = root['win32_tbbdir']
win32_boostdir = root['win32_boostdir']

cxx = str(root['CXX'])
print "Compiling on platform "+ platform + " using mode " + mode + " and compiler " + cxx + "."

root.Append(
	CCFLAGS = ' $CFLAGS',
	CPPPATH = ['#include', '#'],
	LIBPATH = ['#lib'],
)

if platform == 'win32':
    # Win32 specific setup
    if cxx != 'g++':
		root.Append(
			CCFLAGS = '/D_SCL_SECURE_NO_WARNINGS',
			CPPPATH = ['.', '#include'],
		)
    else:
		root.Append(
			CCFLAGS = ' -D_SCL_SECURE_NO_WARNINGS',
			CPPPATH = ['.', '#include'],
		)
    boost_include = ''
    boost_lib = ''
    # see if we can find boost
    dirs = glob.glob(win32_boostdir+"\\*")
    if len(dirs) > 0:
    	dirs.sort()
    	boost_include = dirs[len(dirs) - 1]
    	boost_lib = win32_boostdir+"\\..\\lib"

    # boost include path must go into CCFLAGS because the scons include dependency
    # parser can die otherwise
    if cxx != 'g++' and readopts['toolset'] != 'gcc':
		root.Append(
			CPPFLAGS = ' /D_SCL_SECURE_NO_WARNINGS /I'+boost_include ,
			LIBPATH = [ boost_lib ],
		)

		if subarch == 'AMD64':
			root.Append(
				ARFLAGS = '/MACHINE:X64',
				LINKFLAGS = '/MACHINE:X64 /LTCG /LARGEADDRESSAWARE:NO',
			)
		else:
			root.Append(
				ARFLAGS = '/MACHINE:X86',
				LINKFLAGS = '/MACHINE:X86 /LARGEADDRESSAWARE:NO',
			)

    else:
		root.Append(
			CPPFLAGS = ' -I'+boost_include ,
			LIBPATH = [ boost_lib ],
		)


    # see if we can find tbb
    root.Append(
        CPPPATH = [ win32_tbbdir+'\\include' ],
        # we require that the right libraries are copied here, since it's a lot of
        # work to guess the right system and folder.
        LIBPATH = [ win32_tbbdir+'\\lib' ],
    )

    
else:
		# Unix-like stuff
		root.Append(
			CCFLAGS = ' $CFLAGS',
			CPPPATH = ['.', '#include'],
		)

root.Append(
	CCFLAGS = root['additional_cflags'],
	LINKFLAGS = root['additional_lflags'],
	)

###############################################################################
# Setup debug / release mode flags
###############################################################################

if string.find(cxx, 'g++') >= 0 or string.find(cxx, 'c++') >= 0 or root['toolset'] == 'gcc':
	if mode == 'debug':
		root.Append(
		CCFLAGS=' -g -O0',
		)
	elif mode == 'release':
		if debuginfo:
			root.Append(
				CCFLAGS=' -g ',
				LINKFLAGS=' -g ',
			)
		if profile:
			root.Append(
				CCFLAGS=' -pg -O2',
				LINKFLAGS=' -pg',
			)
		else:
			root.Append(
			CCFLAGS=' -O2',
			)
		
	print "using g++"
else:
	if platform == 'win32':
		msvc_ccflags =  " /EHsc /nologo /wd4099 /D_CRT_SECURE_NO_DEPRECATE /WL /Zi"
		msvc_linkflags = " /DEBUG"
		if mode == 'debug':
			root.Append(
			CCFLAGS='/MDd /Od /Zi /W3 /RTC1 /RTCu /RTCs'+msvc_ccflags,
			LINKFLAGS='/DEBUG '+msvc_linkflags,
			LIBLINKPREFIX="/DEBUG "
			)
		elif mode == 'release':
			root.Append( 
			CCFLAGS='/MD /O2 '+msvc_ccflags,
			LINKFLAGS=msvc_linkflags
			)
			if debuginfo or profile:
				root.Append(
					CCFLAGS=' /Zi ',
					LINKFLAGS=' /DEBUG ',
					LIBLINKPREFIX="/DEBUG "
				)							
		print "using MSVC"

###############################################################################
# Automatic configuration code
###############################################################################

# configure environment
print "Performing autoconfiguration"
autoconfig_h_begin = """
/*
 * This file is generated automatically and will be overwritten.
 */ 
#ifndef __BSPCONFIG_H__
#define __BSPCONFIG_H__

"""
	
autoconfig_h_end = """

#endif /* __BSPCONFIG_H__ */

"""	
conf = Configure(root)
autohdr = open("include/bsp_config.h", 'w')
autohdr.write(autoconfig_h_begin);

if subarch == 'x86_64':
	autohdr.write("#define _X86_64 \n")

if platform == 'win32':
	autohdr.write(
"""
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

""")
	autohdr.write("#define RESTRICT \n")
	autohdr.write("#define __func__ __FUNCTION__ \n")
	autohdr.write("#define BSP_CALLING __cdecl \n") 
	autohdr.write("#define STDCALL __stdcall \n") 
else:
	autohdr.write("#define RESTRICT \n")
	autohdr.write("#define BSP_CALLING \n") 
	autohdr.write("#define STDCALL \n") 

autohdr.write("""
	
#ifdef UNITTESTING
#include "../tests/bsp_test.h"
#else  

#ifdef _HAVE_MPI
#include <mpi.h>

#define _BSP_INIT BSP_INIT_MPI
#define _BSP_EXIT BSP_EXIT_MPI
#define _BSP_ABORT BSP_ABORT_MPI
#define _BSP_COMM0 BSP_MPI_ALLTOALL_COMM
#define _BSP_COMM1 BSP_MPI_ALLTOALLV_COMM

#else 

#define _BSP_INIT BSP_INIT_SEQ
#define _BSP_EXIT BSP_EXIT_SEQ
#define _BSP_ABORT BSP_ABORT_SEQ
#define _BSP_COMM0 BSP_SEQ_ALLTOALL_COMM
#define _BSP_COMM1 BSP_SEQ_ALLTOALLV_COMM

#endif 

#endif  

#ifdef _DEBUG
#include <assert.h>
#define ASSERT assert
#else
#define ASSERT(x)
#endif

""")
	
autohdr.write(autoconfig_h_end)
root = conf.Finish()
autohdr.close()

###############################################################################
# Setup TBB library linking
###############################################################################

if mode=='debug':
	if platform == 'win32':
		root.Append(LIBS = ['tbb_debug'])
	else:
		root.Append(LIBS = ['tbb'])
else:
	root.Append(LIBS = ['tbb'])

###############################################################################
# Set up thread safe version of BSP
###############################################################################

if threadsafe:
	root.Append (CPPDEFINES = ["BSP_THREADSAFE"])

###############################################################################
# Setup MPI capable environment
###############################################################################

mpi = root.Clone()

if not sequential:
	if platform == 'win32':
		win32_ccpdir = root['win32_ccpdir']
		mpi.Append(	CPPPATH = win32_ccpdir+"\\Include",
				LIBS = ["msmpi.lib", "msmpe.lib"]
		  )
		if subarch == 'AMD64':
			mpi.Append(	LIBPATH = win32_ccpdir+"\\Lib\\amd64" )
		else:
			mpi.Append(	LIBPATH = win32_ccpdir+"\\Lib\\i386" )
	else:
		mpi.Replace(
			CXX = root['MPICXX'],
			LINK = root['MPILINK'],
			CC = root['MPICC']
		)
	mpi.Append (CPPDEFINES = "_HAVE_MPI")
else:
	mpi.Append (CPPDEFINES = "_NO_MPI")

###############################################################################
# Set up unit testing
###############################################################################

def builder_unit_test(target, source, env):	
	app = str(source[0].abspath)
	if os.spawnl(os.P_WAIT, app, app) == 0:
		open(str(target[0]),'w').write("PASSED\n")
	else:
		return 1


def builder_unit_test_mpi(target, source, env):
	# for MPI tests, we run with these processor counts
	
	mpiexec = env["mpiexec"]
	mpiexec_params = env["mpiexec_params"]

	app = str(source[0].abspath)
	runme = mpiexec + " " + mpiexec_params + ' "' + app + '"'

	print "Test: running " + runme

	if os.system(runme) == 0:
		open(str(target[0]),'w').write("PASSED\n")
	else:
		return 1

# Create a builder for tests
if sequential:
	bld = Builder(action = builder_unit_test)
	mpi.Append(BUILDERS = {'Test' :  bld})
else:
	bld = Builder(action = builder_unit_test_mpi)
	mpi.Append(BUILDERS = {'Test' :  bld})

###############################################################################
# Export our build environments for the SConscripts
###############################################################################

libsuffix = ''

if sequential:
	libsuffix += 'nompi'

if mode == 'debug':
	libsuffix += '_debug'

if threadsafe:
	libsuffix += '_mt'


Export(['mpi', 'mode', 'sequential', 'threadsafe', 'libsuffix'])

###############################################################################
# get SConscripts
###############################################################################

SConscript('src/SConscript')

if runtests:
	SConscript('tests/SConscript')

SConscript('examples/SConscript')
