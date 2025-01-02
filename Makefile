CXX = gcc
SXX = nasm
TARGET = bin/*
CFLAGS = -lncurses -lhttp
SRCS = C/main.c

all:
	$(CXX) $(SRCS) ./C/*.c $(CFLAGS) $(TARGET)

makeBuild:
	tar dist.tar ./root/ ./bin/

