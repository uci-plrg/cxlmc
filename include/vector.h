#ifndef _VECTOR_H
#define _VECTOR_H

#include <string.h>
#include "allocators.h"
#include "mspace_malloc.h"


#define VECTOR_DEFCAP 8

template<typename type, void** msp>
class Vector {

public:
	Vector() :
		_size(0),
		capacity(VECTOR_DEFCAP),
		array((type *)mspace_calloc(*msp, capacity, sizeof(type))) {
	}

	Vector(uint size) :
		_size(size),
		capacity(size),
		array((type *)mspace_calloc(*msp, capacity, sizeof(type))) {
	}

	Vector(uint size, type *_array)  :
		_size(size),
		capacity(size),
		array((type *)mspace_calloc(*msp, capacity, sizeof(type))) {
		for (uint i=0; i<_size; i++)
			array[i] = _array[i];
	}

	Vector(const Vector& vec) :
		_size(vec._size),
		capacity(vec.capacity),
		array((type *)mspace_calloc(*msp, vec.capacity, sizeof(type))) {
		for (uint i=0; i<_size; i++)
			array[i] = vec.array[i];
	}

	Vector& operator=(const Vector& vec) {
		mspace_free(*msp, array);
		_size = vec._size;
		capacity = vec.capacity;
		array = (type *)mspace_calloc(*msp, vec.capacity, sizeof(type));
		for (uint i=0; i<_size; i++)
			array[i] = vec.array[i];
		return *this;
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
			array = (type *)mspace_realloc(*msp, array, (psize << 1) * sizeof(type));
			capacity = psize << 1;
		}
		bzero(&array[_size], (psize - _size) * sizeof(type));
		_size = psize;
	}

	void push_back(type item) {
		if (_size >= capacity) {
			uint newcap = capacity << 1;
			array = (type *)mspace_realloc(*msp, array, newcap * sizeof(type));
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

	~Vector() {
		for (uint i=0; i<_size; i++) {
			array[i].~type();
		}
		mspace_free(*msp, array);
	}

	void clear() {
		_size = 0;
	}

	type *begin() const {
		return array;
	}

	type *end() const {
		return array + _size;
	}

	TEMPLATEALLOC
private:
	uint _size;
	uint capacity;
	type *array;
};

#endif
