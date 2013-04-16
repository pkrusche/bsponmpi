import platform
import glob

###############################################################################
# Setup Boost library 
###############################################################################

###############################################################################
# Check for presence of boost in a config context
###############################################################################

def Check(context, version):
    # Boost versions are in format major.minor.subminor
	v_arr = version.split(".")
	version_n = 0
	if len(v_arr) > 0:
		version_n += int(v_arr[0])*100000
	if len(v_arr) > 1:
		version_n += int(v_arr[1])*100
	if len(v_arr) > 2:
		version_n += int(v_arr[2])
        
	context.Message('Checking for Boost version >= %s... ' % (version))

	ret = context.TryCompile("""
#include <boost/version.hpp>

#if BOOST_VERSION < %d
#error Installed boost is too old!
#endif
    int main() 
    {
        return 0;
    }
    """ % version_n, '.cpp')
	context.Result(ret)
	return ret

###############################################################################
# Add Boost-specific options
###############################################################################

def MakeOptions(opts):
	opts.AddVariables (
	    ('boostdir', 'Path to Boost library in Win32', 'C:\\Boost\\include'),
	)

###############################################################################
# Find boost and add to environment
###############################################################################

def MakeEnv ( env ):
    boost_include = ''
    boost_lib = ''
    boostdir = env['boostdir']

	# see if we can find boost
    dirs = glob.glob(boostdir+"\\*")
    if len(dirs) > 0:
    	dirs.sort()
    	boost_include = dirs[len(dirs) - 1]
    	boost_lib = boostdir+"\\..\\lib"

    # boost include path must go into CCFLAGS because the scons include dependency
    # parser may die otherwise

	## this is what we do for 
    if env['toolset'] == 'msvc' or env['toolset'] == 'intel_windows':
		env.Append(
			CPPFLAGS = '/I'+boost_include ,
			LIBPATH = [ boost_lib ],
		)
    else:
    	if boost_include:
			env.Append(
				CPPFLAGS = ' -I'+boost_include )

    	if boost_lib:
			env.Append(
				LIBPATH = [ boost_lib ] )
    	env.Append(	LIBS = 'boost_program_options'  )
