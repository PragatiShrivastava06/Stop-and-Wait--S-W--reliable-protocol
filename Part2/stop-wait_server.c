/*
compile - "gcc stop-wait_server.c -o stop-wait_server"
execute - "./stop-wait_server"
*/
#include<stdio.h>
#include<string.h>
#include <sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<math.h>
#include "checksum_ascii_server.c"
#define CHKSUMGENERATORSIZE 9
#define LENGTH 20

int main()
{
int socket_desc_server, socket_accepted, client_address_size, j;
struct sockaddr_in server;
socklen_t addrlen = sizeof(server); /* length of addresses */
char fname[512];

struct{
	char msgseq[100], ackno[100];
	char sendingpkt[100];
	int ACK;
	int rcvdchksum;
	} packet;

//Create socket
socket_desc_server = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_desc_server == -1)
	printf("Could not create socket");

//Prepare the sockaddr_in server structure - will accept connection request from any IPv4 address
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 30000 );

    //Bind
    if( bind(socket_desc_server,(struct sockaddr *)&server , sizeof(server)) < 0){
        puts("bind failed");
	return 1;
    }
    puts("\n[Server] - bind done on UDP, it is always in listening mode..Waiting for incoming connections...\n");

//Receive file name and later write into it
	ssize_t filename_size = recv(socket_desc_server, fname, 10, 0);
	if (filename_size<0){
	printf("[Server] - File name not recieved \n");
	return 1;
	}
	else
	fname[filename_size] = '\0';
	printf("[Server] - Received file name is %s \n", fname);

FILE *pf_write;
int i = 1, msgno, expctdframe = 0, ACK, chksum_computed;
size_t len=1, max_len = 100, rcvdata_len; //Receive data length, can be changed
char rcvbuf_withmsgno[LENGTH], sendack[1], *temp; //Receive buffer
char* f_name = fname;

int rcv_data()
{
	printf("\n--------SERVER PACKET info--------\n");
	printf("[Server] - Received data of %zu bytes \n", rcvdata_len);
	printf("[Server] - Received packet --------->%s \n", rcvbuf_withmsgno);
	printf("[Server] - Expected Msg seq no. %d \n", expctdframe);

//Extract Msg no.
	msgno = rcvbuf_withmsgno[0] - '0';
	printf("[Server] - Received Msg seq no. %d \n", msgno);

//Extract checksum value from recived buffer
	packet.rcvdchksum = rcvbuf_withmsgno[1] - '0';
	printf("[Server] - Recived checksum %d \n", packet.rcvdchksum);

//+2, since wants to exclude msgno and chksum. -2 also for the same reason.
	chksum_computed = checksum_ascii_server(rcvbuf_withmsgno+2, rcvdata_len-2, CHKSUMGENERATORSIZE);

	if(expctdframe == msgno){
		printf("[Server] - Msg seq no. is fine \n");

//Successful case - Send msgno to sender
		if(chksum_computed == packet.rcvdchksum){
		ACK = !expctdframe;
		printf("[Server] - Data is fine as well...sending ACK %d \n", ACK);
		snprintf(sendack, 2,"%d", ACK);
		
		if ((sendto(socket_desc_server, sendack, len, 0, (struct sockaddr *)&server, addrlen)) < 0){
		perror("[Server] - Reason2 msgno send failed %m \n");
		return 0;
		}
		expctdframe = ACK;//Wait for next frame
		printf("[Server] - expctdframe for next packet %d \n", expctdframe);

//Skipping initial two values, since they belong to Msgseq no. and received checksum value and then writing into output file
	printf("[Server] - trying to writing data - %s \n", rcvbuf_withmsgno+2);
	if(pf_write == NULL){
	perror ("The following error occurred");
	return 0;
	}
	else
	fwrite(rcvbuf_withmsgno+2, sizeof(char), rcvdata_len-2, pf_write);
	puts("[Server] - Write successful \n");
	}

//Checksum error - Recieved Data corrupt
		else if(chksum_computed != packet.rcvdchksum){
		printf("[Server] - Data corrupt\n");
		ACK = expctdframe;
		printf("[Server] - Sending Old ACK...expctdframe for next packet %d \n", expctdframe);
		snprintf(sendack, 2,"%d", ACK);
			if ((sendto(socket_desc_server, sendack, len, 0, (struct sockaddr *)&server, addrlen)) < 0){
		perror("[Server] - Data is fine but msgno send failed %m \n");
		return 0;
		}
	}
}

//Msgno. corrupted - send ACK = expctdframe back to client indicating expected frame
	else if(expctdframe != msgno){
		ACK = expctdframe;
		snprintf(sendack, 2,"%d", ACK);
		printf("[Server] - Wrong Msg no. \n");

//Checksum(Data) fine
		if(chksum_computed == packet.rcvdchksum)
		printf("[Server] - Data is fine \n");

//Data also corrupted
		else if(chksum_computed != packet.rcvdchksum)
		printf("[Server] - Data wrong \n");

		printf("[Server] - Sending Old ACK...expctdframe for next packet %d \n", expctdframe);
//send ACK = expctdframe back to client indicating expected frame.
			if ((sendto(socket_desc_server, sendack, len, 0, (struct sockaddr *)&server, addrlen)) < 0){
			perror("[Server] - Data is fine but msgno send failed %m \n");
			return 0;
			}
}

	else{
	printf("[Server] - Multi bits flipped, cannot handle this");
	return 0;
}
memset(rcvbuf_withmsgno, 0 , 100);
}

while(1){
	pf_write = fopen(f_name, "ab+");
	rcvdata_len = recvfrom(socket_desc_server, rcvbuf_withmsgno, max_len, 0, (struct sockaddr *)&server, &addrlen);
	if(rcvdata_len<0){
	perror("[Server] - Receive failed %m \n");
	break ;
	}
	rcv_data();
	fclose(pf_write);
}
close(socket_desc_server);
}