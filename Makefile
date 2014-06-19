



CC=cc
CFLAGS=-Wall
LD=cc
LDFLAGS=-Wall

all: zzparser zzcpp lexer.o

zzcpp: token.o strbuf.o zzcpp.o

zzcpp.o: zzcpp.c token.h strbuf.h
	$(CC) -o zzcpp.o $(CFLAGS) -c zzcpp.c

zzparser: parser.o token.o strbuf.o tokenclass.o stack.o zzparser.o
	$(LD) -o zzparser $(LDFLAGS) parser.o token.o strbuf.o tokenclass.o stack.o zzparser.o

zzparser.o: zzparser.c parser.h token.h strbuf.h
	$(CC) -o zzparser.o $(CFLAGS) -c zzparser.c

parser.o: parser.c parser.h token.h tokenclass.h
	$(CC) -o parser.o $(CFLAGS) -c parser.c

token.o: token.c token.h strbuf.h
	$(CC) -o token.o $(CFLAGS) -c token.c

strbuf.o: strbuf.c strbuf.h
	$(CC) -o strbuf.o $(CFLAGS) -c strbuf.c

tokenclass.o: tokenclass.c tokenclass.h token.h
	$(CC) -o tokenclass.o $(CFLAGS) -c tokenclass.c

lexer.o: lexer.c lexer.h
	$(CC) -o lexer.o $(CFLAGS) -c lexer.c


stack.o: stack.c stack.h
	$(CC) -o stack.o $(CFLAGS) -c stack.c

.PHONY: clean

clean:
	rm *.o

.PHONY: clang

clang:
	clang -Weverything -Wno-format-nonliteral -Wno-deprecated-declarations -Wno-unused-parameter -Wno-missing-noreturn -Wno-unused-value -c *.c
	rm *.o

.PHONY: lint

lint:
	splint -redef -nestcomment -nullret -mustfreeonly -temptrans -predboolint -mustfreefresh -compdestroy -boolops -nullderef -nullstate -unqualifiedtrans -bufferoverflowhigh -compdef -usereleased -branchstate -mayaliasunique -dependenttrans *.c

