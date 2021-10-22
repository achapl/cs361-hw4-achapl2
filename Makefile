hw4: *.c *.h
	gcc -O0 -g ./hw4-library/memlib.c ./hw4-library/mm.c hw4.c main.c -o hw4 --std=gnu99
