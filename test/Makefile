#
# A bunch of simple test programs for LINE to play with
#
EXE=hello rawhello argtest rawfork fork multifork

all: $(EXE) 

hello : hello.c
	gcc -o $@ -static $<

rawhello: rawhello.o
	ld -e _main_ -o $@ $<

argtest: argtest.c
	gcc -o $@ -static $<

rawfork: rawfork.o
	ld -e _main_ -o $@ $<
	
fork: fork.c
	gcc -o $@ -static $<

multifork: multifork.c
	gcc -o $@ -static $<

clean:
	rm -f $(EXE)
