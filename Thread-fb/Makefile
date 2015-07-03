CFLAGS = -O2
LDFLAGS = -lpthread -lm
EXEC = Thread-inc Thread-mutex-inc Memalloc Simultest NoLock \
	WithLock WithTryLock ProdCons ProdCons-cond ProdMCons-cond

all: $(EXEC)

Fork-inc: Fork-inc.c
	gcc $(CFLAGS) -o Fork-inc Fork-inc.c
Fork-shm-inc: Fork-shm-inc.c
	gcc $(CFLAGS) -o Fork-shm-inc Fork-shm-inc.c

clean:
	rm -f $(EXEC) *.o *~
