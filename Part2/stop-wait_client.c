/*
compile - "gcc stop-wait_client.c -o stop-wait_client
execute - "./stop-wait_client 127.0.0.1 30000 output input.txt"
*/
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <inttypes.h>
#include <netinet/in.h>
#include "checksum_ascii_client.c"
#define LENGTH 20 // Buffer length
#define CHKSUMGENERATORSIZE 9

int main(int argc , char *argv[])
{
int socket_desc_client, new_socket;
struct sockaddr_in server;
socklen_t addrlen = sizeof(server); /* length of addresses */
struct{
	char msgseq[100], ackno[100], chksum[100];
	char sendingpkt[100];
	int ACK;
	} packet;

//Create UDP Socket of type IPv4
socket_desc_client = socket(AF_INET , SOCK_DGRAM , 0);
	if (socket_desc_client == -1)
	printf("[Client] - Could not create socket : %m\n");
	else
	printf("[Client] - Socket created successfully \n");

//Convert IP host address to network add
	server.sin_addr.s_addr = inet_addr(argv[1]); //IP address
	server.sin_family = AF_INET;
	long server_port = strtol(argv[2], (char **)NULL, 10); //Server port
	server.sin_port = htons(server_port);

//Connect socket fd to network address above
new_socket = connect(socket_desc_client , (struct sockaddr *)&server , sizeof(server));
	if (new_socket < 0){
	printf("[Client] - connect error : %m\n");
	return 1;
	}

// This part will send the filename for server to save the data
	if((sendto(socket_desc_client, argv[3], sizeof(argv[3]), 0, (struct sockaddr *)&server, addrlen))<0){
	printf("[Client] - Could not send file %m \n");
	return 1;
	}
	else
	printf("[Client] - File name send to server i.e. %s \n", argv[3]);

// Read file and send it across the socket
FILE *pf_read;
pf_read = fopen(argv[4], "rb");
	if (pf_read == NULL){
	printf("[Client] - Read/Send File not found!\n");
	return 1;
	}
	printf("[Client] - Read file - %s\n", argv[4]);

char sdbuf[50], *returnbuf, tempbuf[50];
int flag, framecount = 0, chksum;
size_t f_block_sz;

// Send buffer
int send_data()
{
printf("\n--------NEW CLIENT PACKET info--------\n");
printf("[Client] - Sending block size of %zu bytes \n", f_block_sz);

//Calculate checksum
	chksum = checksum_ascii_client(sdbuf, f_block_sz, CHKSUMGENERATORSIZE);
	memset(packet.chksum, 0 , 100);
	snprintf(packet.chksum, 5,"%d", chksum);

//Add Msg no. to it and send it
	int tmpfrmcnt;
	printf("[Client] - Sending Msg seq no. %d \n", framecount);
//Generate Error in Msg seq no.
	printf("[Client] - Want to Generate Error in Msg seq no. (1 / 0) : ");
	scanf("%d",&flag);
	if(flag == 1){
		tmpfrmcnt = !framecount;
		printf("[Client] - Now sending Msg seq no. %d \n", tmpfrmcnt);
		snprintf(packet.msgseq, 2,"%d", tmpfrmcnt);
}
	else{
		printf("[Client] - No Error generated in Msg seq no. \n");
		snprintf(packet.msgseq, 2,"%d", framecount); //Sending seq no as is
	}

	memset(packet.sendingpkt, 0 , f_block_sz);
	strcat(packet.sendingpkt, packet.msgseq);
	strcat(packet.sendingpkt, packet.chksum);

//Generate Error in payload
	printf("[Client] - Original Payload --------->%s \n", sdbuf);
	printf("[Client] - Want to Generate Error in Payload (1 / 0) : ");
	scanf("%d",&flag);
	if(flag == 1){
	int r = rand()%10;
	memset(tempbuf, 0 , f_block_sz);
	strcpy(tempbuf, sdbuf);
	tempbuf[r] = tempbuf[r] + 1;
	printf("[Client] - Payload with one bit flipped --------->%s \n", tempbuf);
	strcat(packet.sendingpkt, tempbuf);	
}
	else
	strcat(packet.sendingpkt, sdbuf);

//Send packet, +2 since sending msg seq no. and checksum
	if (sendto(socket_desc_client, packet.sendingpkt, f_block_sz+2, 0, (struct sockaddr *)&server, addrlen) >= 0)	{																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																												

		framecount = !framecount;
		printf("[Client] - I will match received ACK with this framecount %d \n", framecount);

		if((recvfrom(socket_desc_client, packet.ackno, 1, 0, (struct sockaddr *)&server, &addrlen))>=0){
		packet.ACK = atoi(packet.ackno);
		printf("[Client] - Expected ACK %d \n", framecount);
		printf("[Server response] - Received ACK %d \n", packet.ACK);
			if(framecount == packet.ACK){
			printf("[Client] - Server received data successfully \n");
			printf("[Client] - ---------END of CLIENT PACKET--------\n\n");
			}
			else{
			printf("\n[Client] - Send data again to the server \n ------ \n");
			memset(packet.sendingpkt, 0 , f_block_sz);
			framecount = !framecount; //Retransmit old Msg seq no.
			printf("[Client] - Msg no. for next frame...same as what sent earlier %d \n", framecount);
			printf("[Client] - ---------END of CLIENT PACKET--------\n\n");
			send_data();
			}
		}
		else{
		perror("[Client] - Receive failed %m \n");
		return 0;
		}
	}
	else{ 
	perror("[Client] - Sendto failed %m \n");
	return 0;
	}
}

while(1){
if( feof(pf_read) ){ 
	break ;
	}
f_block_sz = fread(sdbuf, sizeof(char), LENGTH, pf_read);
send_data();
memset(packet.sendingpkt, '\0' , 100);
}
fclose(pf_read);
close(socket_desc_client);
return 0;
}