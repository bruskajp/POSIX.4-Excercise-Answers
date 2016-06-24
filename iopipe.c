#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#define BUFSIZE (64*1024)

char buf[BUFSIZE];

int main(int argc, char** argv){
	ssize_t nbytes;
	int flags;

	fcntl(fileno(stdout), F_GETFL, &flags);
	flags |= O_DSYNC; 	// Just get the data down
	fcntl(fileno(stdout), F_SETFL, flags);
	
	while(1){
		if((nbytes = read(fileno(stdin), buf, BUFSIZE)) <= 0){
			break;
		}
		if(write(fileno(stdout), buf, nbytes) != nbytes){
			perror("write to stdout");
			break;
		}
	}
	exit(0);
}
