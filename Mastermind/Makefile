# simple Makefile to build and test the MasterMind implementation

prg=master-mind
lib=lcdBinary
matches=mm-matches
tester=testm

CC=gcc
AS=as
OPTS=-W
# NOTE: for debugging, use -pedantic etc as below
# OPTS=-std=c99 -O2 -Wall -pedantic -g

all: $(prg) cw2 $(tester)

cw2: $(prg)
	@if [ ! -L cw2 ] ; then ln -s $(prg) cw2 ; fi

$(prg): $(prg).o $(lib).o $(matches).o
	$(CC) -o $@ $^

%.o:	%.c
	$(CC) $(OPTS) -c -o $@ $<

%.o:	%.s
	$(AS) -o $@ $<

# run the program with debug option to show secret sequence
run:
	sudo ./$(prg) -d

# do unit testing on the matching function
unit: cw2
	sh ./test.sh

# testing the C vs the Assembler version of the matching fct
test:	$(tester)
	./$(tester)

clean:
	-rm $(prg) $(tester) cw2 *.o

