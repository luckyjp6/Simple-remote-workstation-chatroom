TOTAL = conn_func.o proc_client.o worm_server.o
CXXFLAGS=-std=c++17 -Wall -pedantic -pthread -lboost_system
CXX_INCLUDE_PARAMS=$(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_PARAMS=$(addprefix -L , $(CXX_LIB_DIRS))
for_server = conn_func.o proc_client.o worm_server.o
# for_http = http_server console button


all: server client cp #$(for_http)

server: $(for_server)
	g++ -o worm_server conn_func.o proc_client.o worm_server.o

worm_server.o: codes/worm_server.cpp
	g++ -c codes/worm_server.cpp

conn_func.o: codes/conn_func.cpp codes/conn_func.h
	g++ -c codes/conn_func.cpp

proc_client.o: codes/proc_client.cpp codes/proc_client.h
	g++ -c codes/proc_client.cpp

client: codes/worm_client.cpp	
	g++ -o worm_client codes/worm_client.cpp

cp: codes/worm_cp.cpp
	g++ -o worm_cp codes/worm_cp.cpp

# http_server: codes/http_server.cpp 
# 	g++ codes/http_server.cpp -o http_server $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)
# console: codes/console.cpp
# 	g++ codes/console.cpp -o console.cgi $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)
# button: codes/button.cpp
# 	g++ codes/button.cpp -o button

test: codes/test.cpp
	g++ -o test codes/test.cpp
	sudo ./test

.PHONY: clean
clean:
	-rm $(TOTAL) worm_server worm_client worm_cp worm_upload