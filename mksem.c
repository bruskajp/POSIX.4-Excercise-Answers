// Make a named semaphore and leave it

#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char** argv){
	sem_t* s;
	
	if(argc != 2){
		fprintf(stderr, "Usage: %s semaphore-name\n", argv[0]);
		exit(1);
	}

	// Create semaphore
	s = sem_open(argv[1], O_CREAT, S_IRWXU, 1); // value 1==>unlocked
	if(s == (sem_t*) -1){
		perror(argv[1]);
		exit(2);
	}
	exit(0);
}
