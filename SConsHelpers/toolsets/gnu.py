import platform
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
	platform_name = platform.uname()[0]

	if mode == 'debug':
		root.Append(
		CCFLAGS=' -g -O0',
		)
	elif mode == 'release':
		fast = '-Ofast'
		if platform_name == 'Darwin':
			fast= '-fast'
		if debuginfo:
			root.Append(
				CCFLAGS=' -g ',
				LINKFLAGS=' -g ',
			)
		if profile:
			root.Append(
				CCFLAGS=' -pg -O3',
				LINKFLAGS=' -pg',
			)
		else:
			root.Append(
			CCFLAGS=fast,
			)
		root.Replace(LINK = 'g++')

Export( [ 'PrepareEnv'] )