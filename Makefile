EXE=udblk
CC=gcc
CFLAGS=-I. -Werror
LDFLAGS= -ludev
OBJ = udblk_main.o udblk_udev.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

udblk: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(OBJ)
	rm -f $(EXE)
