EXES = $(basename $(wildcard *.c))
CC = cc
CFLAGS = -Wall -pthread

all:
	$(MAKE) $(EXES)
