// NOT WORKING!

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "app.h"


void terminate_normally(int signo);
void query_has_completed(int signo, siginfo_t* info, void* ignored);


char* myname;


main(int argc, char** argv){
	struct sigaction sa;
	/*sigset_t wait_for_these;

	sa.sa_handler = query_has_completed;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIG_QUERY_COMPLETE, &sa, NULL)){
		perror("sigaction")
		exit(1);
	}

	sigemptyset(&wait_for_these);
	sigaddset(&wait_for_these, SIG_QUERY_COMPLETE);
	sigprocmask(SIG_BLOCK, &wait_for_these, NULL);
	*/

	sa.sa_handler = terminate_normally;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIG_GO_AWAY, &sa, NULL)){
		perror("sigaction");
		exit(1);
	}

	sa_sigaction = query_has_completed;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO; // This is a queued signal
	if (sigaction(SIG_QUERY_COMPLETE, &sa, NULL)){
		perror("sigaction");
		exit(1);
	}

	myname = argv[0];
	printf("Terminal \"%s\" here!\n", myname);
	while(1){
		/* Deal with the screen */
		/* Await user input */
		(void)sigsuspend(&wait_for_these);
	}
	exit(0);
} 


void terminate_normally(int signo){
	// Exit gracefully
	exit(0);
}

void query_has_completed(int signo, siginfo_t* info, void* ignored){
	/* Deal with query completion. Query identifier could
	 * be stored as integer in info. */

	void* ptr_val = info->si_value.sival_ptr
}


