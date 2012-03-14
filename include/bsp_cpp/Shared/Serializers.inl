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


/** @file Serializers.inl

@author Peter Krusche
*/

#ifndef __BSPVARSerializers_H__
#define __BSPVARSerializers_H__

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

#endif // __BSPVARSerializers_H__
