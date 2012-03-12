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


/** @file Shared.h

Helper class to allow value sharing between contexts.

@author Peter Krusche
*/

#ifndef __BSP_SHARED_H__
#define __BSP_SHARED_H__ 

#include <iostream>
#include <stdexcept>

#include <boost/shared_ptr.hpp>

namespace bsp {
	
	/** this class is used to declare variables shared between 
	    parent and child contexts */
	class SharedVar {
	public: 
		virtual void inherit_shared_var (const SharedVar * s)  = 0;
	};
	
	/** This class is used for data sharing between parent and child contexts. 
	 *  Declaring member variables as bsp::Shared <_t> var will allow both parent
	 *  and child contexts to access the same value.
	 */
	template <
		class _t 			///< we wrap an object of type _t
	>
	class Shared : public SharedVar {
	public:
		Shared () : mine (true), init(false) { }
		Shared (_t const & t) : mine (true), init(true), data(new _t(t)) {}
		Shared (Shared const & t) : mine (false), init(t.init), data(t.data) {}
		
		/** operator= assigns and makes value 'mine'. */
		Shared<_t> & operator=(const _t & t) {
#ifdef _DEBUG
			if (!mine) {
				std::cerr << "WARNING: Write access to shared variable in child context." << std::endl;
			}
#endif
			mine = true;
			init = true;
			data = boost::shared_ptr<_t>(new _t(t));
			return *this;
		}

		/** initialisation from Shared s makes this element dependent on s */
		Shared<_t> & operator=(const Shared<_t> & s) {
			inherit_shared_var (&s);
			return *this;
		}
		
		void inherit_shared_var (const SharedVar * _s) {
			Shared<_t> const * s(dynamic_cast< Shared<_t> const * > (_s) );
			ASSERT (s->init);
			mine = false;
			data = s->data;
			init = s->init;
		}
		
		/* convert to const _t & */
		operator _t () const {
			ASSERT(init);
			return *data;
		}

		/* convert to _t & */
		operator _t () {
			ASSERT(init && mine);
			return *data;			
		}
				
	private:
		boost::shared_ptr <_t> data;
		bool mine;
		bool init;
	};
	
	/** this class implements sharing of an equal number of variables. */
	class SharedVariables {
	public:
		void operator() (SharedVar * v) {
			svl.push_back(v);
		}

		void update(SharedVariables & vs) {
			ASSERT (vs.svl.size() == svl.size());
			
			for (std::list<SharedVar*>::iterator it = svl.begin(), it2 = vs.svl.begin(); 
					it != svl.end() && it2 != vs.svl.end(); ++it, ++it2) {
				(*it)->inherit_shared_var( *it2 );
			}
		}
	private:
		std::list<SharedVar*> svl;
	};

};

#endif