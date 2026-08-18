TOOL_DIR := c:\p\src\llvm-project\build\bin

CC=${TOOL_DIR}/clang.exe
LD=${CC}

BUILD_DIR := build

SRC := \
	src/dump.c \
	src/expression.c \
	src/json.c \
	src/node.c \
	src/parcom.c \
	src/parser.c \
	src/stack.c \
	src/strbuf.c \
	src/token.c \
	src/tokenclass.c \
	src/gen.c \
	src/gen_asm_amd64.c \
	src/zzcc.c \
	src/zzcpp.c \
	src/zzparser.c

OBJ := ${addprefix ${BUILD_DIR}/, ${SRC:.c=.o}}
#DEPS := ${OBJ:.o=.d}

CFLAGS=-Wall -g3 -glldb -Wno-deprecated-declarations -Isrc
LDFLAGS=-Wall -g3 -glldb

.PHONY: all

all: ${BUILD_DIR}/zzparser ${BUILD_DIR}/zzcpp ${BUILD_DIR}/zzcc

${BUILD_DIR}/zzparser: ${OBJ} | ${BUILD_DIR}
	${LD} -o $@ ${LDFLAGS} $(filter-out ${BUILD_DIR}/src/zzcpp.o ${BUILD_DIR}/src/zzcc.o,$^)

${BUILD_DIR}/zzcpp: ${OBJ} | ${BUILD_DIR}
	${LD} -o $@ ${LDFLAGS} $(filter-out ${BUILD_DIR}/src/zzparser.o ${BUILD_DIR}/src/zzcc.o,$^)

${BUILD_DIR}/zzcc: ${OBJ} | ${BUILD_DIR}
	${LD} -o $@ ${LDFLAGS} $(filter-out ${BUILD_DIR}/src/zzparser.o ${BUILD_DIR}/src/zzcpp.o,$^)

${BUILD_DIR}/%.o: %.c Makefile | ${BUILD_DIR}/src
	${CC} -o $@ ${CFLAGS} -c $<

${BUILD_DIR}:
	mkdir -p ${BUILD_DIR}
	
${BUILD_DIR}/src:
	mkdir -p ${BUILD_DIR}/src
	
.PHONY: clean

clean:
	rm -rf ${BUILD_DIR}

.PHONY: test

test:
	(cd tests; ./runtests.sh)

.PHONY: testparser

testparser:
	(cd tests; ./runparsertests.sh)

.PHONY: testcpp

testcpp:
	(cd tests; ./runcpptests.sh)

.PHONY: testcc

testcc:
	(cd tests; ./runcctests.sh)
