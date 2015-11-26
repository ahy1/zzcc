



CC=cc
CFLAGS=-Wall -g
LD=cc
LDFLAGS=-Wall -g

all: zzparser zzcpp lexer.o

zzcpp: token.o strbuf.o zzcpp.o	dump.o json.o
	$(LD) -o zzcpp $(LDFLAGS) token.o strbuf.o zzcpp.o dump.o json.o

zzcpp.o: zzcpp.c token.h strbuf.h json.h
	$(CC) -o zzcpp.o $(CFLAGS) -c zzcpp.c

zzparser: parser.o token.o strbuf.o tokenclass.o stack.o zzparser.o dump.o parcom.o json.o
	$(LD) -o zzparser $(LDFLAGS) parser.o token.o strbuf.o tokenclass.o stack.o zzparser.o dump.o parcom.o json.o

zzparser.o: zzparser.c parser.h token.h strbuf.h parcom.h
	$(CC) -o zzparser.o $(CFLAGS) -c zzparser.c

parser.o: parser.c parser.h token.h tokenclass.h dump.h json.h
	$(CC) -o parser.o $(CFLAGS) -c parser.c

json.o: json.c json.h
	$(CC) -o json.o $(CFLAGS) -c json.c

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

dump.o: dump.c dump.h
	$(CC) -o dump.o $(CFLAGS) -c dump.c

parcom.o: parcom.c parcom.h
	$(CC) -o parcom.o $(CFLAGS) -c parcom.c

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

.PHONY: check

check:
	(cd tests; ./runtests.sh)

