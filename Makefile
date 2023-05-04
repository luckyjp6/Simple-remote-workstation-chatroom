TOTAL = conn_func.o proc_client.o main_server.o


all: $(TOTAL)
	g++ -o server conn_func.o proc_client.o main_server.o

main_server.o: ./codes/main_server.cpp
	g++ -c codes/main_server.cpp

conn_func.o: codes/conn_func.cpp codes/conn_func.h
	g++ -c codes/conn_func.cpp

proc_client.o: codes/proc_client.cpp codes/proc_client.h
	g++ -c codes/proc_client.cpp

.PHONY: clean
clean:
	-rm $(TOTAL) server