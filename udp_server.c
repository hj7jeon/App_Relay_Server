//--------------------------------------------------------------
// file Name : udp_echoserv.c
// command : cc -o udp_echoserv  udp_echoserv.c
// server 시작 : udp_echoserv  9999
// client에서 전송되는 메시지를 buf.txt 에 저장하고, 다시 client로 전송 후 유효성 체크
//--------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define NUMBER_OF_CLIENT	2

#define UDP_PORT	0x8000

#define MAXLINE    1024
#define MAX_FILE_NAME	128

#define CONNECTION_REQ	0x74160001	
#define CONNECTION_RSP	0x74160001 + 0x1000

#define REPORT_DATA_REQ	0x74160002
#define REPORT_DATA_RSP	0x74160002 + 0x1000

#define PUSH_DATA_REQ	0x74160003
#define PUSH_DATA_RSP	0x74160003 + 0x1000

struct stMsg{
	unsigned int ulMsgId;
	unsigned int ulValue[4];		// Reserved
	char		cName[MAX_FILE_NAME];
};

static unsigned int convertOrder(unsigned int x)
{
	unsigned int result = (x & 0xFF) << 24;
	
	result |= ((x >> 8) & 0xFF) << 16;
	result |= ((x >> 16) & 0xFF) << 8;
	result |= ((x >> 24) & 0xFF);

	return result;
}

int main(int argc, char *argv[]) {
	struct sockaddr_in saved_cliaddr[NUMBER_OF_CLIENT];
	struct sockaddr_in servaddr, cliaddr;
	int socket_fd, nbyte, addrlen = sizeof(struct sockaddr);
	char buf[MAXLINE];
	struct stMsg up_buf;
	struct stMsg *stUdpMsg;
	int icnt;

	if((socket_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
	    perror("socket fail");
	    exit(0);
	}

	memset(saved_cliaddr, 0, sizeof(saved_cliaddr));

	memset(&cliaddr, 0, addrlen);
	memset(&servaddr, 0, addrlen);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(UDP_PORT);

	if(bind(socket_fd, (struct sockaddr *)&servaddr, addrlen) < 0) {
	    perror("bind fail");
	    exit(0);
	}

	printf("Server : waiting request.\n\r");
	
	while(1)
	{
	    memset(buf, 0, sizeof(buf));
	    memset((unsigned char *)&up_buf, 0x00, sizeof(struct stMsg));
	    
	    nbyte = recvfrom(socket_fd, buf, MAXLINE , 0, (struct sockaddr *)&cliaddr, &addrlen);
	    if(nbyte< 0) 
	    {
	        perror("recvfrom fail");
	        exit(1);
	    }
            memcpy(&up_buf, buf, sizeof(struct stMsg));

	    stUdpMsg = (struct stMsg *)buf;

		for(icnt=0; icnt < sizeof(struct stMsg); icnt++)
		{
			printf("%02X ",buf[icnt]);
		}
		printf("\n\r");
                printf("up_buf\n\r");
		up_buf.ulValue[0] = convertOrder(up_buf.ulValue[0]);
		up_buf.ulValue[1] = convertOrder(up_buf.ulValue[1]);
		up_buf.ulValue[2] = convertOrder(up_buf.ulValue[2]);
		up_buf.ulValue[3] = convertOrder(up_buf.ulValue[3]);
		printf("1 %d\n\r",up_buf.ulValue[0]);
		printf("2 %d\n\r",up_buf.ulValue[1]);
		printf("3 %d\n\r",up_buf.ulValue[2]);
		printf("4 %d\n\r",up_buf.ulValue[3]);
		printf("\n\r");
	    switch(ntohl(stUdpMsg->ulMsgId))
	    {
	    	case CONNECTION_REQ:

				printf("Get Connection Req Message\n\r");

				for(icnt=0; icnt<NUMBER_OF_CLIENT; icnt++)
				{
					if(saved_cliaddr[icnt].sin_addr.s_addr == 0)
					{
						memcpy(&saved_cliaddr[icnt], &cliaddr, sizeof(struct sockaddr_in));

						printf("Saved Address Count=%d IP_Add=%d.%d.%d.%d\n\r", \
							icnt, (saved_cliaddr[icnt].sin_addr.s_addr & 0xFF000000) >> 24,\
							(saved_cliaddr[icnt].sin_addr.s_addr & 0x00FF0000) >> 16,\
							(saved_cliaddr[icnt].sin_addr.s_addr & 0x0000FF00) >> 8,\
							(saved_cliaddr[icnt].sin_addr.s_addr & 0x000000FF) );

						break;
					}
				}
	    		up_buf.ulMsgId = htonl(CONNECTION_RSP);

				if(sendto(socket_fd, &up_buf, sizeof(&up_buf), 0, (struct sockaddr *)&cliaddr, addrlen) < 0)
				{
					perror("sendto fail");
					exit(0);
				    puts("sendto complete");
				}
	    		break;

			case REPORT_DATA_REQ:

				printf("Get Report Data Req\n\r");

				up_buf.ulMsgId = htonl(REPORT_DATA_RSP);
				printf("%d\n", up_buf.ulValue[0]);
				if(sendto(socket_fd, (unsigned char *)&up_buf, sizeof(struct stMsg), 0, (struct sockaddr *)&cliaddr, addrlen) < 0)
				{
					perror("sendto fail");
					exit(0);
				    puts("sendto complete");
				}

				up_buf.ulMsgId = htonl(PUSH_DATA_REQ);
				printf("%d\n", up_buf.ulValue[0]);
				for(icnt=0; icnt<NUMBER_OF_CLIENT; icnt++)
				{
					if(memcmp(&cliaddr, &saved_cliaddr[icnt], sizeof(struct sockaddr_in)) != 0)
					{
						printf("Send Push Data Req to %d.%d.%d.%d\n\r",
							(saved_cliaddr[icnt].sin_addr.s_addr & 0xFF000000) >> 24,\
							(saved_cliaddr[icnt].sin_addr.s_addr & 0x00FF0000) >> 16,\
							(saved_cliaddr[icnt].sin_addr.s_addr & 0x0000FF00) >> 8,\
							(saved_cliaddr[icnt].sin_addr.s_addr & 0x000000FF) );
							
						if((sendto(socket_fd, (unsigned char *)&up_buf, sizeof(struct stMsg), 0, (struct sockaddr *)&saved_cliaddr[icnt], addrlen)) < 0) {
					    	perror("sendto fail");
						    exit(0);
						}
						break;
					}
				}
				break;
	    }
	}
	
	close(socket_fd);

	return 0;
}
