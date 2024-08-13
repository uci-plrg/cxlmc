USER_PROGS = userA userB
USER_SO    = $(addsuffix .so, $(USER_PROGS))
CXX_FLAGS  = -g

init: init.cc malloc.o scheduler.o $(USER_SO) allocators.h
	g++ $(CXX_FLAGS) -o $@ $< malloc.o scheduler.o -ldl

# cannot allocate from non-shared mmap for shared memory when forked
malloc.o: malloc.c
	gcc $(CXX_FLAGS) -O3 -fPIC -c $< -o $@ -DONLY_MSPACES -DHAVE_MMAP=0 -Wno-unused-variable

scheduler.o: scheduler.cc malloc.o shared_data.h allocators.h
	g++ $(CXX_FLAGS) -fPIC -c $< -o $@ malloc.o

model.o: model.cc malloc.o shared_data.h allocators.h 
	g++ $(CXX_FLAGS) -fPIC -c $< -o $@ malloc.o

$(USER_SO): %.so: %.cc malloc.o model.o allocators.h
	g++ $(CXX_FLAGS) -fPIC -shared -o $@ $< malloc.o model.o scheduler.o

run: init
	./init 16 $(addprefix ./, $(USER_SO))

clean:
	rm -f init ${USER_SO} malloc.o model.o scheduler.o
