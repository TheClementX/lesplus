#include "common.h"
//TODO add sigint handling

int init_tun(char* name); 
void tun_up(char* name); 
void tun_down(char* name); 
int init_cli(char* ip, int port); 

int main(int argc, char* argv[]) {
	if(argc != 3) 
		exit(EXIT_FAILURE); 

	int tunfd, clifd, e; 
	int port = atoi(argv[2]); 
	char* name = NULL; 
	tunfd = init_tun(name); tun_up(name); 
	fprintf(stdout, "TUN init succesful\n"); 
	clifd = init_cli(argv[1], port); 
	fprintf(stdout, "client socket init succesful\n"); 

	struct pollfd pfds[2]; 
	pfds[0].fd = clifd; 
	pfds[0].events = POLLIN; 
	pfds[1].fd = tunfd; 
	pfds[1].events = POLLIN; 

	uint8_t bbuf[MTU]; 
	memset(&bbuf, 0, MTU); 
	while(1) {
		e = poll(pfds, 2, -1); 
		if(e < 0)
			err("poll failed\n"); 
		
		if(pfds[0].revents & POLLIN) {
			e = recv(clifd, &bbuf, MTU, 0); 	
			if(e <= 0) {
				fprintf(stdout, "server terminated connection\n"); 
				break; 
			}
			write(tunfd, &bbuf, e); 
			memset(&bbuf, 0, MTU); 
		}
		if(pfds[1].revents & POLLIN) {
			e  = read(tunfd, &bbuf, MTU); 
			send(clifd, &bbuf, e, 0); 
			memset(&bbuf, 0, MTU); 
		}
	}

	tun_down(name); close(tunfd); 
	close(clifd); 
	exit(0); 
}

int init_tun(char* name) {
	struct ifreq ifr; 
	int fd, e, per = 0; 
	char* clone = "/dev/net/tun"; 
	memset(&ifr, 0, sizeof(ifr)); 
	
	if(fd = open(clone, O_RDWR) < 0)
		err("tun open failed"); 

	if(name != NULL) strcpy(ifr.ifr_name, name); 
	else strcpy(ifr.ifr_name, "\0"); 
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI; 

	e = ioctl(fd, TUNSETIFF, (void*)&ifr); 
	if(e < 0) {
		close(fd); 
		err("TUNSETIFF failed"); 
	}
	e = ioctl(fd, TUNSETPERSIST, &per); 
	if(e < 0) {
		close(fd); 
		err("TUNSETPERSIST failed"); 
	}

	strcpy(name, ifr.ifr_name); 
	return fd; 
}

void tun_up(char* name) {
	char cmd[1024]; 
	memset(&cmd, 0, 1024); 

	snprintf(cmd, 1024, "ip addr add 10.8.0.2/24 dev %s", name); 
	system(cmd); memset(&cmd, 0, 1024); 
	snprintf(cmd, 1024, "ip -6 addr add fd00::2/64 dev %s", name); 
	system(cmd); memset(&cmd, 0, 1024); 
	snprintf(cmd, 1024, "ip link set %s up", name); 
	system(cmd); memset(&cmd, 0, 1024); 

	snprintf(cmd, 1024, "ip route add default via 10.8.0.2/24 dev %s metric 50", name); 
	system(cmd); memset(&cmd, 0, 1024); 
	snprintf(cmd, 1024, "ip -6 route add default via fd00::2/64 dev %s metric 50", name); 
	system(cmd); memset(&cmd, 0, 1024); 
}

void tun_down(char* name) {
	char cmd[1024]; 
	memset(&cmd, 0, 1024); 

	snprintf(cmd, 1024,  "ip addr del 10.8.0.2/24 dev %s", name); 
	system(cmd); memset(&cmd, 0, 1024); 
	snprintf(cmd, 1024,  "ip -6 addr del fd00::2/64 dev %s", name); 
	system(cmd); memset(&cmd, 0, 1024); 

	snprintf(cmd, 1024, "ip route del default via 10.8.0.2/24 dev %s", name); 
	system(cmd); memset(&cmd, 0, 1024); 
	snprintf(cmd, 1024, "ip -6 route del default via fd00::2/64 dev %s", name); 
	system(cmd); memset(&cmd, 0, 1024); 

	snprintf(cmd, 1024, "ip link set %s down", name); 
	system(cmd); memset(&cmd, 0, 1024); 
}


int init_cli(char* ip, int port) {
	int clifd, e; 
	struct sockaddr_in saddr; 
	memset(&saddr, 0, sizeof(saddr)); 
	saddr.sin_family = AF_INET; 
	saddr.sin_port = htons(port); 
	if(e = inet_pton(AF_INET, ip, &saddr.sin_addr) < 0)
		err("inet_pton failed\n"); 

	if(clifd = socket(AF_INET, SOCK_STREAM, 0) < 0)
		err("clifd socket() failed\n"); 
	if(e = connect(clifd, (struct sockaddr*) &saddr, sizeof(saddr)) < 0)
		err("connect() to vpn failed\n"); 

	return clifd; 
}
