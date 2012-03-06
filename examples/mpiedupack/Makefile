CC=cc
CFLAGS= -O3
LFLAGS= -lm -lmpi
OBJIP= mpiinprod.o mpiedupack.o
OBJBEN= mpibench.o mpiedupack.o
OBJLU= mpilu_test.o mpilu.o mpiedupack.o
OBJFFT= mpifft_test.o mpifft.o mpiedupack.o
OBJMV= mpimv_test.o mpimv.o mpiedupack.o

all: ip bench lu fft matvec

ip: $(OBJIP)
	$(CC) $(CFLAGS) -o ip $(OBJIP) $(LFLAGS)

bench: $(OBJBEN)
	$(CC) $(CFLAGS) -o bench $(OBJBEN) $(LFLAGS)

lu: $(OBJLU)
	$(CC) $(CFLAGS) -o lu $(OBJLU) $(LFLAGS)

fft: $(OBJFFT)
	$(CC) $(CFLAGS) -o fft $(OBJFFT) $(LFLAGS)

matvec: $(OBJMV)
	$(CC) $(CFLAGS) -o matvec $(OBJMV) $(LFLAGS)

mpiinprod.o: mpiinprod.c mpiedupack.h
	$(CC) $(CFLAGS) -c mpiinprod.c

mpibench.o: mpibench.c mpiedupack.h
	$(CC) $(CFLAGS) -c mpibench.c

mpilu_test.o: mpilu_test.c mpiedupack.h
	$(CC) $(CFLAGS) -c mpilu_test.c

mpilu.o: mpilu.c mpiedupack.h
	$(CC) $(CFLAGS) -c mpilu.c

mpifft_test.o: mpifft_test.c mpiedupack.h
	$(CC) $(CFLAGS) -c mpifft_test.c

mpifft.o: mpifft.c mpiedupack.h
	$(CC) $(CFLAGS) -c mpifft.c

mpimv_test.o: mpimv_test.c mpiedupack.h
	$(CC) $(CFLAGS) -c mpimv_test.c

mpimv.o: mpimv.c mpiedupack.h
	$(CC) $(CFLAGS) -c mpimv.c

mpiedupack.o: mpiedupack.c mpiedupack.h
	$(CC) $(CFLAGS) -c mpiedupack.c

clean:
	rm -f *.o ip bench lu fft matvec
