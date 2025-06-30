#ifndef S_UTILS_H
#define S_UTILS_H

#include "common.h"
//every function returns -1 upon failure 
//and a function specific value on success

struct server {
	int serverfd; 
	int clifd; 
	int tunfd; 
}; 
typedef struct server server; 

int init_tun() {

}

int tun_up() {

}

server* init_server() {
	
}

int tun_down() {

}

int accept_cli(int serverfd) {

}


#endif 
