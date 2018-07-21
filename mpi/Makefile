CC = mpicc
CC_FLAGS = -lm -m64 -O3 -Wall -Wextra

EXEC = prog
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)

$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXEC) $(CC_FLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CC_FLAGS)

clean:
	rm -f $(EXEC) $(OBJECTS)
