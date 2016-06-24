// NOT WORKING!

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "app.h"


void init_server();
void check_for_exited_children();
request_t await_request();
void service_request(request_t r);
void send_reply(request_t r);
void shutdown_server();


main(int argc, char **argv){
	request_t r;
	r.check = NULL;
	r.r_op = PASS;
	pid_t terminated;
	int status;

 	init_server();

	do {
		check_for_exited_children();
		r = await_request();
		service_request(r);
		send_reply(r);
	} while (r.check != NULL);

	shutdown_server();
	exit(0);
}

void init_server(){}

void check_for_exited_children(){
	// Deal with terminated children
	int status;
	pid_t terminated;
	terminated = waitpid(-1, &status, WNOHANG);
	if (terminated > 0){
		if (WIFEXITED(status)) {
			printf("Child %d exit(%d)\n", 
				terminated, WEXITSTATUS(status));
		}else if (WIFSIGNALED(status)){
			printf("Child %d got signal %d\n", 
				terminated, WTERMSIG(status));
		}else if (WIFSTOPPED(status)){
			printf("Child %d stopped by signal %d\n", 
				terminated, WSTOPSIG(status));
		}
	}
}

request_t await_request(){
	request_t temp; 
	return temp;
}

void service_request(request_t r){
	pid_t child;
	switch (r.r_op){
		case OP_NEW:
			// Create a new client process
			child = fork();
			if (child){
				// parent process
				break;
			}else{
				// child process
				execlp("terminal", "terminal application", "/dev/com1", NULL);
				perror("execlp");
				exit(1);
			}
			break;
		default:
			printf("Bad op %d\n", r.r_op);
			break;
	}
	return;
}

void send_reply(request_t r){
	union sigval sval;

	// Send a notification to the terminal
	sval.sival_ptr = r.r_params;
	if (sigqueue(r.r_requestor, SIG_QUERY_COMPLETE, sval) < 0){
		perror("sigqueue");
	}
}

void shutdown_server(){
	printf("Shutting down server\n");
	// Kill all children with a signal to the process group
	kill(0, SIG_GO_AWAY);
}


