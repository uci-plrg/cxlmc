#ifndef _DATA_STRUCTURES_H
#define _DATA_STRUCTURES_H

#include <strings.h>
#include "allocators.h"

#define VECTOR_DEFCAP 8

template<typename type>
class SharedVector {
public:
	SharedVector(uint _capacity = VECTOR_DEFCAP) :
		_size(0),
		capacity(_capacity),
		array((type *)mspace_malloc(shared_space, _capacity * sizeof(type))) {
	}

	SharedVector(uint _capacity, type *_array)  :
		_size(_capacity),
		capacity(_capacity),
		array((type *)mspace_malloc(shared_space, _capacity * sizeof(type))) {
		memcpy(array, _array, capacity * sizeof(type));
	}
	void pop_back() {
		_size--;
	}

	type back() const {
		return array[_size - 1];
	}

	void resize(uint psize) {
		if (psize <= _size) {
			_size = psize;
			return;
		} else if (psize > capacity) {
			array = (type *)mspace_realloc(shared_space, array, (psize << 1) * sizeof(type));
			capacity = psize << 1;
		}
		bzero(&array[_size], (psize - _size) * sizeof(type));
		_size = psize;
	}

	void push_back(type item) {
		if (_size >= capacity) {
			uint newcap = capacity << 1;
			array = (type *)mspace_realloc(shared_space, array, newcap * sizeof(type));
			capacity = newcap;
		}
		array[_size++] = item;
	}

	type operator[](int index) const {
		return array[index];
	}

	type & operator[](int index) {
		return array[index];
	}

	bool empty() const {
		return _size == 0;
	}

	type & at(uint index) const {
		return array[index];
	}

	void setExpand(uint index, type item) {
		if (index >= _size)
			resize(index + 1);
		set(index, item);
	}

	void set(uint index, type item) {
		array[index] = item;
	}

	void insertAt(uint index, type item) {
		resize(_size + 1);
		for (uint i = _size - 1;i > index;i--) {
			set(i, at(i - 1));
		}
		array[index] = item;
	}

	void removeAt(uint index) {
		for (uint i = index;(i + 1) < _size;i++) {
			set(i, at(i + 1));
		}
		resize(_size - 1);
	}

	inline uint size() const {
		return _size;
	}

	~SharedVector() {
		mspace_free(shared_space, array);
	}

	void clear() {
		_size = 0;
	}

	type *begin() {
		return array;
	}

	type *end() {
		return array + _size;
	}

	MODELALLOC;
private:
	uint _size;
	uint capacity;
	type *array;
};

#endif
