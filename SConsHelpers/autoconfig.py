
import platform

###############################################################################
# Automatic configuration code
###############################################################################

"""Autoconfig function

@param root an environment to use
@param crunner a config runner function (def crunner (conf, autohdr) )
@param customtests a set of custom tests to use for the config enviroment
"""

def AutoConfig (root, crunner,customtests = {}):	
	platform_name = platform.uname()[0]
	subarch = platform.uname()[4]

	conf = root.Configure(custom_tests = customtests )
	
	autohdr = open("include/bsp_config.h", 'w')

	# configure environment
	print "Performing autoconfiguration for " + platform_name + "/" + subarch
	autohdr.write("""
/*
 * This file is generated automatically and will be overwritten whenever BSPonMPI is built.
 */ 
#ifndef __BSPCONFIG_H__
#define __BSPCONFIG_H__

#include <tbb/scalable_allocator.h>

/** No MSVC => no __cdecl */
#ifndef _MSC_VER
#ifndef __cdecl
#define __cdecl
#endif 
#endif 

#ifdef _DEBUG
#include <assert.h>
#define ASSERT assert
#else
#define ASSERT(x)
#endif

""")
	
	if subarch == 'x86_64':
		autohdr.write("#define _X86_64 \n")

	if platform_name == 'Windows':
		autohdr.write("""
#ifndef __cplusplus
#define inline 
#endif

#ifdef _MSC_VER
#define _SCL_SECURE_NO_WARNINGS
#endif

#define BSP_CALLING __cdecl
#define __func__ __FUNCTION__ 
#define RESTRICT

""")
	else:
		autohdr.write("""
#define RESTRICT
#define BSP_CALLING
""")

	crunner(conf, autohdr)

	autohdr.write("""
#endif /* __BSPCONFIG_H__ */
""")


	root = conf.Finish()
	autohdr.close()

