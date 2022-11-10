TARGET = functions.o np_simple.o

np_sime:  $(TARGET) npshell
	g++ -o np_simple $(TARGET)

np_simple.o: np_simple.cpp
	g++ -c np_simple.cpp

npshell: npshell.cpp npshell.h
	g++ -o npshell npshell.cpp

functions.o: functions.cpp functions.h
	g++ -c functions.cpp

.PHONY: clean
clean:
	-rm edit $(TARGET)