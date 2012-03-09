import platform
import os.path
import re

###############################################################################
# Setup TBB library linking
###############################################################################

###############################################################################
# Check for presence of TBB in a config context
###############################################################################

def Check(context, version):       
	context.Message('Checking for TBB version >= %s.0... ' % (version))

	ret = context.TryLink("""
#include <tbb/tbb_stddef.h>

#if TBB_VERSION_MAJOR < %d
#error Installed TBB is too old!
#endif
int main() 
{
    return 0;
}
""" % int (version), '.cpp')
	context.Result(ret)	

	return ret

def MakeOptions (opts):
	arch   = platform.uname()[0]
	if arch == 'Windows':
		opts.AddVariables(
			('tbbdir', 'Path to TBB library distribution.', 'C:\\tbb'),
		)
	else:
		opts.AddVariables(
			('tbbdir', 'Path to TBB library distribution.', ''),
		)

###############################################################################
# Add TBB to an enviroment
###############################################################################

def MakeEnv (root):
	platform_name = platform.uname()[0]
	subarch = platform.uname()[4]
	tbbdir = root['tbbdir']

	if tbbdir != '':
		if platform_name == 'Windows':
			root.Append( CPPPATH = [ tbbdir+'\\include' ] )

			if root['toolset'] == "msvc":
				msvcver =  re.split (r"\.", root['MSVC_VERSION'])
				archsubdir = 'ia32'
				if subarch == "AMD64":
					archsubdir = 'intel64'

				searchpath = tbbdir+"\\lib\\" + archsubdir + "\\vc" + str(msvcver[0])

				if os.path.exists (searchpath):
					root.Append( 
						LIBPATH = [ searchpath ]
					)
				else:
					print "Your version of MSVC isn't supported by your version of TBB, please place TBB libraries for your compiler in " + tbbdir + "\\lib"
					root.Append( LIBPATH = tbbdir + "\\lib" )
					
			else:
				print "When not using MSVC on Windows, please place TBB libraries for your compiler in " + tbbdir + "\\lib"
				root.Append( LIBPATH = tbbdir + "\\lib" )
		else:
			root.Append( 
				CPPPATH = [ tbbdir+'/include',  ],
				LIBPATH = [ tbbdir + "/lib", ], 
			)

	if platform_name == 'Windows':
		if root['mode'] == 'debug':
			root.Append( 
				LIBS = ['tbb_debug', 'tbbmalloc_debug'],
			)
		else:
			root.Append( 
				LIBS = ['tbb', 'tbbmalloc'],
			)
	else:
		root.Append( 
			LIBS = ['tbb', 'tbbmalloc'],
		)
