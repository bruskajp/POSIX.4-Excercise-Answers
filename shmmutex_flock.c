#include <unistd.h> 	// POSIX et a
#include <limits.h>		// PAGESIZE
#include <sys/types.h>	// fnctl
#include <sys/mman.h>	// shm_open, mmap
#include <signal.h>		// sigaction
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>		// O_CREAT and other macros
#include <string.h> 	// strlen()

#define SHMNAME		"/my_shm"
#define TIME_PERIOD 60			// Run test for a minute
#define PAGESIZE getpagesize()

int iterations = 0;
int nbytes;	// Test, passing 4 bytes of information

void timer_expired(int called_via_signal){
	printf("%d iterations for region of %d bytes\n", iterations, nbytes);
	exit(0);
}

int main(int argc, char** argv){
	int d, lockfile;
	char* addr;
	struct sigaction sa;
	struct flock lockit, unlockit;

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
	
	lockfile = open("lockfile", O_RDWR|O_CREAT|O_TRUNC, S_IRWXU);
	write(lockfile, "A Couple Of Bytes", strlen("A Couple Of Bytes"));
	unlink("lockfile");	// So it foes away on exit

	// Begin test (get hold, write, release hold)
	lockit.l_type = F_WRLCK;
	lockit.l_whence = SEEK_SET;
	lockit.l_start = 0;
	lockit.l_len = 1;

	unlockit = lockit;
	unlockit.l_type = F_UNLCK;

	alarm(TIME_PERIOD);
	while(1){
		// Acquire parent's lock
		if(fcntl(lockfile, F_SETLKW, &lockit) < 0){
			perror("fcntl(F_SETLKW wrlock)");
			exit(1);
		}	
		// Store data in shared memory aread
		int i;
		for(i = 0; i<nbytes; i++){
			addr[i] = 'A';
		}
		// Release parent's lock
		if(fcntl(lockfile, F_SETLKW, &unlockit) < 0){
			perror("fcntl(F_SETLKW unlock)");
			exit(1);
		}
		iterations++;
	}

}























