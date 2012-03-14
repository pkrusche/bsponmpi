import platform
import os.path
import re

###############################################################################
# Setup TBB library linking
###############################################################################

###############################################################################
# Check for presence of TBB in a config context
###############################################################################

def Check(context):       
	context.Message('Checking CBLAS')
	ret = context.TryLink("""
#include "../include/helpers/cblas.h"

int main(int argc, char ** argv) {
	double * vecA, * vecB;
	int n;
	cblas_ddot(n, vecA, 1,vecB, 1);
	return 0;
}
""", '.c')
	return ret

def MakeOptions (opts):
	arch   = platform.uname()[0]
	opts.AddVariables(
		('cblas', 'Which version of CBLAS to use. Allowed values: none|blas|atlas|openblas|accelerate (MacOS X only)', 'none'),
	)

###############################################################################
# Add TBB to an enviroment
###############################################################################

def MakeEnv (root):
	platform_name = platform.uname()[0]
	subarch = platform.uname()[4]
	cblas = root['cblas']
	
	if cblas == 'blas':
		root.Append(LIBS='blas')
	elif cblas == 'atlas':
		root.Append(LIBS=[ 'cblas', 'atlas' ])
	elif cblas == 'openblas':
		root.Append(LIBS=[ 'openblas' ])
	elif cblas == 'accelerate':
		root.Prepend(LINKFLAGS = '-framework Accelerate')

