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
#include <boost/mpl/if.hpp>
#include <boost/type_traits.hpp>

#include <tbb/parallel_reduce.h>
#include <tbb/parallel_for.h>

#include "Shared.h"

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
	template <class _var >
	struct Initializer {
		Initializer ( const _var * _input, 
			std::vector<Shared*> const & _vars ) : input(_input), vars(_vars) {} 

		void operator()( const tbb::blocked_range<size_t>& r ) const { 
			for( size_t i = r.begin(); i != r.end(); ++i ) {
				(typename _var::value_type & ) (* ((_var*)vars[i])) = 
					 (typename _var::value_type const & )(*input);
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
		Reducer ( std::vector<Shared*> & _vars ) : 
			vars(_vars) {
			static _op rr;
			rr.make_neutral(result);
		}

		Reducer ( Reducer & r, tbb::split ) : vars(r.vars)
		{
			static _op rr;
			rr.make_neutral(result);
		}

		void join( const Reducer & y ) { 
			static _op red;
			red (result, y.result);
		} 

		void operator()( const tbb::blocked_range<size_t>& r ) { 
			static _op red;

			for( size_t i = r.begin(); i < r.end(); ++i ) {
				red ( result,  (typename _var::value_type const & ) (* ((_var*)vars[i])) );
			}
		}

		std::vector< Shared* > & vars;
		typename _var::value_type result;
	};
	
	/** Serialization is implemented by this class and its specializations. 
	 *
	 * @see Serializers.inl
	 */
	
	template <class _t>
	struct scalar_serializer {		
		void serialize(_t * valadr, void * target, size_t nbytes) {
			BOOST_STATIC_ASSERT(boost::is_scalar<_t>::value);
			memcpy(target, valadr, sizeof(_t));
		}

		/** default byte deserialization */
		void deserialize(_t * valadr, void * source, size_t nbytes) {
			BOOST_STATIC_ASSERT(boost::is_scalar<_t>::value);
			memcpy(valadr, source, sizeof(_t));
		}

		/** default size */
		size_t serialized_size(_t * valadr ) {
			BOOST_STATIC_ASSERT(boost::is_scalar<_t>::value);
			return sizeof(_t);
		}
	};

	template <class _t>
	struct byte_serializer {		
		void serialize(_t * valadr, void * target, size_t nbytes) {
			valadr->serialize(target, nbytes);
		}

		/** default byte deserialization */
		void deserialize(_t * valadr, void * source, size_t nbytes) {
			valadr->deserialize(source, nbytes);
		}

		/** default size */
		size_t serialized_size(_t * valadr ) {
			return valadr->serialized_size();
		}
	};

	/** this is a class template which allows us to 
	 *  add serialisation to existing types which are 
	 *  not of class ByteSerializable. You can specialize
	 *  bsp::SharedSerializable< your type > :: ... 
	 *  to do this.
	 *  Just below, we show an example for string objects.
	 */ 
	template <class _t> 
	class SharedSerializable : public Shared {
	public:
		typedef _t value_type;

		typedef typename boost::mpl::if_
			< boost::is_scalar< _t >, 
			  scalar_serializer<_t>, 
			  byte_serializer<_t> > :: type 
			  impl_t;

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
		 * alternatively, have your objects implement the ByteSerializable interface
		 * 
		 * @see Serializers.inl
		 * 
		 */
		void serialize(void * target, size_t nbytes) {
		 	impl.serialize(valadr, target, nbytes);
		}

		void deserialize(void * source, size_t nbytes) {
		 	impl.deserialize(valadr, source, nbytes);
		}

		size_t serialized_size() {
			return impl.serialized_size(valadr);
		}

	protected:
		_t * valadr;

		impl_t impl;
	};

	/** Node-local shared variable implementation */
	template <class _t, typename _red>
	class SharedVariable : public SharedSerializable<_t> {
	public:
		typedef SharedVariable <_t, _red> my_type;
		SharedVariable() : deleteme(true) {
			this->valadr = new _t ;
		}

		SharedVariable(_t & mine) : 
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
			Initializer< my_type > i (this, this->vars);
			tbb::parallel_for( tbb::blocked_range<size_t> (0, this->vars.size()), i );
			return true;
		}
		
		/** Run reducer in tbb parallel reduce */
		bool reduce() {
			if (this->vars.size() == 0) {
				return false;
			}
			static _red rr;
			Reducer<my_type, _red> r ( this->vars );
			tbb::parallel_reduce( tbb::blocked_range<size_t> (0, this->vars.size()), r );
			rr.make_neutral(*(this->valadr));
			rr (*(this->valadr), r.result);
			return true;
		}

		/** convert to _t const & */
		operator _t const & () const {
			return *(this->valadr);
		}

		/** convert to _t const & */
		operator _t & () {
			return *(this->valadr);
		}

		/** overwrite variable with neutral reduction element */
		void make_neutral() {
			static _red rr;
			rr.make_neutral(*(this->valadr));
		}

		/** assign value to other variable */
		void assign(Shared * other) {
			SharedVariable * o = static_cast<SharedVariable*>(other);
			*(o->valadr) = *(this->valadr);
		}

	private:
		bool deleteme;
	};

/************************************************************************/
/* std::string serializer                                               */
/************************************************************************/

	template <> 
	inline void SharedSerializable<std::string>::serialize (void * target, size_t nbytes) {
		ASSERT (nbytes >= sizeof(size_t) + valadr->size());
		*((size_t *) target) = valadr->size();
		memcpy ( ((size_t *) target) + 1, valadr->c_str(), valadr->size() );
	}

	template <> 
	inline void SharedSerializable<std::string>::deserialize(void * source, size_t nbytes) {
		ASSERT (nbytes >= sizeof(size_t) );
		size_t len = *((size_t*)source);
		ASSERT (nbytes >= sizeof(size_t) + len );
		*valadr = std::string ( (char*) ( ((size_t*)source) + 1), len);
	}

	template <> 
	inline size_t SharedSerializable<std::string>::serialized_size() {
		return sizeof(size_t) + valadr->size();
	}
};

#endif // __SharedVariable_H__
