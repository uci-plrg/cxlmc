USER_PROGS = userA userB
USER_SO    = $(addsuffix .so, $(USER_PROGS))
CXX_FLAGS  = -g

init: init.cc malloc.o scheduler.o user.o model.o $(USER_SO) allocators.h
	g++ $(CXX_FLAGS) -o $@ $< malloc.o scheduler.o model.o user.o -ldl

# cannot allocate from non-shared mmap for shared memory when forked
malloc.o: malloc.c
	gcc $(CXX_FLAGS) -O3 -fPIC -c $< -o $@ -DONLY_MSPACES -DHAVE_MMAP=0 -Wno-unused-variable

scheduler.o: scheduler.cc shared_data.h allocators.h scheduler.h
	g++ $(CXX_FLAGS) -fPIC -c $< -o $@

model.o: model.cc shared_data.h allocators.h model.h
	g++ $(CXX_FLAGS) -fPIC -c $< -o $@

user.o: user.cc shared_data.h allocators.h user.h
	g++ $(CXX_FLAGS) -fPIC -c $< -o $@

$(USER_SO): %.so: %.cc malloc.o model.o user.o allocators.h
	g++ $(CXX_FLAGS) -fPIC -shared -o $@ $< malloc.o model.o scheduler.o user.o

run: init
	./init 16 $(addprefix ./, $(USER_SO))

clean:
	rm -f init ${USER_SO} malloc.o model.o scheduler.o user.o
