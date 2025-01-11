#include "nodestack.h"
#include "assert.h"
#include "stdio.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

/**
 * @brief Node constructor
 *
 * Constructs a single Node for use in a NodeStack. Each Node is associated
 * with exactly one ModelAction (exception: the first Node should be created
 * as an empty stub, to represent the first thread "choice") and up to one
 * parent.
 *
 */
Node::Node(int mrf_size) :
	read_from_idx(0),
	rf_size(mrf_size)
{
}

Node::Node(int idx, int mrf_size) :
	read_from_idx(idx),
	rf_size(mrf_size)
{
}

/** @brief Node desctructor */
Node::~Node() {
}

/** Prints debugging info for the ModelAction associated with this Node */
// void Node::print() const {
// }

/****************************** read from ********************************/

/** @brief Prints info about read_from set */
// void Node::print_read_from()
// {
// }

/**
 * Gets the next 'read_from' action from this Node. Only valid for a node
 * where this->action is a 'read'.
 */
int Node::get_choice() const {
	// if(model->isRandomExecutionEnabled()) {
	// 	return random() % rf_size;
	// }
	return read_from_idx;
}

int Node::get_read_from_size() const
{
	return rf_size;
}

/**
 * Checks whether the readsfrom set for this node is empty.
 * @return true if the readsfrom set is empty.
 */
bool Node::has_more_choices() const {
	// if(model->isRandomExecutionEnabled()) {
	// 	return false;
	// }
	return (read_from_idx + 1) < rf_size;
}

/**
 * Increments the index into the readsfrom set to explore the next item.
 * @return Returns false if we have explored all items.
 */
void Node::increment_read_from() {
	// DBG();
	read_from_idx++;
}

/************************** end read from ********************************/

NodeStack::NodeStack() :
	node_list(),
	last_backtrack(NULL),
	curr_backtrack(NULL),
	save_state_path(NULL),
	head_idx(-1)
{
}

NodeStack::~NodeStack()
{
	for (unsigned int i = 0;i < node_list.size();i++)
		delete node_list[i];
}

// void NodeStack::print() const
// {
// 	model_print("............................................\n");
// 	model_print("NodeStack printing node_list:\n");
// 	for (unsigned int it = 0;it < node_list.size();it++) {
// 		if ((int)it == this->head_idx)
// 			model_print("vvv following action is the current iterator vvv\n");
// 		node_list[it]->print();
// 	}
// 	model_print("............................................\n");
// }

void NodeStack::save_state() const {
	if (save_state_path == NULL) {
		return;
	}

	int fd;
	if ((fd = open(save_state_path, O_WRONLY | O_CREAT, 00777)) == -1) {
		perror("open");
		return;
	}

	int16_t curr_ind = -1;
	for (unsigned int it = 0; it < node_list.size(); it++) {
		if (node_list[it] == curr_backtrack) {
			curr_ind = it;
			break;
		}
	}
	assert(curr_ind != -1);
	write(fd, &curr_ind, 2);
	for (unsigned int it = 0; it < node_list.size(); it++) {
		uint8_t i1 = node_list[it]->get_choice();
		uint8_t i2  = node_list[it]->get_read_from_size();
		write(fd, &i1, 1);
		write(fd, &i2, 1);
	}
	close(fd);
}

/**
 * Empties the stack of all trailing nodes after a given position and calls the
 * destructor for each. This function is provided an offset which determines
 * how many nodes (relative to the current replay state) to save before popping
 * the stack.
 * @param numAhead gives the number of Nodes (including this Node) to skip over
 * before removing nodes.
 */
void NodeStack::pop_restofstack(int numAhead)
{
	/* Diverging from previous execution; clear out remainder of list */
	unsigned int it = head_idx + numAhead;
	for (unsigned int i = it;i < node_list.size();i++)
		delete node_list[i];
	node_list.resize(it);
}

/** Reset the node stack. */
void NodeStack::full_reset()
{
	for (unsigned int i = 0;i < node_list.size();i++)
		delete node_list[i];
	node_list.clear();
	reset_execution();
}

Node * NodeStack::get_head() const
{
	if (node_list.empty() || head_idx < 0)
		return NULL;
	return node_list[head_idx];
}

Node * NodeStack::get_next() {
	if (node_list.empty()) {
		// DEBUG("Empty\n");
		return NULL;
	}
	unsigned int it = head_idx + 1;
	if (it == node_list.size()) {
		// DEBUG("At end\n");
		return NULL;
	}
	head_idx++;
	return node_list[it];
}

void NodeStack::reset_execution() {
	curr_backtrack = last_backtrack;
	last_backtrack = NULL;
	head_idx = -1;
}

Node * NodeStack::explore_next(uint numchoices) {
	Node * node = get_next();
	if (node == NULL) {
		node = create_node(numchoices);
	} else {
		assert(((int)numchoices) == node->get_read_from_size());
		if (node == curr_backtrack) {
			node->increment_read_from();
			pop_restofstack(1);	//dump other nodes...
			curr_backtrack = NULL;
		}
	}
	if (node->has_more_choices())
		last_backtrack = node;
	return node;
}

Node * NodeStack::create_node(uint numchoices) {
	Node * n = new Node(numchoices);
	node_list.push_back(n);
	head_idx++;
	return n;
}

void NodeStack::set_state(char *file) {
	int fd;
	uint16_t curr_ind;
	uint8_t buf[2];

	save_state_path = file;

	if ((fd = open(file, O_RDONLY)) == -1) {
		if (errno != ENOENT) {
			perror("open");
		}
		return;
	}

	printf("Loading nodestack state from file: %s\n", file);
	full_reset();
	int size;
	if ((size = read(fd, &curr_ind, 2)) != 2) {
		if (size == -1) {
			perror("read");
		}
		return;
	}
	while ((size = read(fd, buf, 2)) == 2) {
		node_list.push_back(new Node(buf[0], buf[1]));
	}
	if (size == -1) {
		perror("read");
	}
	curr_backtrack = node_list[curr_ind];\
	close(fd);
}