/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#ifndef __BSP_CPP_H__
#define __BSP_CPP_H__

#include "bsp.h"

#include <tbb/task_scheduler_init.h>
#include <util/TypeList.h>
#include <string>
#include <string.h>

#include "bspcpp/tools/singletons.h"

#include "bsp_broadcast.h"
#include "ParameterFile.h"

/**
 * @mainpage BSP C++ library
 * 
 */
namespace bsp {

	/**
	 * return the default number of threads
	 */
	inline int bsp_nthreads() {
		return tbb::task_scheduler_init::default_num_threads();
	}

	template <typename _t>
	void bsp_broadcast(int source, _t & data) {
		::bsp_broadcast(source, &data, sizeof(_t));
	}

	template <>
	void bsp_broadcast(int source, std::string & data) {
		size_t len;

		len = data.size();
		bsp_broadcast(source, len);

		char * data_copy = new char[len];

		if(bsp_pid() == source) {
			data.resize(len);
			memcpy(data_copy, data.c_str(), len);
		}

		::bsp_broadcast(source, data_copy, len);
		data = std::string(data_copy,len);
		delete [] data_copy;
	}

	template <>
	void bsp_broadcast(int source, utilities::Parameter & data) {
		using namespace std;
		using namespace utilities;
		bsp_broadcast(source, data.len);
		bsp_broadcast(source, data.last_update);
		bsp_broadcast(source, data.type);
		bsp_broadcast(source, data.updated);

		if (bsp_pid() != source) {
			data.data = boost::shared_array<char>(new char[data.len]);
		}
		::bsp_broadcast(source, data.data.get(), data.len);
	}

	template <>
	void bsp_broadcast(int source, utilities::ParameterFile & data) {
		using namespace std;
		using namespace utilities;
		size_t size = data.getcontents().size();
		bsp_broadcast(source, size);

		if (bsp_pid() == source) {
			for (map<string, Parameter>::iterator it = data.getcontents().begin(); it != data.getcontents().end(); ++it) {
				string s1 = it->first; 
				bsp_broadcast(source, s1);
				bsp_broadcast(source, it->second);
			}
		} else {
			for (size_t j = 0; j < size; ++j) {
				string name;
				Parameter value; 
				bsp_broadcast(source, name);
				bsp_broadcast(source, value);
				data.set(name.c_str(), value);
			}
		}
	}

};

#include "Superstep.h"
#include "Computation.h"
#include "Avector.h"
#include "tools/utilities.h"
#include "tools/aligned_allocator.h"
#include "tools/singletons.h"
#include "tools/global_params.h"

#endif
