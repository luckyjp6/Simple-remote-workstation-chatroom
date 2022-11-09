TARGET = server.o npshell.o functions.o

npshell: $(TARGET)
	g++ -o server $(TARGET)

server.o: server.cpp
	g++ -c server.cpp

npshell.o: npshell.cpp npshell.h
	g++ -c npshell.cpp

functions.o: functions.cpp functions.h
	g++ -c functions.cpp

.PHONY: clean
clean:
	-rm edit npshell.o