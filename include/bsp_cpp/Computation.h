/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/
#ifndef Computation_h__
#define Computation_h__


#include "util/TypeList.h"
#include "ProcMapper.h"
#include "Superstep.h"

namespace bsp {
	template<class _superstep_list,
			 class _context = utilities::NullType,
			 class _procmapper = ProcMapper<_context>
	>
	class FlatComputation;

	/**
	 * @brief For an empty type list, just sync
	 */
	template<class _context,
			 class _procmapper>
	class FlatComputation<utilities::NullType, _context, _procmapper> {
	public:
		typedef _procmapper procmapper_t;

		static void run(_procmapper & p) {
			bsp_sync();
		}
	};

	/**
	 * @brief Specialisation that executes the typelist computations one by one, and sync's in between
	 */
	template<class H, class T, class _context, class _procmapper>
	class FlatComputation<utilities::Typelist<H, T>, _context, _procmapper > {
	public:
		typedef FlatComputation<T, _context, _procmapper> tail;
		typedef _procmapper procmapper_t;
		static void run(_procmapper & p) {
			bsp_sync();
			{
				H computation(p);
				computation.start();
				computation.join();
			}
			tail::run (p);
		}
	};


#define BSP_SUPERSTEP_DEF_BEGIN(name, context_type, procmapper_type)	  \
	class _impl_##name;													  \
	typedef  bsp::Superstep<_impl_##name, procmapper_type >				  \
		name ;														  \
	class _impl_##name : public bsp::DefaultSuperstep <context_type> {	  \
	public:																  \
		_impl_##name () {}												  \
		_impl_##name (_impl_##name const & c) {}						  \
		\
		virtual ~ _impl_##name () {}									  \
																		  \
		void run() {													  \
			context_type & context = get_context();

#define BSP_SUPERSTEP_DEF_END()		\
	} };

};

#endif // Computation_h__
