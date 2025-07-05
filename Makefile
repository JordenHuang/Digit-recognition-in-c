CC = gcc
CFLAGS = -O2 -Wall -Wextra
CLIBS = -lm
CPATHS = -I. -Itools -Ithirdparty

all: server

server:
	$(CC) server.c -o server.out $(CLIBS) $(CPATHS) -lpthread

train:
	$(CC) train.c -o train.out $(CFLAGS) $(CLIBS) $(CPATHS)

# predict: predict.c
# 	$(CC) predict.c -o predict.out $(CFLAGS) $(CLIBS)

squash: tools/mnist_squash.c
	$(CC) tools/mnist_squash.c -o tools/mnist_squash.out $(CFLAGS) $(CLIBS) $(CPATHS)

.PHONY: clean
clean:
	rm *.out
