all: thermd

thermd: thermd.o
	g++ -pthread thermd.o -o thermd
thermd.o: thermd.cpp therm.h
	g++ -pthread -c thermd.cpp -Wall

clean:
	rm -f core* *.o *~ thermd
