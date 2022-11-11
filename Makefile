TOTAL = functions.o np_simple.o np_single_proc.o rwg.o npshell

np_sime:  $(TOTAL)
	g++ -o np_simple np_simple.o
	g++ -o np_single_proc np_single_proc.o rwg.o functions.o

np_simple.o: np_simple.cpp
	g++ -c np_simple.cpp

np_single_proc.o: np_single_proc.cpp
	g++ -c np_single_proc.cpp

npshell: npshell.cpp npshell.h
	g++ -o npshell npshell.cpp

functions.o: functions.cpp functions.h
	g++ -c functions.cpp

rwg.o: rwg.cpp rwg.h
	g++ -c rwg.cpp

.PHONY: clean
clean:
	-rm edit $(TARGET)