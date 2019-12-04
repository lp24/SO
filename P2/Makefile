# Makefile, versao 02
# Sistemas Operativos, DEI/IST/ULisboa 2017-18

CFLAGS= -g -Wall -pedantic -std=gnu99
CC=gcc

heatSim: main.o matrix2d.o mplib3.o leQueue.o
	$(CC) $(CFLAGS) -pthread -o heatSim main.o matrix2d.o mplib3.o leQueue.o

main.o: main.c matrix2d.h mplib3.h
	$(CC) $(CFLAGS) -c main.c

matrix2d.o: matrix2d.c matrix2d.h
	$(CC) $(CFLAGS) -c matrix2d.c
	
leQueue.o: leQueue.c leQueue.h
	$(CC) $(CFLAGS) -c leQueue.c
	
mplib3.o: mplib3.c mplib3.h leQueue.h
	$(CC) $(CFLAGS) -c mplib3.c

clean:
	rm -f *.o heatSim

zip:
	zip heatSim_ex02_solucao.zip main.c matrix2d.c matrix2d.h leQueue.c leQueue.h mplib3.c mplib3.h Makefile

run:
	./heatSim 20 7.0 6.0 5.0 4.0 10 4 0
