obj = fdList.o
target=fdList_test
cc = gcc -m32 -g -std=c99

$(target): fdList.h $(obj)
	$(cc) -o $(target) fdList_test.c $(obj)
fdList.o: fdList.h
	$(cc) -c fdList.c

.PHONY: clean
clean:
	-rm -f *.o $(target)
