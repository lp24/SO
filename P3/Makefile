# Makefile, versao 02
# Sistemas Operativos, DEI/IST/ULisboa 2017-18

CFLAGS= -g -Wall -pedantic -std=gnu99
CC=gcc

heatSim: main.o matrix2d.o
	$(CC) $(CFLAGS) -pthread -o heatSim main.o matrix2d.o 

main.o: main.c matrix2d.h
	$(CC) $(CFLAGS) -c main.c

matrix2d.o: matrix2d.c matrix2d.h
	$(CC) $(CFLAGS) -c matrix2d.c
	
clean:
	rm -f *.o heatSim

zip:
	zip heatSim_ex03_solucao.zip main.c matrix2d.c matrix2d.h Makefile

run:
	./heatSim 4 7.0 6.0 5.0 4.0 2 2 0.5
