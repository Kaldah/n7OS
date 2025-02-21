include ../build/build_env.mak

CFLAGS += -I../include
ASFLAGS += -I../include

all: crt0.o

%.o: %.c
	$(CC) $(CFLAGS)  -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@

clean:
	rm -f *.o *~ task_dump_screen.o
