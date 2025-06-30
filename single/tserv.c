#include "common.h"

int init_tun(char* name); 
void tun_up(char* name); 
void tun_down(char* name); 
int init_server(); 

int main(int argc, char* argv[]) {
	//arg check
	if(argc != 2) 
		exit(EXIT_FAILURE); 

	//initialize fds
	int tunfd, servfd, clifd, e; 
	int port = atoi(argv[1]); 
	char* name = NULL; 
	tunfd = init_tun(name); tun_up(name); 
	fprintf(stdout, "TUN init succesful\n"); 
	servfd = init_server(port); 
	fprintf(stdout, "server socket init succesful\n"); 
	
	//wait for connection
	fprintf(stdout, "vpn server awaiting client connect\n"); 
	clifd = accept(servfd, NULL, NULL); 
	fprintf(stdout, "vpn client accepted tunneling connection"); 

	//setup pollfd
	struct pollfd pfds[2]; 
	pfds[0].fd = tunfd; 
	pfds[0].events = POLLIN; 
	pfds[1].fd = clifd; 
	pfds[1].events = POLLIN | POLLHUP; 

	//server should only read well formed IP packets 
	//because client is only sending well formed IP packets
	//could be an error if this is false
	uint8_t bbuf[MTU]; 
	memset(&bbuf, 0, MTU); 
	while(1) {
		e = poll(pfds, 2, -1); 
		if(e < 0)
			err("poll failure\n"); 

		if(pfds[1].revents & POLLHUP) {
			fprintf(stdout, "client terminated connection without err\n"); 
			break; 
		}
		if(pfds[1].revents & POLLIN) {
			e = recv(clifd, &bbuf, MTU, 0); 
			if(e == 0) {
				fprintf(stdout, "client terminated connection without err\n"); 
				break; 
			} if (e < 0) {
				fprintf(stdout, "client terminated connection with err\n"); 
				break; 
			}
			write(tunfd, &bbuf, e); 
			memset(&bbuf, 0, MTU); 
		}
		if(pfds[1].revents & POLLIN) {
			e = read(tunfd, &bbuf, MTU); 
			send(clifd, &bbuf, e, 0); 
			memset(&bbuf, 0, MTU); 
		}
	}

	tun_down(name); close(tunfd); 
	close(servfd); close(clifd); 
	exit(0); 
}

//returns fd of TUN and writes its name to name
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
		err("TUNSETIFF failed\n"); 
	}
	e = ioctl(fd, TUNSETPERSIST, &per); 
	if(e < 0) {
		close(fd); 
		err("TUNSETPERSIST failed\n"); 
	}

	strcpy(name, ifr.ifr_name); 
	return fd; 
}

void tun_up(char* name) {
	char cmd[1024]; 
	memset(&cmd, 0, 1024); 
	/* Enable forwarding
	 * Add TUN IP
	 * Bring TUN up 
	 * masquerade 
	 */
	//enable forwarding
	system("echo 1 > /proc/sys/net/ipv4/ip_forward"); 
	system("echo 1 > /proc/sys/net/ipv6/conf/all/forwarding"); 

	//add IP addresses
	snprintf(cmd, 1024, "ip addr add 10.8.0.1/24 dev %s", name); 
	system(cmd); memset(&cmd, 0, 1024); 
	snprintf(cmd, 1024, "ip -6 addr add fd00::1/64 dev %s", name); 
	system(cmd); memset(&cmd, 0, 1024); 
	snprintf(cmd, 1024, "ip link set %s up", name); 
	system(cmd); memset(&cmd, 0, 1024); 

	//enable masquerading
	system("iptables -t nat -A POSTROUTING -o wlp3s0 -j MASQUERADE"); 
	system("ip6tables -t nat -A POSTROUTING -o wlp3s0 -j MASQUERADE"); 
}

void tun_down(char* name) {
	char cmd[1024]; 
	memset(&cmd, 0, 1024); 
	//disable forwarding 
	system("echo 0 > /proc/sys/net/ipv4/ip_forward"); 
	system("echo 0 > /proc/sys/net/ipv6/conf/all/forwarding"); 

	//remove IP addresses 
	snprintf(cmd, 1024, "ip addr del 10.8.0.1/24 dev %s", name); 
	system(cmd); memset(&cmd, 0, 1024); 
	snprintf(cmd, 1024, "ip -6 addr del fd00::1/64 dev %s", name); 
	system(cmd); memset(&cmd, 0, 1024); 
	snprintf(cmd, 1024, "ip link set %s down", name); 
	system(cmd); memset(&cmd, 0, 1024); 

	//disbale masquerading
	system("iptables -t nat -D POSTROUTING -o wlp3s0 -j MASQUERADE"); 
	system("ip6tables -t nat -D POSTROUTING -o wlp3s0 -j MASQUERADE"); 
}

int init_server(int port) {
	struct sockaddr_in saddr; 
	int fd, e, opt = 1; 
	socklen_t saddr_len = sizeof(saddr); 
	memset(&saddr, 0, sizeof(saddr)); 

	saddr.sin_family = AF_INET; 
	saddr.sin_addr.s_addr = INADDR_ANY; 
	saddr.sin_port = htons(port); 
	
	if(fd = socket(AF_INET, SOCK_STREAM, 0) < 0)
		err("server socket init failed\n"); 
	if(e = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt) < 0))
		err("setsockopt failed for server\n"); 
	if(e = bind(fd, (struct sockaddr*) &saddr, saddr_len) < 0)
		err("server bind failed\n"); 
	if(e = listen(fd, 1) < 0)
		err("listen failed\n"); 

	return fd; 
}
