#ifndef _HASH_TABLE_H
#define _HASH_TABLE_H

#include "stdlib.h"
#include "allocators.h"
#include "mspace_malloc.h"

template<typename _Key>
inline size_t default_hash_function(_Key hash) {
	size_t t = (size_t)hash;
	return t ^ (t << 5);
}

template<typename _Key>
inline bool default_equals(_Key key1, _Key key2) {
	return key1 == key2;
}

template<typename _Key, typename _Val, void** msp, size_t (*_hash)(_Key)=default_hash_function, bool (*_equals)(_Key, _Key)=default_equals>
class HashTable;

template<typename T1, typename T2>
struct Pair {
	Pair(T1 t1, T2 t2) : first(t1), second(t2) {}
	T1 first;
	T2 second;
};

/**
 * @brief HashTable node
 *
 * @tparam _Key    Type name for the key
 * @tparam _Val    Type name for the values to be stored
 */
template<typename _Key, typename _Val, void** msp>
class htnode : public Pair<_Key, _Val> {
public:
	htnode(_Key key, _Val val) : Pair<_Key, _Val>(key, val), next(NULL) {}
	TEMPLATEALLOC
private:
	htnode<_Key, _Val, msp> *next;
	template<typename _K, typename _V, void** p, size_t (*_hash)(_K), bool (*_equals)(_K, _K)>
	friend class HashTable;
};

template<typename _Key, typename _Val, void** msp, size_t (*_hash)(_Key), bool (*_equals)(_Key, _Key)>
class HashTable {
	using Node = htnode<_Key, _Val, msp>;
public:
	class iterator {
	public:
		iterator(Node** table, size_t buckets, Node* node, Node* prev, size_t index) : _table(table), _buckets(buckets), _node(node), _prev(prev), _index(index) {}
		iterator(Node** table, size_t buckets) : _table(table), _buckets(buckets), _node(NULL), _prev(NULL), _index(buckets) {}
		Node& operator*() const { return *_node; }
		Node* operator->() const { return _node; }
		iterator& operator++() {
			_prev = _node;
			_node = _node->next;
			while (_node == NULL) {
				_prev = NULL;
				if (++_index < _buckets) {
					_node = _table[_index];
				} else {
					break;
				}
			}
			return *this;
		}
		bool operator==(const iterator& other) const {
			return _table == other._table && _node == other._node && _index == other._index;
		}
		bool operator!=(const iterator& other) const {
			return !this->operator==(other);
		}
	private:
		Node** _table;
		size_t _buckets;
		Node* _node;
		Node* _prev;
		size_t _index;
		friend class HashTable<_Key, _Val, msp, _hash, _equals>;
	};

	HashTable(size_t initial_buckets = 16, double factor = 0.75) :
		table((Node**)mspace_calloc(*msp, initial_buckets, sizeof(Node*))),
		_size(0),
		buckets(initial_buckets),
		max_factor(factor),
		threshold((size_t)(initial_buckets * factor)) {
			assert(table && "bad alloc");
		}

	HashTable(const HashTable& hashtable) :
		table((Node**)mspace_calloc(*msp, hashtable.buckets, sizeof(Node*))),
		_size(0),
		buckets(hashtable.buckets),
		max_factor(hashtable.max_factor),
		threshold(hashtable.threshold) {
		assert(table && "bad alloc");
		for (Pair<_Key, _Val> p: hashtable) {
			(*this)[p.first] = p.second;
		}
	}

	HashTable& operator=(const HashTable& hashtable) {
		clear();
		mspace_free(*msp, table);
		buckets = hashtable.buckets;
		max_factor = hashtable.max_factor;
		threshold = hashtable.threshold;

		table = (Node**)mspace_calloc(*msp, buckets, sizeof(Node*));
		assert(table && "bad alloc");
		for (Pair<_Key, _Val> p: hashtable) {
			this->operator[](p.first) = p.second;
		}
		return *this;
	}

	~HashTable() {
		clear();
		mspace_free(*msp, table);
	}

	_Val& operator[](const _Key& key) {
		resize();
		size_t index = _hash(key) % buckets;
		Node* node = table[index];
		Node* last = NULL;
		while (node != NULL) {
			if (_equals(node->first, key)) {
				return node->second;
			}
			last = node;
			node = node->next;
		}
		node = new Node(key, _Val());
		if (last == NULL) {
			table[index] = node;
		} else {
			last->next = node;
		}
		_size++;
		return node->second;
	}

	_Val& at(const _Key& key) {
		resize();
		size_t index = _hash(key) % buckets;
		Node* node = table[index];
		while (node != NULL) {
			if (_equals(node->first, key)) {
				return node->second;
			}
			node = node->next;
		}
		abort();
	}

