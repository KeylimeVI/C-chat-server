FLAGS = -Wall -g 
DEPENDENCIES = family.h reading.h word_list.h

all: wheel

wheel: wheel.o family.o reading.o word_list.o
	gcc ${FLAGS} -o $@ $^

wheel.o: wheel.c family.h reading.h word_list.h
	gcc ${FLAGS} -c $<

reading.o: reading.c reading.h
	gcc ${FLAGS} -c $<

word_list.o: word_list.c word_list.h
	gcc ${FLAGS} -c $<

family.o: family.c family.h
	gcc ${FLAGS} -c $<

clean: 
	rm -f *.o wheel *.gch
