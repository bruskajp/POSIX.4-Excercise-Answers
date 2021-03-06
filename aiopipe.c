#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>


//#ifdef _POSIX_ASYNCHRONOUS_iO
	#include <aio.h>
//#else
//	#error Error: Need asynchronous I/O!
//#endif

#define SIG_AIO_READ_DONE (SIGRTMIN)
#define SIG_AIO_WRITE_DONE (SIGRTMIN+1)

struct aiocb_plus{
	struct aiocb a;
	off_t curr_offset;
	char* buffer;
} a1, a2;

#define BUFSIZE (64*1024)
char buf1[BUFSIZE], buf2[BUFSIZE];

int aio_to_go; 	// Global flag

// Called when this read is complete
void aioread_done(int signo, siginfo_t* info, void* ignored){
	struct aiocb_plus* ap;
	ssize_t nbytes_read;
	
	ap = (struct aiocb_plus*) (info->si_value.sival_ptr);
	// No need to call aio_error here -- know AIO's done
	nbytes_read = aio_return(&ap->a);
	
	//fprintf(stderr, "buf1: %s buf2: %s ap_buf: %s", buf1, buf2, ap->a.aio_buf);


	if(nbytes_read > 0){
		//fprintf(stderr, "aio_to_go: %d nbytes_read: %d\n", aio_to_go, nbytes_read);
		// Read some data, so turn around and write it out.
		ap->a.aio_fildes = fileno(stdout);
		//ap->a.aio_buf = buf?		// This is taken care of by the write call
		ap->a.aio_nbytes = nbytes_read;
		ap->a.aio_offset = ap->curr_offset;
		ap->a.aio_reqprio = 0;
		ap->a.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
		ap->a.aio_sigevent.sigev_signo = SIG_AIO_WRITE_DONE;
		ap->a.aio_sigevent.sigev_value.sival_ptr = (void*) ap;
		aio_write(&ap->a);
	}else{
		aio_to_go--;
	}

		
}

// Called when this write is complete
void aiowrite_done(int signo, siginfo_t* info, void* ignored){
	struct aiocb_plus* ap;
	ssize_t nbytes_written;
	
	
	ap = (struct aiocb_plus*) (info->si_value.sival_ptr);
	// No need to call aio_error here -- know AIO's done
	nbytes_written = aio_return(&ap->a);
	// Fire up another aio_read, skipping the data being read by our peer.
	ap->a.aio_fildes = fileno(stdout);
	if(ap->a.aio_buf == buf2){
		ap->a.aio_buf = buf1;
	}else{
		ap->a.aio_buf = buf2;
	}
	ap->a.aio_nbytes = BUFSIZE;
	ap->curr_offset += 2*BUFSIZE;
	ap->a.aio_offset = ap->curr_offset;
	ap->a.aio_reqprio = 0;
	ap->a.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	ap->a.aio_sigevent.sigev_signo = SIG_AIO_READ_DONE;
	ap->a.aio_sigevent.sigev_value.sival_ptr = (void*) ap;
	aio_read(&ap->a);
	//aio_to_go++;
	// MAKE SURE IM NOT SUPPOSE TO INCREASE aio_to_go

}

int main(int argc, char** argv){
	sigset_t allsigs;
	struct sigaction sa;
	int flags;	

	// Handler for read completions.
	sa.sa_sigaction = aioread_done;
	sa.sa_flags = SA_SIGINFO;
	// Prevent the WRITE signal from coming in while we're handling
	// a READ completion. Just to keep things more clean.
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIG_AIO_WRITE_DONE);
	if(sigaction(SIG_AIO_READ_DONE, &sa, NULL) < 0){
		perror("sigaction");
		exit(1);
	}	
	
	// Handler for write completions
	sa.sa_sigaction = aiowrite_done;
	sa.sa_flags = SA_SIGINFO;
	// Prevent the READ signal from coming in while we're handling
	// a WRITE completion. Just to keep things more clean.
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIG_AIO_READ_DONE);
	if(sigaction(SIG_AIO_WRITE_DONE, &sa, NULL) < 0){
		perror("sigaction");
		exit(1);
	}

	// Block these signals from the mainline code so we can saely 
	// examine the global variable aio_to_go
	sigemptyset(&allsigs);
	sigaddset(&allsigs, SIG_AIO_READ_DONE);
	sigaddset(&allsigs, SIG_AIO_WRITE_DONE);
	sigprocmask(SIG_BLOCK, &allsigs, NULL);

	aio_to_go = 2; // Global flag

	fcntl(fileno(stdout), F_GETFL, &flags);
	flags |= O_DSYNC;	// Just get the data down
	fcntl(fileno(stdout), F_SETFL, flags);

	// Set up asynchronous I/O
	a1.a.aio_fildes = fileno(stdout);
	a1.a.aio_buf = a1.buffer = buf1;
	a1.a.aio_nbytes = BUFSIZE;
	a1.a.aio_offset = a1.curr_offset = (off_t) 0;
	a1.a.aio_reqprio = 0;
	a1.a.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	a1.a.aio_sigevent.sigev_signo = SIG_AIO_READ_DONE;
	a1.a.aio_sigevent.sigev_value.sival_ptr = (void*) &a1;
	aio_read(&a1.a);

// *** PROBLEM WITH THE INTERACTION OF buf1 AND buf2 ***

	a2.a.aio_fildes = fileno(stdout);
	a2.a.aio_buf = a2.buffer = buf2;
	a2.a.aio_nbytes = BUFSIZE;
	a2.a.aio_offset = a2.curr_offset = (off_t) BUFSIZE;
	a2.a.aio_reqprio = 0;
	a2.a.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	a2.a.aio_sigevent.sigev_signo = SIG_AIO_READ_DONE;
	a2.a.aio_sigevent.sigev_value.sival_ptr = (void*) &a2;
	aio_read(&a2.a);

	// Let the signals take it from here!

	sigemptyset(&allsigs); // Mask no signals when we suspend
	while(aio_to_go){
		sigsuspend(&allsigs);
	}

	exit(0);
}

















