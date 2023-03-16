TOTAL = functions.o \
		simple_server.o \
		single_proc_server.o \
		rwg.o multi_proc_server.o \
		multi_proc_rwg.o \
		multi_proc_functions.o

server:  $(TOTAL)
	g++ -o simple_server simple_server.o rwg.o functions.o
	g++ -o single_proc_server single_proc_server.o rwg.o functions.o
	g++ -o multi_proc_server multi_proc_server.o multi_proc_functions.o multi_proc_rwg.o

simple_server.o: codes/simple_server.cpp
	g++ -c codes/simple_server.cpp

single_proc_server.o: codes/single_proc_server.cpp
	g++ -c codes/single_proc_server.cpp

multi_proc_server.o: codes/multi_proc_server.cpp
	g++ -c codes/multi_proc_server.cpp

functions.o: codes/functions.cpp codes/functions.h
	g++ -c codes/functions.cpp

multi_proc_functions.o: codes/multi_proc_functions.cpp codes/multi_proc_functions.h
	g++ -c codes/multi_proc_functions.cpp

rwg.o: codes/rwg.cpp codes/rwg.h
	g++ -c codes/rwg.cpp

multi_proc_rwg.o: codes/multi_proc_rwg.cpp codes/multi_proc_rwg.h
	g++ -c codes/multi_proc_rwg.cpp

.PHONY: clean
clean:
	-rm $(TOTAL)