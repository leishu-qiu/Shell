CFLAGS = -g3 -Wall -Wextra -Wconversion -Wcast-qual -Wcast-align -g
CFLAGS += -Winline -Wfloat-equal -Wnested-externs
CFLAGS += -pedantic -std=gnu99 -Werror
CFLAGS += -D_GNU_SOURCE

PROMPT = -DPROMPT

.PHONY: all clean test

all: 33sh 33noprompt

33sh: sh.c jobs.c
	#TODO: compile your program, including the -DPROMPT macro
	gcc $(CFLAGS) $(PROMPT) sh.c jobs.c jobs.h -o 33sh

33noprompt: sh.c jobs.c
	#TODO: compile your program without the prompt macro
	gcc $(CFLAGS) sh.c jobs.c jobs.h -o 33noprompt

clean:
	#TODO: clean up any executable files that this Makefile has produced
	rm -f 33sh 33noprompt

test:
	./cs0330_shell_2_test -s 33noprompt
