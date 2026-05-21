TARGET = test
CC = gcc
CCFLAGS = -Wall

run: build
	./${TARGET}

build: allocator.c test.c
	${CC} ${CCFLAGS} -o ${TARGET} test.c