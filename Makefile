CC = gcc
CFLAGS = -O2 -Wall -Wextra
CLIBS = -lm

all:
	$(CC) train.c -o train.out $(CFLAGS) $(CLIBS)

predict: predict.c
	$(CC) predict.c -o predict.out $(CFLAGS) $(CLIBS)

compress: mnist_compress.c
	$(CC) mnist_compress.c -o mnist_compress.out $(CFLAGS) $(CLIBS)

.PHONY: clean
clean:
	rm *.out
