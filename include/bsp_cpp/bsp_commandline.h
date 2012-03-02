/***************************************************************************
 *   Copyright (C) 2012   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#ifndef __bsp_commandline_H__
#define __bsp_commandline_H__

#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>

#include <vector>
#include <string>

/************************************************************************/
/* BSP Command line handling.                                           */
/************************************************************************/

/**
 * When on MPI, some implementations only make command line arguments 
 * available to the first node. Sucks for us when we want to do the 
 * parsing transparently on all nodes. Here are some functions to make 
 * things easier, together with boost program options.
 * 
 * Note: The code here shouldn't be used for reading large inputs.
 * 
 * The code here is SPMD, it will use bsp_broadcast.
 *
 */

namespace bsp {

	/**
	 * Core command line handler. We get a vector of strings containing 
	 * all the arguments on a given processor (normally, this would be
	 * processor 0), broadcast these to all, and store them in the
	 * boost variable map. in SPMD style, you then declare your
	 * boost option descriptions just like in sequential code.
	 * 
	 * If we specify a value smaller than 0 for p_from, no broadcasting
	 * will be done.
	 * 
	 */
	void bsp_command_line (
		int p_from,
		std::vector<std::string> & args,
		boost::program_options::options_description & all_opts, 
		boost::program_options::variables_map & vm
	);

	/**
	 * We can do the same when we have a command line as a (possibly multi-line) string.
	 */

	void bsp_command_line (
		std::string const & cmdline,
		boost::program_options::options_description & all_opts, 
		boost::program_options::variables_map & vm
	);

	/**
	 * This is how we handle standard C args. We use the ones passed on node 0.
	 */

	void bsp_command_line (
		int argc, char ** argv, 
		boost::program_options::options_description & all_opts, 
		boost::program_options::variables_map & vm);
};

#endif // __bsp_commandline_H__