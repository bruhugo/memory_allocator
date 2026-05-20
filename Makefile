TARGET = test
CC = gcc
CCFLAGS = -Wall

run: build
	./${TARGET}

build: 
	${CC} ${CCFLAGS} -o ${TARGET} test.c