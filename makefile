# Compiler and flags
CC = gcc
CFLAGS = -O3 -Wall -std=c2x
LDFLAGS = 

# Source and binary files
SRC = netmap.c
OBJ = netmap.o
TARGET = netmap

# Default target
all: $(TARGET)

# Linking the object file to create the executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

# Compiling the source file into an object file
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC)

# Clean the generated files
clean:
	rm -f $(OBJ) $(TARGET)

# Phony targets
.PHONY: all clean

