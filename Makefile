USER_PROGS = userA userB
USER_SO    = $(addsuffix .so, $(USER_PROGS))
CXX_FLAGS  = -g

init: init.cc malloc.o $(USER_SO) shared_mem.h
	g++ $(CXX_FLAGS) -o $@ $< malloc.o -ldl

# cannot allocate from non-shared mmap for shared memory when forked
malloc.o: malloc.c
	g++ $(CXX_FLAGS) -O3 -fPIC -c $< -o $@ -DONLY_MSPACES -DHAVE_MMAP=0 -Wno-unused-variable

$(USER_SO): %.so: %.cc malloc.o model.h shared_mem.h
	g++ $(CXX_FLAGS) -fPIC -shared -o $@ $< malloc.o

run: init
	./init 16 $(addprefix ./, $(USER_SO))

clean:
	rm -f init ${USER_SO} malloc.o
