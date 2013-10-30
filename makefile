all: therm thermd

therm: therm.o
	gcc therm.o -o therm
therm.o: therm.c therm.h
	gcc -c therm.c

thermd: thermd.o
	g++ thermd.o -o thermd
thermd.o: thermd.cpp therm.h
	g++ -pthread -c thermd.cpp -Wall

clean:
	rm -f core* *.o *~ therm thermd
