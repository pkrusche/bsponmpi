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


/** @file unittest.h

Some unit testing helpers

@author Peter Krusche
*/
#ifndef __unittest_H__
#define __unittest_H__

#include <stdexcept>
#include <iostream>

inline void check_err_impl(const char * f, int l, const char * func ) {
	using namespace std;
	ostringstream s;
	
	s << "CHECK failed at "<< f << ":"	
		<< l << " in " << func
		<< std::endl;

	throw std::runtime_error(s.str());
}

#define CHECK(cond) do {					\
	if( !(cond) ) {							\
		check_err_impl(__FILE__, __LINE__, __func__);\
	}										\
} while (0);

template <class _t>
inline void checkequal_impl(const char * f, int l, const char * func, const _t & _exp, const _t & _res ) {
	using namespace std;

	if (_exp != _res) {
		ostringstream s;
		s << "CHECK failed at "<< f << ":"	
			<< l << " in " << func 
			<< ". Expected " << _exp
			<< " and got " << _res		
			<< std::endl;
		throw std::runtime_error(s.str());

	}
}


#define CHECK_EQUAL(_exp, _res) do {	\
	checkequal_impl(__FILE__, __LINE__, __func__, _exp, _res);	\
} while (0);


#endif // __unittest_H__

