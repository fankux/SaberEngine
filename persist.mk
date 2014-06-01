cc = gcc -m32 -std=c99 -g -D_XOPEN_SOURCE
target = persist_test

$(target):
	$(cc) persist.c -o $(target)

.PHONY: clean
clean:
	-rm -f $(target)
