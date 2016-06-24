// Make a named semaphore and leave it

#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char** argv){
	if(argc != 2){
		fprintf(stderr, "Usage: %s semaphore-name\n", argv[0]);
		exit(1);
	}

	// Create semaphore
	if(sem_unlink(argv[1]) < 0){
		perror(argv[1]);
		exit(2);
	}
	exit(0);
}
