all : truncate greeting basename


truncate : truncate.c
	gcc -Wall -g -o truncate truncate.c

greeting : greeting.c
	gcc -Wall -g -o greeting greeting.c

basename : basename.c
	gcc -Wall -g -o basename basename.c

clean : 
	-rm basename truncate greeting 
