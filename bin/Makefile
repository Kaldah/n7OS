include ../build/build_env.mak

CFLAGS += -I../include
ASFLAGS += -I../include

OBJECTS = $(addsuffix .o, $(basename  $(wildcard *.c)))

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) -c $< -o $@

app.o:  $(OBJECTS)
	$(LD) $(LDFLAGS) -r -o app.o $(OBJECTS)

clean:
	rm -f *.o *~ *.a
