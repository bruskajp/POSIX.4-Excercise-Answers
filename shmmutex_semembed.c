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
	struct fu {
		sem_t s;
		char beginning_of_data[1];
	} *p;

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
	p = (struct fu*) addr;

	// Create semaphore
	sem_init(&p->s, 1, 1); 	// pshared == 1, value 1==>unlocked
	
	// Begin test (get hold, write, release hold) 
	alarm(TIME_PERIOD);
	while(1){
		// Acquire parent's lock
		sem_wait(&p->s);
		// Store data in shared memory area
		int i;
		for(i=0; i<nbytes; i++){
			addr[i] = 'A';
		}		
		// Release parent's lock
		sem_post(&p->s);
		iterations++;
	}
	// Semaphore is automatically destroyed when shm is closed on exit

}























