all: oss worker

oss: oss.o
	gcc oss.o -o oss

oss.o: oss.c
	gcc -c oss.c

worker: worker.o
	gcc worker.o -o worker

worker.o: worker.c
	gcc -c worker.c

clean:
	/bin/rm -f *.o *~ oss worker
