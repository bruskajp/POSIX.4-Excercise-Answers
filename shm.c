#include <unistd.h>		// POSIX et al
#include <stdio.h>		
#include <limits.h>		// PAGESIZE
#include <stdlib.h>		
#include <sys/mman.h>	// shm_open, mmap
#include <sys/types.h>	// waitpid
#include <sys/wait.h>	// waitpid
#include <signal.h>		// sigaction
#include <fcntl.h>		// O_CREAT and related macros


#define PARENT_SHM_DATA "Parent Wrote This"
#define CHILD_PRE_EXEC_DATA "Child Wrote This Before Exec-ing"
#define CHILD_POST_EXEC_DATA "Child Wrote This AFTER Exec-ing"

//#define SHM_1 "SHM_1"
#define SHM_2 "SHM_2"

#ifdef SHM_1
#define SHMNAME		"/my_shm"
#define NEW_IMAGE	"shm_2"

void cleanup(int called_via_signal){
	(void) shm_unlink(SHMNAME);
	if(called_via_signal){
		exit(3);
	}
}

int main(int argc, char** argv){
	int d;
	char* addr;
	int chpid;
	int w;
	struct sigaction sa;

	// Handle premature termination cleanup
	sa.sa_handler = cleanup;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	(void) sigaction(SIGINT, &sa, NULL);
	(void) sigaction(SIGBUS, &sa, NULL);
	(void) sigaction(SIGSEGV, &sa, NULL);

	// Create shared memory region
	d = shm_open(SHMNAME, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU);
	int PAGESIZE = getpagesize();
	ftruncate(d, (off_t)PAGESIZE);	// Make region PAGESIZE big
	addr = (char*) mmap(NULL, (size_t)PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED, d, 0);
	
	// Put data in the shared memory region
	printf("Parent stores in SHM: \"%s\"\n", PARENT_SHM_DATA);
	sprintf(addr, PARENT_SHM_DATA);

	// Create a child process
	switch(chpid = fork()){
		case -1:	// error
			perror("fork");
			cleanup(0);
			exit(1);
			break;
		case 0:		// child
			break;
		default:		// parent; await child
			chpid = wait(&w);
			// Child is done: see what's in SHM
			printf("Parent sees in SHM: \"%s\"\n", (char*) addr);
			exit(0);
			break;
	}
	
	// Code executed by child
	printf("Child, pre-exec, sees in SHM: \"%s\"\n", addr);
	printf("Child, pre-exec, stores in SHM: \"%s\"\n", CHILD_PRE_EXEC_DATA);
	sprintf(addr, CHILD_PRE_EXEC_DATA);
	
	// Exec a new process image (problem with pathing)
	execlp("/home/iai/Coding/Posix.4/shm_2", NEW_IMAGE, SHMNAME, NULL);
	perror("returned from execlp");
	exit(2);
}
#endif 	// SHM_1

#ifdef SHM_2
int main(int argc, char** argv){
	int d;
	char* addr;
	
	// Single argument is the name of the shared memory region to map in
	#define SHMNAME argv[1]
	//#define SHMNAME argv[1]
	d = shm_open(SHMNAME, O_RDWR, S_IRWXU);
	int PAGESIZE = getpagesize();
	addr = (char*) mmap(NULL, (size_t)PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED, d, 0);
	printf("Child, post-exec, see: \"%s\"\n", addr);
	printf("Child, post-exec, stores in SHM: \"%s\"\n", CHILD_POST_EXEC_DATA);
	sprintf(addr, CHILD_POST_EXEC_DATA);

	exit(0);
}
#endif	// SHM_2


