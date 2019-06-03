
# Adapt this makefile to macos
SYSNAME := $(shell uname -s)
ifeq ($(SYSNAME), Linux)
	CC=gcc
endif
ifeq ($(SYSNAME), Darwin)
	CC=gcc-9
endif

CFLAGS=-Wpedantic -Wall -Werror -Wextra -std=c89
SOURCE_FILES=shell.c parse.c

all: shell

shell: shell.c parse.c parse.h
	${CC} ${CFLAGS} ${SOURCE_FILES} -o shell

.PHONY: clean submission

clean:
	rm -f shell

submission:
	tar czvf project4.tar .git
