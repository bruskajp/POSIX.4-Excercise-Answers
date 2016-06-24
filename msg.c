#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <mqueue.h>
#include <errno.h>
#include <pthread.h>

int nreads = 0, nwrites = 0;
pid_t chpid, parentpid;

char* progname;
char* whoami;

#define DEFAULT_NBYTES 4
int nbytes = DEFAULT_NBYTES;

#define MQ_ONE	"/mq_one"
#define MQ_TWO	"/mq_two"

char* buf;

void usage(){
	printf("USage: %s {nbytes} (default nbytes is %d) \n", progname, DEFAULT_NBYTES);
	exit(1);
}

void be_a_child(mqd_t write_this, mqd_t read_this, unsigned int priority);
void be_the_parent(mqd_t write_this, mqd_t read_this, unsigned int priority);

int main(int argc, char** argv){
	struct sigaction sa;
	extern void alarm_handler(int);
	mqd_t m1, m2;
	struct mq_attr ma;
	
	progname = argv[0];

	if(argc == 2){
		nbytes = atoi(argv[1]);
	}else if(argc > 2){
		usage();
	}

	printf("Testing IPC through POSIX.4 mqs using %d-byte sends/recvs\n", nbytes);

	if((buf = (char *)malloc(nbytes)) == NULL){
		perror("malloc");
		exit(1);
	}else{
		buf[0] = 'a';
	}

	//Set up signals used for terminating the experiment
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = alarm_handler; // Terminates experiment
	if(sigaction(SIGALRM, &sa, NULL) < 0){
		perror("sigaction SIGALRM");
		exit(1);
	}

	ma.mq_flags = 0; // No special behavior
	ma.mq_maxmsg = 1;
	ma.mq_msgsize = nbytes;
	int i = mq_unlink(MQ_ONE); // Deal with possible leftovers
	if((i<0) && (errno != ENOENT)){
		perror("mq_unlink 1");
		exit(1);
	}
	i = mq_unlink(MQ_TWO); // Deal with possible leftovers
	if((i<0) && (errno != ENOENT)){
		perror("mq_unlink 2");
		exit(1);
	}
	if((m1 = mq_open(MQ_ONE, O_CREAT|O_EXCL|O_RDWR, S_IRWXU, &ma)) < 0){
		perror("mq_open 1");
		exit(1);
	}
	if((m2 = mq_open(MQ_TWO, O_CREAT|O_EXCL|O_RDWR, S_IRWXU, &ma)) < 0){
		perror("mqopen 2");
		exit(1);
	}
	
	// Duplicate the process
	switch(chpid = fork()){
		case -1:	// error
			perror("fork");
			exit(2);
			break;
		case 0:		// child
			whoami = "child";
			be_a_child(m1, m2, 0);
			exit(0);
			break;
		default:	// parent
			whoami = "parent";
			be_the_parent(m2 , m1, 0);
			exit(0);
			break;
	}

	fprintf(stderr, "Unexpected exit from test program!\n");
	exit(3);
}

void be_a_child(mqd_t write_this, mqd_t read_this, unsigned int priority){
	int ret;
	unsigned int prio;

	while(1){
		if((ret=mq_receive(read_this, buf, nbytes, &prio)) != nbytes){
			printf("Returned %d bytes trying to read %d\n", ret, nbytes);
			perror("child read from msg queue");
			exit(1);
		}
		nreads++;
		if(mq_send(write_this, buf, nbytes, priority) == -1){
			perror("child write to msg queue");
			exit(1);
		}
		nwrites++;
	}
}

void be_the_parent(int write_this, int read_this, unsigned int priority){
	unsigned int prio;
	
	alarm(60);
	while(1){
		//fprintf(stderr, "buf: %c\n", buf[0]);
		if(mq_send(write_this, buf, nbytes, priority) == -1){
			perror("parent write to msg queue");
			exit(1);
		}
		nwrites++;
		if(mq_receive(read_this, buf, nbytes, &prio) != nbytes){
			perror("parent read from pipe");
			exit(1);
		}
		nreads++;
	}
}


void alarm_handler(int signo){
	printf("%d/%d reads/writes (%d bytes each) by %s (%d bytes sent/sec)\n",
			nreads, nwrites, nbytes, whoami, (nwrites*nbytes)/60);
	if(getpid() != chpid){	// Parent--kill child too
		kill(chpid, SIGALRM);	
	}
	exit(0);
}




