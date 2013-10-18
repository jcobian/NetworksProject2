all: therm thermd

therm: therm.o
	gcc therm.o -o therm
therm.o: therm.c therm.h
	gcc -c therm.c
clean:
	rm -f core* *.o *~ therm

thermd: 
	g++ thermd.cpp -o thermd

