include ../build/build_env.mak

CFLAGS += -I../include
ASFLAGS += -I../include 

OBJECTS = $(addsuffix .o, $(basename  $(wildcard *.c)))

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@

lib.o:  $(OBJECTS)
	$(LD) $(LDFLAGS) -r -o lib.o $(OBJECTS)

clean:
	rm -f *.o *~ *.a
