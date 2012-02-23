/***************************************************************************
 *   Copyright (C) 2007   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#ifndef __PERMUTATION_H__
#define __PERMUTATION_H__

#include <iostream>
#include <algorithm>
#include <vector>
#include <iterator>
#include <map>

namespace utilities {

template <class _value_type, class _iterator_type, class _comparison_fun>
struct PermutationComparisonFun  : public std::binary_function<_iterator_type, _iterator_type, bool> {
	bool operator()(_iterator_type i1, _iterator_type i2)  {
		static _comparison_fun comp;
		return comp(*i1, *i2); 
	}
	bool operator()(_iterator_type i1, _value_type const & v2)  {
		static _comparison_fun comp;
		return comp(*i1, v2); 
	}
	bool operator()(_value_type const & v1, _iterator_type i2 )  {
		static _comparison_fun comp;
		return comp(v1, *i2); 
	}
	bool operator()(_value_type const & v1, _value_type const & v2 )  {
		static _comparison_fun comp;
		return comp(v1, v2); 
	}
};

template <class _container_type>
class Permutation {
public:
	typedef typename _container_type::value_type _value_type;
	typedef typename _container_type::iterator _container_iterator;
	typedef typename std::vector < _container_iterator > :: iterator iterator;

	Permutation(_container_type const & c) : 
    	c_begin(c.begin()), c_end(c.end()) {
		identity();
	}

	Permutation(_container_iterator _begin, _container_iterator _end) : 
		c_begin (_begin), c_end(_end) {
		identity();
	}

	_value_type & operator[](size_t i) {
		return *permutation[i];
	}

	iterator begin() {
		return permutation.begin();
	}

	iterator end() {
		return permutation.end();
	}

	/**
	 * @brief swap two elements in the permutation
	 */
	void swap (size_t i, size_t j) {
		ASSERT (i < permutation.size() && j < permutation.size());
		std::swap(permutation[i], permutation[j]);
	}

	/**
	 * @brief Create identity permutation
	 */
	void identity() {
		permutation.resize(std::distance(c_begin, c_end));
		size_t len = size();
		_container_iterator it = c_begin;
		for(size_t j = 0; j < len; ++j) {
			permutation[j] = it++;
		}
	}

	/**
	 * @brief Randomize permutation
	 */
	void randomize() { 
		size_t len = permutation.size();
		for(size_t k= 0; k < len; ++k) {
			size_t Z= (size_t)(((double) (rand()/(RAND_MAX+1.0)))*((double)len-k));
			std::swap(permutation[k], permutation[Z+k]);
		}
	}

	/**
	 * @brief Generate next permutation from current one
	 */
	void nextPermutation() {
		size_t len = permutation.size();
		size_t i = len - 1;
		while (i > 0 && permutation[i - 1] >= permutation[i]) {
			i = i - 1;
		}

		size_t j = len;

		while (		j > 0 
				&& 	i > 0 
				&& 	permutation[j - 1] <= permutation[i - 1]) {
			j = j - 1;
		}

		// swap values at positions (i-1) and (j-1)
		if(i > 0 && j > 0) {
			std::swap(permutation[i - 1], permutation[j - 1]);
		}

		i++;
		j = len;

		while (i < j) {
			std::swap(permutation[i - 1], permutation[j - 1]);
			i++;
			j--;
		}
	}

	size_t size() const {
		return permutation.size();
	}

private:
	_container_iterator c_begin, c_end;
	std::vector< _container_iterator > permutation;
};

};

namespace std {
	template <class _container_type> std::ostream & operator<<(std::ostream & o,  utilities::Permutation<_container_type> & p) {
	for(typename utilities::Permutation<_container_type>::iterator it = p.begin(); it != p.end(); ++it) {
		o << *it << " " ; // << "->" << *it <<" ";
	}
	return o;
}
};

#endif
