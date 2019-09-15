CC = gcc
CFLAGS = -Wall -pedantic -std=gnu99
DEBUG = -g
TARGETS = 2310hub 2310alice

# Mark the default target to run (otherwise make will select the first target in the file)
.DEFAULT: all
## Mark targets as not generating output files (ensure the targets will always run)
.PHONY: all debug clean

all: $(TARGETS)

## A debug target to update flags before cleaning and compiling all targets
debug: CFLAGS += $(DEBUG)
debug: clean $(TARGETS)

## Create a shared object for inclusion in our programs

#shared.o: shared.c shared.h
# 	$(CC) $(CFLAGS) -c shared.c -o shared.o
#2310hub: hub.o
#	$(CC) $(CFLAGS) hub.o -o 2310hub

2310hub: 2310hub.c shared.o
	$(CC) $(CFLAGS) 2310hub.c shared.o -lm -o 2310hub

2310alice: 2310alice.c shared.o
	$(CC) $(CFLAGS) 2310alice.c shared.o -o 2310alice

shared.o: shared.h
	$(CC) $(CFLAGS) -c shared.h

#follow below for linking
#client: client.c shared.o
# 	$(CC) $(CFLAGS) shared.o client.c -o client

# Clean up our directory - remove objects and binaries
#clean:
# 	rm -f $(TARGETS) *.o
