CC = gcc
CFLAGS =
LIBS = -lm -lportaudio
DEPS = GBA.h
OBJ = main.o GBA.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

AdvancePlay: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	@rm -f $(OBJ)
	@rm -f AdvancePlay
