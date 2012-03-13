/*
BSPonMPI. This is an implementation of the BSPlib standard on top of MPI
Copyright (C) 2006  Wijnand J. Suijlen, 2012 Peter Krusche

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

See the AUTHORS file distributed with this library for author contact
information.
*/


/** @file SharedVariable.h

@author Peter Krusche
*/

#ifndef __SharedVariable_H__
#define __SharedVariable_H__

#include <string.h>
#include <stdlib.h>

#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>

#include <tbb/parallel_reduce.h>
#include <tbb/parallel_for.h>

#include "Shared.h"
#include "SharedVariableSet.h"

#include "Initializers.h"
#include "Reducers.h"

namespace bsp {

	/** Generic initializer which is usable in tbb::parallel_for 
	 * 
	 * Takes a variable type and an initializer of this form:
	 * 
	 * @code
	 * struct _init {
	 *		void operator () (const var & input, _var & v) {
	 *			// do something to v
	 *		}
	 * }
	 * @end
	 *
	 */
	template <class _var, template<class> class _init>
	struct Initializer {
		Initializer ( const _var * _input, 
			std::vector<Shared*> const & _vars ) : input(_input), vars(_vars) {} 

		void operator()( const tbb::blocked_range<size_t>& r ) const { 
			static _init<typename _var :: value_type > initer;
			for( size_t i = r.begin(); i != r.end(); ++i ) {
				initer ( (typename _var::value_type const & )(*input), (typename _var::value_type & ) (* ((_var*)vars[i])) );
			}
		} 

		const _var * input;
		std::vector<Shared*> const & vars;
	};

	/** Generic reducer that takes a variable type and an operator of the form
	 *
	 * @code
	 * struct _op {
	 *		void operator (_var::value_type & l, _var::value_type const & r);
	 * }
	 * @end
	 *
	 * (l is merged with r (a'la foldl))
	 *  
	 * and makes it usable with tbb::parallel_reduce
	 *  
	 */
	template <class _var, template<class> class _op>
	struct Reducer {
		Reducer ( _var * dst, std::vector<Shared*> & _vars ) : vars(_vars), mine(dst) {} 

		Reducer ( Reducer & r, tbb::split ) : vars (r.vars), mine (r.mine) {}

		void join( const Reducer & y ) { 
			static _op< typename _var::value_type > red;
			red ( (typename _var::value_type & ) *mine,  (typename _var::value_type const & ) (*y.mine) );
		} 

		void operator()( const tbb::blocked_range<size_t>& r ) { 
			static _op< typename _var::value_type > red;

			for( size_t i = r.begin(); i != r.end(); ++i ) {
				red ( (typename _var::value_type & ) *mine,  (typename _var::value_type const & ) (* ((_var*)vars[i])) );
			}
		}

		std::vector< Shared* > & vars;
		_var * mine;
	};


	/** Node-local shared variable implementation */
	template <class _t, template<class> class _init = InitAssign, template<class> class _red = ReduceFirst>
	class SharedVariable : public Shared {
	public:
		typedef SharedVariable <_t, _init, _red> my_type;
		typedef _t value_type;

		SharedVariable() : valadr(new _t), deleteme(true) {}

		SharedVariable(_t & mine) : valadr (&mine), deleteme (false) {}

		~SharedVariable() {
			if (deleteme) {
				delete valadr;
			}
		}

		/** Run initializer in tbb parallel for */
		bool initialize() {
			Initializer<my_type, _init> i (this, vars);
			tbb::parallel_for( tbb::blocked_range<size_t> (0, vars.size()), i );
			return true;
		}
		
		/** Run reducer in tbb parallel for */
		bool reduce() {
			Reducer<my_type, _red> r (this, vars);
			tbb::parallel_reduce( tbb::blocked_range<size_t> (0, vars.size()), r );
			return true;
		}

		/** convert to _t const & */
		operator _t const & () const {
			return *valadr;
		}

		/** convert to _t const & */
		operator _t & () {
			return *valadr;
		}

		/** Byte Serialization. Provide specializations if necessary: 
		 * 
		 * 
		 * @code
		 * template < template<class> class _init = InitDummy, template<class> class _red > 
		 * SharedVariable <yourtype, _init, red>::serialize/deserialize/...(void * target, size_t nbytes) {
		 *		// your code here
		 * }
		 * @end
		 * 
		 * we only allow the variants here to be used on scalar types.
		 * 
		 */

		/** default byte serialization */
		void serialize(void * target, size_t nbytes) {
			BOOST_STATIC_ASSERT(boost::is_scalar<_t>::value);
			memcpy(target, valadr, sizeof(_t));
		}

		/** default byte deserialization */
		void deserialize(void * source, size_t nbytes) {
			BOOST_STATIC_ASSERT(boost::is_scalar<_t>::value);
			memcpy(valadr, source, sizeof(_t));
		}

		/** default size */
		size_t serialized_size() {
			BOOST_STATIC_ASSERT(boost::is_scalar<_t>::value);
			return sizeof(_t);
		}

	private:
		_t * valadr;
		bool deleteme;
	};
};


/** helper macro to add a node reduction slot */
#define __SHARE_VARIABLE_NODEREDUCE(set, init, reduce, var, ...)	\
	if ( set.get_reduce_slot (#var) == NULL ) {					\
		set.init_reduce_slot(#var, new bsp::SharedVariable< __VA_ARGS__, init, reduce >() );	\
	}
	

#define SHARE_VARIABLE_IR(set, init, reduce, var, ...) \
	do { set.add_var(#var, new bsp::SharedVariable< __VA_ARGS__, init, reduce >(var), true, true ); \
	__SHARE_VARIABLE_NODEREDUCE(set, init, reduce, var, __VA_ARGS__)		\
} while(0)

#define SHARE_VARIABLE_I(set, init, reduce, var, ...) \
	do { set.add_var(#var, new bsp::SharedVariable< __VA_ARGS__, init, reduce >(var), true, false ); } while(0)

#define SHARE_VARIABLE_R(set, init, reduce, var, ...) \
	do { set.add_var(#var, new bsp::SharedVariable< __VA_ARGS__, init, reduce >(var), false, true ); \
	__SHARE_VARIABLE_NODEREDUCE(set, init, reduce, var, __VA_ARGS__)		\
} while(0)

#endif // __SharedVariable_H__
