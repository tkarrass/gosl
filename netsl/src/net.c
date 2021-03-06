#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <unistd.h>  // ?
#include <errno.h>   //   geguttenbergt aus bejees networking guide
#include <string.h>  // ?

// networking
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <time.h> // nanosleep

#include "net.h"


// little helper
static void *get_in_addr(struct sockaddr *sa) {
	switch (sa->sa_family) {
		case AF_INET:
			return &(((struct sockaddr_in*)sa)->sin_addr);
			break;
		case AF_INET6:
			return &(((struct sockaddr_in6*)sa)->sin6_addr);
			break;
		case AF_UNSPEC:
		default:
			return NULL;
	}
}


// server mode
// pumpt im for(;;) den status ins eth
//
int run_server(const struct prog_info *pinfo, char *img, int w, int h, int frms) {
	struct addrinfo hints, *servinfo, *p;
	int ret;
	int sockfd;
	char portbuf[6];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	sprintf(portbuf, "%d", pinfo->port);
	if ((ret = getaddrinfo("255.255.255.255", portbuf, &hints, &servinfo)) != 0) {
		fprintf(stderr, "so getaddrinfo error: %s\n", gai_strerror(ret));
		return 1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
	 	if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("talker: much socket error");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: such fail to bind socket\n");
		return 2;
	}

	struct timespec tim;
	tim.tv_sec = 0;
	tim.tv_nsec = 1000000000 / pinfo->fps;

	// Einem Socket muss das Broadcasting explizit erlaubt werden:
 	int broadcastPermission = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission, sizeof(broadcastPermission)) < 0){
		fprintf(stderr, "very setsockopt error");
		exit(1);
	}

	//
	int t = 0;
	for (;;) {
		//printf("sending...\n");
		int numbytes;
		
		t++;
		t %= pinfo->width;
		struct message *outmsg = (struct message *) malloc(sizeof(struct message));
		outmsg->timestamp = (uint32_t)t; //(uint32_t)time(NULL);
		// send image every 100 frames
		bool sendimg = (t % 100) == 0;
		outmsg->width  = sendimg ? w   : 0;
		outmsg->height = sendimg ? h   : 0;
		outmsg->frames = sendimg ? frms: 0;
		outmsg->image  = sendimg ? img : NULL;

		int buflen = getBufferSize(outmsg);
		char *outbuf = (char *) malloc(buflen);
		serialize(outbuf, outmsg);

		if ((numbytes = sendto(sockfd, outbuf, buflen, 0, p->ai_addr, p->ai_addrlen)) == -1) {
			perror("much error while sending");
			return -42;
		}
		nanosleep(&tim, NULL);
	}

	freeaddrinfo(servinfo);

	close(sockfd);

	return 0;
}


// lauschangriff:
// einfach mal in den äther horchen und alle messages rausnehmen die wo da gibt.
//
int run_client(const struct prog_info *pinfo, void (*framecallback)(const struct message *, const struct prog_info *) ) {

	struct addrinfo hints, *servinfo;
	char portbuf[6];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;			// IPv4
	hints.ai_socktype = SOCK_DGRAM;		// UDP
	hints.ai_flags = AI_PASSIVE;

	sprintf(portbuf, "%d", pinfo->port);

	int rv;
	if ((rv = getaddrinfo(NULL, portbuf, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	printf("got local address\n");
	
	struct addrinfo *info;
	int sockfd;
	// mal drin lassen: falls die ip der gegenstelle relevant werden sollte: char s[INET6_ADDRSTRLEN];
	for (info = servinfo; info != NULL; info = info->ai_next) {
		if ((sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) == -1) {
			perror("very socket error");
			continue;
		}

		if (bind(sockfd, info->ai_addr, info->ai_addrlen) == -1) {
			perror("much bind error");
			continue;
		}

		break;
	}
	if (!info) {
		fprintf(stderr, "many unbound\n");
		return 2;
	}
	printf("so check!\n");

	freeaddrinfo(servinfo); // free whole list

	struct sockaddr_storage their_addr;
	socklen_t addr_len = sizeof their_addr;
	int numbytes;
	char buf[10000];
	do {
		if ((numbytes = recvfrom(sockfd, buf, 9999 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}
		
		//printf("listener: got packet from %s\n", inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
		//printf("listener: packet is %d bytes long\n", numbytes);
		//buf[numbytes] = '\0';
		//printf("listener: packet contains \"%s\"\n", buf);

		struct message *msg = (struct message *)malloc(sizeof(struct message));
		deserialize(msg, buf);

		framecallback(msg, pinfo);

	} while (strncmp(buf, "much exit", 10000));
	
	close(sockfd);
	
	return 0;
}
