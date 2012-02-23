/***************************************************************************
 *   Copyright (C) 2011   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/
#ifndef __global_params_H__
#define __global_params_H__

namespace bsp {
	/*!
	 * \brief read global options files
	 *
	 *
	 */
	void bspcpp_read_global_options() {
		if(bsp_pid() == 0) {
			// check current path, and home directory on unix systems
			singletons::global_options.read(".bspcpp_global_params");
#ifndef _WIN32
			singletons::global_options.read("~/.bspcpp_global_params");
#endif
		}
		// broadcast, so everyone gets it.
		bsp_broadcast(0, singletons::global_options);
	}
};

#endif // __global_params_H__
