cpmRun: diskSimulator.o  cpmfsys.o fsysdriver.o 
	gcc -o cpmRun diskSimulator.o cpmfsys.o fsysdriver.o -g

diskSimulator.o: diskSimulator.c diskSimulator.h
	gcc -c diskSimulator.c -g

cpmfsys.o: cpmfsys.h cpmfsys.c 
	gcc -c cpmfsys.c -g

fsysdriver.o: fsysdriver.c
	gcc -c fsysdriver.c -g 

all: 
	cpmRun

clean: 
	rm *.o 

