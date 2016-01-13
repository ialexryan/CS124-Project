CFLAGS = -Wall -g -std=gnu11
LDFLAGS = -lreadline -lbsd

OBJS = mysh.o

all: mysh

mysh: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean: 
	rm -f *.o *~ mysh

.PHONY: all clean
