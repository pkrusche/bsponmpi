###############################################################################
# Make add options to an environment to use GCC
###############################################################################

#
# Required options in root env:
#   mode		: 'debug' or 'release
#   debuginfo	: true or false  to include debug info also in release version
#   profile  	: true or false  to enable/disable gprof support
#

def PrepareEnv (root):
	mode      = root['mode']
	debuginfo = root['debuginfo']
	profile   = root['profile']

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

Export( [ 'PrepareEnv'] )