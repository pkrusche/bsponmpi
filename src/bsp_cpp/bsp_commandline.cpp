/***************************************************************************
 *   Copyright (C) 2012   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#include "bsp_config.h"

#include "bsp_cpp/bsp_commandline.h"

#include "bsp.h"
#include "bsp_broadcast.h"

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
	) {

		if (p_from >= 0) {
			size_t argc = args.size();
			bsp_broadcast(0, argc);

			if (bsp_pid() != p_from) {
				args.resize(argc);
			}

			for (size_t a = 0; a < argc; ++a) {
				bsp_broadcast(0, args[a]);
			}
		}

		namespace po = boost::program_options;
		po::store(po::command_line_parser(args).
			options(all_opts).run(), vm);
		vm.notify();
	}

	/**
	 * We can do the same when we have a command line as a (possibly multi-line) string.
	 */

	void bsp_command_line (
		std::string const & cmdline,
		boost::program_options::options_description & all_opts, 
		boost::program_options::variables_map & vm
		) {
		using namespace std;
		using namespace boost;

		char_separator<char> sep(" \n\r");
		tokenizer<char_separator<char> > tok(cmdline, sep);
		vector<string> args;
		copy(tok.begin(), tok.end(), back_inserter(args));

		bsp_command_line(-1, args, all_opts, vm);
	}

	/**
	 * This is how we handle standard C args. We use the ones passed on node 0.
	 */

	void bsp_command_line (
		int argc, char ** argv, 
		boost::program_options::options_description & all_opts, 
		boost::program_options::variables_map & vm) {

		std::vector<std::string> args;

		if (bsp_pid() == 0) {
			for (int a = 0; a < argc-1; ++a) {
				args.insert(args.end(), argv[a+1]);
			}
		}
		bsp_command_line(0, args, all_opts, vm);
	}

};

