objs = fstr.o fdList.o fdict.o fintset.o fbintree.o fheap.o \
client.o command.o table.o event.o common.o persist.o \
listType.o hashType.o

cc = gcc -m32 -g -std=c99 -D_XOPEN_SOURCE
target = saber

$(target): saber.h $(objs)
	$(cc) -o $(target) saber.c $(objs)
fstr.o: fstr.h
	$(cc) -c fstr.c
fdList.o: fdList.h
	$(cc) -c fdList.c
fdict.o: fdict.h
	$(cc) -c fdict.c
fintset.o: fintset.h
	$(cc) -c fintset.c
fbintree.o: fbintree.h
	$(cc) -c fbintree.c
fheap.o: fheap.h
	$(cc) -c fheap.c

########### saber.h split into many files to implement #########
client.o: saber.h
	$(cc) -c client.c
command.o: saber.h
	$(cc) -c command.c
table.o: saber.h
	$(cc) -c table.c
event.o: saber.h
	$(cc) -c event.c
common.o: common.h
	$(cc) -c common.c
persist.o: saber.h
	$(cc) -c persist.c
listType.o: saber.h
	$(cc) -c listType.c
hashType.o: saber.h
	$(cc) -c hashType.c

.PHONY: clean
clean: 
	-rm -f *.o
	-rm -f $(target)
