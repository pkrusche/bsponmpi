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
# Modular helpers
###############################################################################

import SConsHelpers.mpi
import SConsHelpers.tbb
import SConsHelpers.boost
import SConsHelpers.autoconfig

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
	BoolVariable('threadsafe', 'Make bspwww library thread safe.', 1),
	('toolset', 'Specify compiler and linker tools: msvc|gnu|intel', 'gnu'),
	('additional_lflags', 'Additional linker flags', ''),
	('additional_cflags', 'Additional compiler flags', ''),
	('mpiexec', 'MPI exec command for testing', 'mpiexec'),
	('mpiexec_params', 'MPI exec parameters for testing', '-n 3')
	)

SCons.Defaults.DefaultEnvironment(tools = [])

# read options before creating root environment
readopts = Environment(tools = [], options = opts)

###############################################################################
# Set up the root environment
###############################################################################

toolset = readopts['toolset']

if toolset == 'msvc':
	ttools = ['msvc', 'mslib', 'mslink']
elif toolset == 'gnu':
	ttools = ['gnulink', 'gcc', 'g++','c++','ar']
elif toolset == 'intel':
	ttools = ['icc', 'ilink', 'intelc' ,'ar']
elif toolset == 'intel_windows':
	ttools = ['ilink', 'icl', 'mslib']
else:
	print "[W] Unknown toolset " + toolset + ", using default tools"
	ttools = ['default']

## add included options
SConsHelpers.mpi.MakeOptions(opts)
SConsHelpers.boost.MakeOptions(opts)
SConsHelpers.tbb.MakeOptions(opts)

root = Environment(
    tools = ttools,
    options = opts,
)

# load toolset specific implementation of PrepareEnv
SConscript ('SConsHelpers/toolsets/'+toolset+'.py')

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

platform_name = platform.uname()[0]
platform = ARGUMENTS.get('OS', root['PLATFORM'])

mode = root['mode']
profile = root['profile']
sequential = root['sequential']
threadsafe = root['threadsafe']
debuginfo = root['debuginfo']
runtests = root['runtests']

root.Append(
	CCFLAGS = ' $CFLAGS',
	CPPPATH = ['#include', '#src'],
	LIBPATH = ['#lib'],
)

###############################################################################
# Setup debug / release mode flags
###############################################################################

Import ('PrepareEnv')
PrepareEnv(root)

###############################################################################
# Setup Boost, TBB and MPI library linking
###############################################################################

SConsHelpers.tbb.MakeEnv(root)
SConsHelpers.mpi.MakeEnv(root)
SConsHelpers.boost.MakeEnv(root)

## additional flags not covered by any of the above
root.Append ( 
	LINKFLAGS = root['additional_lflags'],
	CCFLAGS = root['additional_cflags'] 
)

###############################################################################
# Automatic configuration code
###############################################################################

def ConfRunner(conf, autohdr):
	if not root['sequential']:
		if not conf.CheckMPI(2):
			print "You have enabled MPI, but I could not find an installation. Have a look at SConsHelpers/mpi.py"
			Exit (1)

	if not conf.CheckBoost('1.45'):
		print "I could not find Boost >= 1.48. Have a look at SConsHelpers/boost.py"
		Exit (1)

	if not conf.CheckTBB(3):
		print "I could not find Intel TBB version >= 3.0. Have a look at SConsHelpers/tbb.py"
		Exit (1)

SConsHelpers.autoconfig.AutoConfig ( root, ConfRunner, { 
	'CheckBoost' : SConsHelpers.boost.Check,
	'CheckMPI' : SConsHelpers.mpi.Check,
	'CheckTBB' : SConsHelpers.tbb.Check,
	} )

###############################################################################
# Set up thread safe version of BSP
###############################################################################

if threadsafe:
	root.Append (CPPDEFINES = ["BSP_THREADSAFE"])

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
	root.Append(BUILDERS = {'Test' :  bld})
else:
	bld = Builder(action = builder_unit_test_mpi)
	root.Append(BUILDERS = {'Test' :  bld})


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

bsp = root.Clone()
bsp.Prepend(LIBS = ['bsponmpi'+libsuffix ,])

Export(['root', 'bsp', 'sequential' , 'libsuffix'])

###############################################################################
# get SConscripts
###############################################################################

SConscript('src/SConscript')

if runtests:
	SConscript('tests/SConscript')

SConscript('examples/SConscript')
