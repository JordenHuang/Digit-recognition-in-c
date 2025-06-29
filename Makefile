CC = gcc
CFLAGS = -O2 -Wall -Wextra
CLIBS = -lm -I. -Itools -Ithirdparty

all:

train:
	$(CC) train.c -o train.out $(CFLAGS) $(CLIBS)

# predict: predict.c
# 	$(CC) predict.c -o predict.out $(CFLAGS) $(CLIBS)

squash: tools/mnist_squash.c
	$(CC) tools/mnist_squash.c -o tools/mnist_squash.out $(CFLAGS) $(CLIBS)

.PHONY: clean
clean:
	rm *.out
