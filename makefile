INCLUDE = -I./include/

COMPILERFLAGS = -Wall
CC = g++
CFLAGS = $(COMPILERFLAGS) $(INCLUDE)

All: clean OddBackup

OddBackup:
	$(CC) $(CFLAGS) OddBackup.c -o oddbackup
clean:
	rm -f oddbackup

l launch: All
	rm -rf destination && ./oddbackup source destination -V
