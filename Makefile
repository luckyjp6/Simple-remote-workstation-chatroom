TARGET = npshell.o functions.o np_simple.o

np_sime:  $(TARGET)
	g++ -o np_simple $(TARGET)

np_simple.o: np_simple.cpp
	g++ -c np_simple.cpp

npshell.o: npshell.cpp npshell.h
	g++ -c npshell.cpp

functions.o: functions.cpp functions.h
	g++ -c functions.cpp

.PHONY: clean
clean:
	-rm edit $(TARGET)