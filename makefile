CC      ?= cc
CFLAGS  ?= -O2 -g
CFLAGS  += -Wall -Wextra -std=gnu11 -pthread
LDFLAGS += -pthread

EXEC = nmzyz
SRC  = nmzyz.c

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(EXEC) $(LDFLAGS)

clean:
	rm -f $(EXEC) *.o
