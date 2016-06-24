#define SIG_GO_AWAY SIGUSR1
//#define SIG_QUERY_COMPLETE SIGUSR2
#define SIG_QUERY_COMPLETE SIGRTMIN

typedef enum {
	OP_NEW,
	PASS
} OP;

typedef struct request_t{
	OP r_op;
	int* check;
	pid_t r_requestor;
	int* r_params;
} request_t;


