TOTAL = functions.o np_simple.o np_single_proc.o rwg.o np_multi_proc.o rwg_multi_proc.o

np_:  $(TOTAL)
	g++ -o np_simple np_simple.o rwg.o functions.o
	g++ -o np_single_proc np_single_proc.o rwg.o functions.o
	g++ -o np_multi_proc np_multi_proc.o rwg_multi_proc.o functions.o

np_simple.o: np_simple.cpp
	g++ -c np_simple.cpp

np_single_proc.o: np_single_proc.cpp
	g++ -c np_single_proc.cpp

np_multi_proc.o: np_multi_proc.cpp
	g++ -c np_multi_proc.cpp

functions.o: functions.cpp functions.h
	g++ -c functions.cpp

rwg.o: rwg.cpp rwg.h
	g++ -c rwg.cpp

rwg_multi_proc.o: rwg_multi_proc.cpp rwg_multi_proc.h
	g++ -c rwg_multi_proc.cpp

.PHONY: clean
clean:
	-rm edit $(TARGET)