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
	template <class _var, typename _init>
	struct Initializer {
		Initializer ( const _var * _input, 
			std::vector<Shared*> const & _vars ) : input(_input), vars(_vars) {} 

		void operator()( const tbb::blocked_range<size_t>& r ) const { 
			static _init initer;
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
	template <class _var, typename _op>
	struct Reducer {
		Reducer ( std::vector<Shared*> & _vars, typename _var::value_type const & _neutral ) : 
			vars(_vars), neutral(_neutral), 
			result ( neutral ) {}

		Reducer ( Reducer & r, tbb::split ) : 
			vars (r.vars), neutral(r.neutral),
			result ( r.neutral ) {}

		void join( const Reducer & y ) { 
			static _op red;
			red (result, y.result);
		} 

		void operator()( const tbb::blocked_range<size_t>& r ) { 
			static _op red;

			for( size_t i = r.begin(); i != r.end(); ++i ) {
				red ( result,  (typename _var::value_type const & ) (* ((_var*)vars[i])) );
			}
		}

		std::vector< Shared* > & vars;
		typename _var::value_type neutral;
		typename _var::value_type result;
	};
	
	/** Serialization is implemented by this class and its specializations. 
	 *
	 * @see Serializers.inl
	 */
	template <class _t> 
	class SharedSerializable : public Shared {
	public:
		typedef _t value_type;

		/** Byte Serialization. Provide specializations if necessary: 
		 * 
		 * 
		 * @code
		 * template <> 
		 * bsp::SharedSerializable < yourtype >::serialize/deserialize/...(void * target, size_t nbytes) {
		 *		// your code here
		 * }
		 * @end
		 * 
		 * we only allow the variants here to be used on scalar types.
		 * 
		 * @see Serializers.inl
		 * 
		 */

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

	protected:
		_t * valadr;
	};

	/** Node-local shared variable implementation */
	template <class _t, typename _init, typename _red>
	class SharedVariable : public SharedSerializable<_t> {
	public:
		typedef SharedVariable <_t, _init, _red> my_type;
		SharedVariable( _t const & _neutral ) : neutral(_neutral), deleteme(true) {
			this->valadr = new _t (neutral);
		}

		SharedVariable(_t & mine, _t const & _neutral) : 
			neutral(_neutral),
			deleteme (false) {
			this->valadr = &mine;
		}

		~SharedVariable() {
			if (deleteme) {
				delete this->valadr;
			}
		}

		/** Run initializer in tbb parallel for */
		bool initialize() {
			Initializer<my_type, _init> i (this, this->vars);
			tbb::parallel_for( tbb::blocked_range<size_t> (0, this->vars.size()), i );
			return true;
		}
		
		/** Run reducer in tbb parallel reduce */
		bool reduce() {
			static _red rr;
			Reducer<my_type, _red> r ( this->vars, neutral );
			tbb::parallel_reduce( tbb::blocked_range<size_t> (0, this->vars.size()), r );
			rr (*(this->valadr), r.result);
			return true;
		}

		/** assign neutral value */
		void make_neutral() {
			*(this->valadr) = neutral;
		}

		/** convert to _t const & */
		operator _t const & () const {
			return *(this->valadr);
		}

		/** convert to _t const & */
		operator _t & () {
			return *(this->valadr);
		}
	private:
		bool deleteme;
		_t neutral;
	};

	#include "Serializers.inl"

	template <class _t>
	inline void __shr_init( bsp::SharedVariableSet & set, const char * id , _t & value ) {
		set.add_var(
			id, 
			new bsp::SharedVariable< 
					_t, 
					bsp::InitAssign<_t>,  
					bsp::ReduceFirst<_t> 
			> (value, value), 
		true, false );
	}

	template <class _t, template<class> class _r >
	inline void __shr_reduce( bsp::SharedVariableSet & set, const char * id , _t & value, _t neutral, bool also_init) {
		set.add_var(
			id, 
			new bsp::SharedVariable< 
					_t, 
					bsp::InitAssign<_t>, 
					_r <_t> 
			> (value, neutral), 
		also_init, true );

		if ( ::bsp_nprocs() > 1 ) {
			bsp::Shared ** psp = new bsp::Shared * [::bsp_nprocs() + 2 ];
	
			for (int p = 0; p <= ::bsp_nprocs() + 1 ; ++p) {
				psp[p] = new bsp::SharedVariable< 
							_t, 
							bsp::InitAssign<_t>, 
							_r <_t> 
						> ( neutral );
			}

			set.init_reduce_slot( id, psp );
		}
	}
};

#define SHARE_VARIABLE_IR(set, reduce, neutral, var, ...) do { 							\
	bsp::__shr_reduce<__VA_ARGS__, reduce > (set, #var, var, neutral, true);	\
} while(0)

#define SHARE_VARIABLE_R(set, reduce, neutral, var, ...) do { 							\
	bsp::__shr_reduce<__VA_ARGS__, reduce > (set, #var, var, neutral, false);	\
} while(0)

#define SHARE_VARIABLE_I(set, var, ...) do { 								\
	bsp::__shr_init<__VA_ARGS__> (set, #var, var);											\
} while(0)


#endif // __SharedVariable_H__
