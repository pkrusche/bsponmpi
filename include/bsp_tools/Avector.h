/***************************************************************************
*   Copyright (C) 2007   Peter Krusche, The University of Warwick         *
*   peter@dcs.warwick.ac.uk                                               *
***************************************************************************/

#ifndef __AVector_H__
#define __AVector_H__

#include <string.h>
#include <algorithm>

#include "aligned_allocator.h"

namespace utilities {

	/**
	* An AVector is an implementation of a vector (resizable array)
	* with a public data pointer that is cache aligned. 
	* 
	* We don't implement the whole interface of std::vector, this 
	* is just a helper class to manage storage.
	* 
	*/
	template <class _t>
	class AVector {
	public:
		typedef aligned_allocator<_t> allocator;

		AVector(): size(0),
			data(NULL), data_is_mine(true), grow(1), offset(0) {}

		AVector(size_t _size, size_t _g = 1):
		size(_size), grow(_g), offset(0) ,
			data_is_mine(true) {
				data = allocator().allocate(size);
		}

		AVector(AVector<_t> const& v) {
			data_is_mine= true;
			size= v.size;
			grow= v.grow;
			data = allocator().allocate(size);
			offset = 0;
			memcpy(data, v.data, size*sizeof(_t));
		}

		AVector(AVector<_t> const& v, size_t _offset, size_t len) {
			ASSERT(_offset + len <= v.size);
			data_is_mine= false;
			size= len;
			grow= v.grow;
			data = v.data;
			offset = _offset;
		}

		~AVector() {
			if (data && data_is_mine) {
				allocator().deallocate(data, size);
			}
		}

		inline _t & operator[](int index) {
			ASSERT(offset + index >= 0 && offset + index < size);
			return data[offset + index];
		}

		inline const _t & operator[](int index) const {
			ASSERT(offset + index >= 0 && offset + index < size);
			return data[offset + index];
		}

		inline void resize(size_t _size, size_t _grow= 1) {
			using namespace std;
			ASSERT(_size >= 0);
			grow= _grow;
			size_t tcount= size; 
			size_t oldsize= size;

			/*
			* determine new size
			*/
			if (grow < 1)
				grow = 1;
			if (_size != 0 && grow != 1)
				_size+= grow - (_size%grow);
			if (size == _size)
				return;
			else
				size= _size;

			/*
			* reallocate and copy data.
			*
			* note that we don't enlarge the data block if it doesn't belong
			* to us, the data block must be large enough then. No checking
			* is done since we don't remember the original size.
			*/
			if(data_is_mine) {
				if (size > 0) {
					_t* newdata= allocator().allocate(size);
					tcount= min(tcount, size);
					if (data != NULL) {
						memcpy(newdata, data, tcount*sizeof(_t));
						allocator().deallocate(data, oldsize);
					}
					data= newdata;
					data_is_mine= true;
				} else {
					if ( data != NULL ) {
						allocator().deallocate(data, oldsize);
					}
					data = NULL;
				}
			}
		}

		/** Get the address of the aligned block which contains the item at index */
		inline _t * aligned_data(int index) const {
			return (_t*)((uint64_t)(data + index + offset) & (allocator::ALIGN_MASK));
		}

		/** How much is an item shifted because of alignment in units of sizeof (_t) */
		inline size_t alignment_penalty(int index) const {
			return ( ( ((index + offset)*sizeof(_t)) & allocator::ALIGN_MASK ) + sizeof(_t) - 1) / sizeof(_t);
		}

		/** Data size including the alignment penalty */
		inline size_t aligned_size() const {
			return size + alignment_penalty(0);
		}

		/** Exact location of data item at index */
		inline _t * exact_data(int index) const {
			return data + offset + index;
		}

		/** Exact number of items in this vector  */
		inline size_t exact_size() const {
			return size;
		}

		size_t offset;
		size_t size;
		size_t grow;
		_t* data;
		bool data_is_mine;
	};
};

#endif /*_AVector_H_*/
