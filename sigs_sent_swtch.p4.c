#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

int nsigs_sent = 0, nsigs_recv = 0;
pid_t chpid, parentpid;

void be_a_child();
void be_the_parent();

int main(){
	struct sigaction sa;
	extern void alarm_handler(), child_terminate();
	extern void handler(int, siginfo_t*, void*);
	sigset_t blockem;

	parentpid = getpid();
	sigemptyset(&blockem);
	sigaddset(&blockem, SIGRTMIN);
	sigprocmask(SIG_BLOCK, &blockem, NULL);

	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = alarm_handler;	// Terminates Experiment
	
	if(sigaction(SIGALRM, &sa, NULL) < 0){
		perror("sigaction SIGALRM");
		exit(1);
	}

	sa.sa_handler = child_terminate;	// Terminates child
	if(sigaction(SIGUSR2, &sa, NULL)){
		perror("sigaction SIGUSR2");
		exit(1);
	}

	sigemptyset(&sa.sa_mask);	// No particular signal bloackage
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = handler;
	if(sigaction(SIGRTMIN, &sa, NULL) < 0){
		perror("sigaction SIGRTMIN");
		exit(1);
	}

	switch(chpid = fork()){
		case -1:	// error
			perror("fork");
			exit(2);
			break;
		case 0:		// child
			be_a_child();
			exit(0);
			break;
		default:	// parent
			be_the_parent();
			exit(0);
			break;
	}
	
	fprintf(stderr, "Unexpected exit from test program!\n");
	exit(3);
}

void be_a_child(){
	sigset_t sigset;
	union sigval val;
	
	sigemptyset(&sigset);
	val.sival_int = 0;
	while(1){
		sigsuspend(&sigset);
		if(sigqueue(parentpid, SIGRTMIN, val) < 0){
			perror("sigqueue");
			return;
		}
		nsigs_sent++;
		val.sival_int++;	// Send different extra information
	}
}

void be_the_parent(){
	sigset_t sigset;
	union sigval val;

	sigemptyset(&sigset);
	alarm(60);
	val.sival_int = 0;
	while(1){
		if(sigqueue(chpid, SIGRTMIN, val) < 0){
			perror("sigqueue");
			return;
		}
		nsigs_sent++;
		sigsuspend(&sigset);
		val.sival_int++; 	// Send different information
	}
}

void handler(int signo, siginfo_t* info, void* extra){
	nsigs_recv++;
	//int i = (*info).si_value.sival_int;
	//printf("signo: %d\textra: %d\n", signo, i);
}

void child_terminate(int signo){
	printf("%d/%d signals sent/received by child (%d sent/sec)\n",
			nsigs_sent, nsigs_recv, nsigs_sent/60);
	exit(0);
}

void alarm_handler(int signo){
	printf("%d/%d signals sent/received by child (%d sent/sec)\n",
			nsigs_sent, nsigs_recv, nsigs_sent/60);
	exit(0);
}




