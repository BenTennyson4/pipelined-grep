CC = g++
CFLAGS = -pthread -std=c++11

all: pipegrep

pipegrep: pipegrep.cpp
 $(CC) $(CFLAGS) -o pipegrep pipegrep.cpp

clean:
 rm -f pipegrep
