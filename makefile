# Compiler
CC = cc

# Compiler Flags
CFLAGS = -Wall -std=c2x -pthread -g -O2

# Source and Object Files
SRC = nmzyz.c
OBJ = nmzyz.o
EXEC = nmzyz

# Default target: Build the program
all: $(EXEC)

# Compile the object file
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

# Link the executable
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(EXEC)

# Clean compiled files
clean:
	rm -f $(OBJ) $(EXEC)
