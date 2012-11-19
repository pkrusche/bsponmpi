/***************************************************************************
*   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
*   peter@dcs.warwick.ac.uk                                               *
***************************************************************************/

#ifndef ALIGNED_ALLOCATOR_H_
#define ALIGNED_ALLOCATOR_H_

#include <new>
#include "boost/cstdint.hpp"

namespace utilities {
	template<class T, size_t _align = 32> class aligned_allocator {
	public:
		typedef T value_type;
		typedef value_type* pointer;
		typedef const value_type* const_pointer;
		typedef value_type& reference;
		typedef const value_type& const_reference;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		template<class U>
		struct rebind {
			typedef aligned_allocator<U> other;
		};

		enum { ALIGNMENT = _align,
			ALIGN_MASK = ALIGNMENT - 1
		};

	public:
		static void *aligned_malloc(size_t size) {
			char *block = new char[size + 2*ALIGNMENT + 4];

			char** aligned = (char**)(((boost::uint64_t)block + ALIGNMENT + sizeof(char*)) & (~ALIGN_MASK));
			aligned[-1] = block;

			return aligned;
		}


		static void aligned_free(void *memory) {
			delete [] ((char**)memory)[-1];
		}

		static bool is_aligned_memory(void * v) {
			return (((boost::uint64_t)v) & ALIGN_MASK) == 0;
		}

	public:
		aligned_allocator() {
		}
		aligned_allocator(const aligned_allocator&) {
		}
		template<class U>
		aligned_allocator(const aligned_allocator<U>&) {
		}
		~aligned_allocator() {
		}

		pointer address(reference x) const {
			return &x;
		}
		const_pointer address(const_reference x) const {
			return x;
		}

		pointer allocate(size_type n, const_pointer = 0) {
			void* p = aligned_malloc(n * sizeof(T));
			if (!p)
				throw std::bad_alloc();
			return static_cast<pointer> (p);
		}

		void deallocate(pointer p, size_type) {
			aligned_free(p);
		}

		size_type max_size() const {
			return static_cast<size_type> (-1) / sizeof(T);
		}

		void construct(pointer p, const value_type& x) {
			new (p) value_type(x);
		}
		void destroy(pointer p) {
			p->~value_type();
		}

	private:
		void operator=(const aligned_allocator&);
	};

	template<> class aligned_allocator<void> {
		typedef void value_type;
		typedef void* pointer;
		typedef const void* const_pointer;

		template<class U>
		struct rebind {
			typedef aligned_allocator<U> other;
		};
	};

	template<class T>
	inline bool operator==(const aligned_allocator<T>&, const aligned_allocator<T>&) {
		return true;
	}

	template<class T>
	inline bool operator!=(const aligned_allocator<T>&, const aligned_allocator<T>&) {
		return false;
	}
};

#endif /* ALIGNED_ALLOCATOR_H_ */
