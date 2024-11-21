#ifndef _DATA_STRUCTURES_H
#define _DATA_STRUCTURES_H

#include <strings.h>

#define TEMPLATEALLOC \
	void * operator new(size_t size) { \
				return _malloc(size); \
			} \
	void operator delete(void *p, size_t size) { \
				_free(p); \
			} \
	void * operator new[](size_t size) { \
				return _malloc(size); \
			} \
	void operator delete[](void *p, size_t size) { \
				_free(p); \
			} \
	void * operator new(size_t size, void *p) {	/* placement new */ \
				return p; \
			}

template<typename _Tp, void * (*_malloc)(size_t), void * (*_realloc)(void*, size_t), void (*_free)(void*)>
class List;

template<typename _Tp, void * (*_malloc)(size_t), void * (*_realloc)(void*, size_t), void (*_free)(void*)>
class llnode {
	using Node = llnode<_Tp, _malloc, _realloc, _free>;
public:
	TEMPLATEALLOC
	~llnode() {
		printf("DELETE %p\n", (void*)this);
	}
private:
	Node * next;
	Node * prev;
	_Tp val;
	friend class List<_Tp, _malloc, _realloc, _free>;
};

template<typename _Tp, void * (*_malloc)(size_t), void * (*_realloc)(void*, size_t), void (*_free)(void*)>
class List
{
	using Node = llnode<_Tp, _malloc, _realloc, _free>;
public:
	class iterator
	{
		public:
			iterator(Node* node=NULL, bool reverse=false) : _node(node), _reverse(reverse) {}

			_Tp& operator*() const { return _node->val; }
			iterator& operator++() {
				_node = _reverse ? _node->prev : _node->next;
				return *this;
			}
			iterator& operator++(int) { return operator++(); }
			bool operator!=(const iterator& other) const { return _node != other._node || _reverse != other._reverse; }
			TEMPLATEALLOC
		private:
			Node* _node;
			bool _reverse;
	};
	
	List() : head(NULL),
		tail(NULL), _size(0) {
	}

	List(const List& list) = delete;

	~List() {
		clear();
	}

	void push_front(_Tp val) {
		Node * tmp = new Node();
		tmp->prev = NULL;
		tmp->next = head;
		tmp->val = val;
		if (head == NULL)
			tail = tmp;
		else
			head->prev = tmp;
		head = tmp;
		_size++;
	}

	void push_back(_Tp val) {
		Node * tmp = new Node();
		tmp->prev = tail;
		tmp->next = NULL;
		tmp->val = val;
		if (tail == NULL)
			head = tmp;
		else tail->next = tmp;
		tail = tmp;
		_size++;
	}

	Node* add_front(_Tp val) {
		Node * tmp = new Node();
		tmp->prev = NULL;
		tmp->next = head;
		tmp->val = val;
		if (head == NULL)
			tail = tmp;
		else
			head->prev = tmp;
		head = tmp;
		_size++;
		return tmp;
	}

	Node * add_back(_Tp val) {
		Node * tmp = new Node();
		tmp->prev = tail;
		tmp->next = NULL;
		tmp->val = val;
		if (tail == NULL)
			head = tmp;
		else tail->next = tmp;
		tail = tmp;
		_size++;
		return tmp;
	}

	_Tp pop_front() {
		Node *tmp = head;
		head = head->next;
		if (head == NULL)
			tail = NULL;
		else
			head->prev = NULL;
		_Tp tmpval = tmp->val;
		delete tmp;
		_size--;
		return tmpval;
	}

	void pop_back() {
		Node *tmp = tail;
		tail = tail->prev;
		if (tail == NULL)
			head = NULL;
		else
			tail->next = NULL;
		delete tmp;
		_size--;
	}

	void clear() {
		while(head != NULL) {
			Node *tmp=head->next;
			delete head;
			head = tmp;
		}
		tail=NULL;
		_size=0;
	}

	Node * insertAfter(Node * node, _Tp val) {
		Node *tmp = new Node();
		tmp->val = val;
		tmp->prev = node;
		tmp->next = node->next;
		node->next = tmp;
		if (tmp->next == NULL) {
			tail = tmp;
		} else {
			tmp->next->prev = tmp;
		}
		_size++;
		return tmp;
	}

	void insertBefore(Node * node, _Tp val) {
		Node *tmp = new Node();
		tmp->val = val;
		tmp->next = node;
		tmp->prev = node->prev;
		node->prev = tmp;
		if (tmp->prev == NULL) {
			head = tmp;
		} else {
			tmp->prev->next = tmp;
		}
		_size++;
	}

	Node * erase(Node * node) {
		if (head == node) {
			head = node->next;
		} else {
			node->prev->next = node->next;
		}

		if (tail == node) {
			tail = node->prev;
		} else {
			node->next->prev = node->prev;
		}

		Node *next = node->next;
		delete node;
		_size--;
		return next;
	}

	iterator begin() {
		return iterator(head);
	}

	iterator end() {
		return iterator(NULL);
	}

	iterator rbegin() {
		return iterator(head, true);
	}

	iterator rend() {
		return iterator(NULL, true);
	}

	_Tp front() {
		return head->val;
	}

	_Tp back() {
		return tail->val;
	}
	uint size() {
		return _size;
	}
	bool empty() {
		return _size == 0;
	}

	TEMPLATEALLOC
private:
	Node *head;
	Node *tail;
	uint _size;
};

#define VECTOR_DEFCAP 8

template<typename type, void * (*_malloc)(size_t), void * (*_realloc)(void*, size_t), void (*_free)(void*)>
class Vector {
public:
	Vector(uint _capacity = VECTOR_DEFCAP) :
		_size(0),
		capacity(_capacity),
		array((type *)_malloc(_capacity * sizeof(type))) {
	}

	Vector(uint _capacity, type *_array)  :
		_size(_capacity),
		capacity(_capacity),
		array((type *)_malloc(_capacity * sizeof(type))) {
		memcpy(array, _array, capacity * sizeof(type));
	}

	Vector(const Vector& vec) = delete;

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
			array = (type *)_realloc(array, (psize << 1) * sizeof(type));
			capacity = psize << 1;
		}
		bzero(&array[_size], (psize - _size) * sizeof(type));
		_size = psize;
	}

	void push_back(type item) {
		if (_size >= capacity) {
			uint newcap = capacity << 1;
			array = (type *)_realloc(array, newcap * sizeof(type));
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
		_free(array);
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

	TEMPLATEALLOC
private:
	uint _size;
	uint capacity;
	type *array;
};

#endif
