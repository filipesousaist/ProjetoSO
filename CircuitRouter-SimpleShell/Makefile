# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2018-19

SOURCES = CircuitRouter-SimpleShell.c ../lib/commandlinereader.c
OBJS = $(SOURCES:%.c=%.o)
CC   = gcc
CFLAGS =-Wall -std=gnu99 -I../
LDFLAGS=
TARGET = CircuitRouter-SimpleShell

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(TARGET) $(LDFLAGS) 

CircuitRouter-SimpleShell.o: CircuitRouter-SimpleShell.c ../lib/commandlinereader.h ../lib/types.h
../lib/commandlinereader.o: ../lib/commandlinereader.c ../lib/commandlinereader.h

$(OBJS):
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo Cleaning...
	rm -f $(OBJS) $(TARGET)