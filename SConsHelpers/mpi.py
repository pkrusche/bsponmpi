import platform
import sys
import os.path

from SCons.Variables import BoolVariable

###############################################################################
# Make an MPI environment
###############################################################################

def Check(context, version):
	version_n = 1
	ret = 0
	if version < 2:
		ret = context.TryCompile("""
#include <mpi.h>

int main(int argc, char ** argv) {
	MPI_Init (&argc, &argv);

	MPI_Finalize();
	return 0;
}
""", '.cpp')
	else:
		## MPI 2: check if we can call MPI_Get
		ret = context.TryCompile("""
#include <mpi.h>

int main(int argc, char ** argv) {
	MPI_Init (&argc, &argv);
	MPI_Win w;
	MPI_Get(NULL, 1, MPI_BYTE, 0, 0, 1, MPI_BYTE, w);
	MPI_Finalize();
	return 0;
}
""", '.cpp')
	context.Result(ret)

	return ret

###############################################################################
# Add MPI Options
###############################################################################

def MakeOptions(opts):
	arch   = platform.uname()[0]
	opts.AddVariables(
		BoolVariable('sequential', 'Compile library that does not use MPI.', 0)
	)
	if arch == 'Windows':
		opts.AddVariables(
			('MPIDIR', 'Path to Microsoft Compute Cluster Pack in Win32', 'C:\\Program Files\\Microsoft Compute Cluster Pack'),
		)
	else:
		opts.AddVariables(
			('MPICC', 'MPI C compiler wrapper (Unix only)', 'mpicc'),
			('MPICXX', 'MPI C++ compiler wrapper (Unix only)', 'mpicxx'),
			('MPILINK', 'MPI linker wrapper (Unix only)', 'mpicxx'),
		)

###############################################################################
# Make an MPI-capable environment
###############################################################################

def MakeEnv(root):
	arch   = platform.uname()[0]
	subarch   = platform.uname()[4]

	if not root['sequential']:
		if arch == 'Windows':
			mpidir = root['MPIDIR']
			
			if os.path.exists (mpidir + "\\lib\\msmpi.lib"):
				print "Found MSMPI in " + mpidir
				
				## TODO add support for HPC pack here

				root.Append(CPPPATH = mpidir+"\\Include",
					LIBS = ["msmpi.lib", "msmpe.lib"]
				)

				if subarch == 'AMD64':
					root.Append( LIBPATH = mpidir+"\\Lib\\amd64" )
				else:
					root.Append(	LIBPATH = mpidir+"\\Lib\\i386" )
			else:
				print "Found other MPI in " + mpidir
				root.Append( CPPPATH = mpidir+"\\include",
					LIBPATH = mpidir+"\\lib" ,
					LIBS = ["mpi", "mpi_cxx"]
				)
		else:
			root.Replace(
				CXX = root['MPICXX'],
				LINK = root['MPILINK'],
				CC = root['MPICC']
			)
		root.Append (CPPDEFINES = "_HAVE_MPI")
	else:
		root.Append (CPPDEFINES = "_NO_MPI")

