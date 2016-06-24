#include <unistd.h> 	// POSIX et a
#include <limits.h>		// PAGESIZE
#include <sys/mman.h>	// shm_open, mmap
#include <signal.h>		// sigaction
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>		// O_CREAT and other macros
#include <string.h> 	// strlen()
#include <semaphore.h>	// semaphores


#define SHMNAME		"/my_shm"
#define SEMNAME		"/my_sem"
#define TIME_PERIOD 60			// Run test for a minute
#define PAGESIZE getpagesize()

unsigned int iterations = 0;
int nbytes;	// Test, passing 4 bytes of information

void timer_expired(int called_via_signal){
	printf("%u iterations for region of %d bytes\n", iterations, nbytes);
	exit(0);
}

int main(int argc, char** argv){
	int d;
	char* addr;
	struct sigaction sa;
	sem_t* s;

	if(argc == 2){
		nbytes = atoi(argv[1]); // Use #bytes passed
	}else{
		nbytes = 4;
	}

	sa.sa_handler = timer_expired;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	(void) sigaction(SIGALRM, &sa, NULL);
	
	// Create shared memory region
	d = shm_open(SHMNAME, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU);
	ftruncate(d, (off_t)PAGESIZE);	// Make region PAGESIZE big
	addr = (char*) mmap(NULL, (size_t)PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED, d, 0);
	shm_unlink(SHMNAME); 	// So it goes away on exit

	// Create semaphore
	s = sem_open(SEMNAME, O_CREAT, S_IRWXU, 1);	// value 1==>unlocked
	sem_unlink(SEMNAME);	// So it goes away on exit
	
	// Begin test (get hold, write, release hold) 
	alarm(TIME_PERIOD);
	while(1){
		// Acquire parent's lock
		sem_wait(s);
		// Store data in shared memory area
		int i;
		for(i=0; i<nbytes; i++){
			addr[i] = 'A';
		}		
		// Release parent's lock
		sem_post(s);
		iterations++;
	}
	// Semaphore is automatically close on exit, as is shm

}























