#ifndef _LIST_H
#define _LIST_H

#include "allocators.h"
#include "mspace_malloc.h"

template<typename _Tp, void** msp>
class List;

template<typename _Tp, void** msp>
class llnode {
	using Node = llnode<_Tp, msp>;
public:
	llnode(Node *_prev, Node *_next, _Tp _val) : prev(_prev), next(_next), val(_val) {}
	TEMPLATEALLOC
private:
	Node * prev;
	Node * next;
	_Tp val;
	friend class List<_Tp, msp>;
};

template<typename _Tp, void** msp>
class List
{
	using Node = llnode<_Tp, msp>;
public:
	class iterator
	{
		public:
			iterator(Node* node=NULL) : _node(node) {}

			_Tp& operator*() const { return _node->val; }
			iterator& operator++() {
				_node =  _node->next;
				return *this;
			}
			iterator& operator++(int) { return operator++(); }
			bool operator!=(const iterator& other) const { return _node != other._node; }
			TEMPLATEALLOC
		private:
			Node* _node;
	};
	
	class reverse_iterator
	{
		public:
			reverse_iterator(Node* node=NULL) : _node(node) {}

			_Tp& operator*() const { return _node->val; }
			reverse_iterator& operator++() {
				_node = _node->prev;
				return *this;
			}
			reverse_iterator& operator++(int) { return operator++(); }
			iterator base() const { return iterator(_node->next); }
			bool operator!=(const reverse_iterator& other) const { return _node != other._node; }
			TEMPLATEALLOC
		private:
			Node* _node;
	};

	List() : head(NULL),
		tail(NULL), _size(0) {
	}

	List(const List& list) : head(NULL),
		tail(NULL), _size(0) {
		for (_Tp& item: list) {
			push_back(item);
		}
	}

	List& operator=(const List& list) {
		clear();
		for (_Tp& item: list) {
			push_back(item);
		}
		return *this;
	}

	~List() {
		clear();
	}

	void push_front(_Tp val) {
		Node * tmp = new Node(NULL, head, val);
		if (head == NULL)
			tail = tmp;
		else
			head->prev = tmp;
		head = tmp;
		_size++;
	}

	void push_back(_Tp val) {
		Node * tmp = new Node(tail, NULL, val);
		if (tail == NULL)
			head = tmp;
		else tail->next = tmp;
		tail = tmp;
		_size++;
	}

	Node* add_front(_Tp val) {
		Node * tmp = new Node(NULL, head, val);
		if (head == NULL)
			tail = tmp;
		else
			head->prev = tmp;
		head = tmp;
		_size++;
		return tmp;
	}

	Node * add_back(_Tp val) {
		Node * tmp = new Node(tail, NULL, val);
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
		Node *tmp = new Node(node, node->next, val);
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
		Node *tmp = new Node(node->prev, node, val);
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

	iterator begin() const {
		return iterator(head);
	}

	iterator end() const {
		return iterator(NULL);
	}

	reverse_iterator rbegin() const {
		return reverse_iterator(tail);
	}

	reverse_iterator rend() const {
		return reverse_iterator(NULL);
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

#endif
