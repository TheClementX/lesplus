#ifndef COMMON_H
#define COMMON_H

#include <sys/socket.h> 
#include <sys/types.h> 
#include <signal.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h>
#include <stdarg.h>
#include <netdb.h>
#include <stdbool.h>
#include <poll.h>
#include <Linux/if.h>
#include <Linux/if_tun.h>
#include <sys/ioctl.h>

#define MAXLINE 4096

void err(char* er_message) {
	fprintf(stderr, "%s", er_message); 
	exit(EXIT_FAILURE); 
}

#endif 