	Pair<iterator, bool> emplace(const _Key& key, const _Val& val) {
		resize();
		size_t index = _hash(key) % buckets;
		Node* node = table[index];
		Node* last = NULL;
		while (node != NULL) {
			if (_equals(node->first, key)) {
				return Pair<iterator, bool>{iterator(table, buckets, node, last, index), false};
			}
			last = node;
			node = node->next;
		}
		node = new Node(key, val);
		if (last == NULL) {
			table[index] = node;
		} else {
			last->next = node;
		}
		_size++;
		return Pair<iterator, bool>{iterator(table, buckets, node, last, index), true};
	}

	Pair<iterator, bool> try_emplace(const _Key& key, const _Val& val) {
		return emplace(key, val);
	}

	Pair<iterator, bool> try_emplace(const _Key& key) {
		resize();
		size_t index = _hash(key) % buckets;
		Node* node = table[index];
		Node* last = NULL;
		while (node != NULL) {
			if (_equals(node->first, key)) {
				return Pair<iterator, bool>{iterator(table, buckets, node, last, index), false};
			}
			last = node;
			node = node->next;
		}
		node = new Node(key, _Val());
		if (last == NULL) {
			table[index] = node;
		} else {
			last->next = node;
		}
		_size++;
		return Pair<iterator, bool>{iterator(table, buckets, node, last, index), true};
	}

	iterator find(const _Key& key) const {
		size_t index = _hash(key) % buckets;
		Node* node = table[index];
		Node* last = NULL;
		while (node != NULL) {
			if (_equals(node->first, key)) {
				return iterator(table, buckets, node, last, index);
			}
			last = node;
			node = node->next;
		}
		return end();
	}

	iterator erase(iterator pos) {
		_size--;
		if (pos._prev != NULL) {
			pos._prev->next = pos._node->next;
		} else {
			table[pos._index] = pos._node->next;
		}
		return ++pos;
	}

	iterator begin() const {
		for (size_t i = 0; i < buckets; i++) {
			if (table[i] != NULL) {
				return iterator(table, buckets, table[i], NULL, i);
			}
		}
		return iterator(table, buckets);
	}

	iterator end() const {
		return iterator(table, buckets);
	}

	void clear() {
		for (size_t i = 0; i < buckets; i++) {
			Node* node = table[i];
			while (node != NULL) {
				Node* tmp = node->next;
				delete node;
				node = tmp;
			}
			table[i] = NULL;
		}
		_size = 0;
	}

	void resize() {
		if (_size < threshold)
			return;
		size_t old_capacity = buckets;
		Node** old_table = table;

		buckets <<= 1;
		threshold = (size_t)(buckets * max_factor);
		table = (Node**)mspace_calloc(*msp, buckets, sizeof(Node*));
		assert(table && "bad alloc");

		for (size_t i = 0; i < old_capacity; i++) {
			Node* node = old_table[i];
			while (node != NULL) {
				Node* tmp = node->next;
				size_t index = _hash(node->first) % buckets;
				node->next = table[index];
				table[index] = node;
				node = tmp;
			}
		}

		mspace_free(*msp, old_table);
	}

	size_t size() const {
		return _size;
	} 

	TEMPLATEALLOC
private:
	Node** table;
	size_t _size;
	size_t buckets;
	double max_factor;
	size_t threshold;
};

struct Empty {};

template<typename T, void** msp, size_t (*_hash)(T)=default_hash_function, bool (*_equals)(T, T)=default_equals>
class HashSet {
	using Base = HashTable<T, struct Empty, msp, _hash, _equals>;
	using BaseIterator = typename Base::iterator;
public:
	class iterator {
	public:
		iterator(BaseIterator base) : _base(base) {}
		T& operator*() const { return _base->first; }
		T* operator->() const { return &_base->first; }
		iterator& operator++() {
			++_base;
			return *this;
		}
		bool operator==(const iterator& other) const {
			return _base == other._base;
		}
		bool operator!=(const iterator& other) const {
			return !this->operator==(other);
		}
	private:
		BaseIterator _base;
		friend class HashSet;
	};

	HashSet(size_t initial_buckets = 16, double factor = 0.75) :
		base(initial_buckets, factor) {}
	HashSet(const HashSet& hashset) :
		base(hashset.base) {}
	HashSet& operator=(const HashSet& hashset) {
		base = hashset.base;
	}
	iterator begin() const {
		return iterator(base.begin());
	}
	iterator end() const {
		return iterator(base.end());
	}

	Pair<iterator, bool> insert(const T& item) {
		Pair<BaseIterator, bool> ret = base.emplace(item, {});
		return Pair(iterator(ret.first), ret.second);
	}

	iterator find(const T& item) {
		return iterator(base.find(item));
	}

	iterator erase(iterator iter) {
		return iterator(base.erase(iter._base));
	}

	size_t size() {
		return base.size();
	}

	void clear() {
		base.clear();
	}

	TEMPLATEALLOC;
private:
	Base base;
};

#endif
