/*
compile - "gcc stop-wait_server.c -o stop-wait_server"
execute - "./stop-wait_server"
*/
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>
#include "checksum_ascii_server.c"
#define CHKSUMGENERATORSIZE 9
#define LENGTH 20
//#define TIMEOUT_SERVER 7
 
int main()
{
int socket_desc_server, socket_accepted, client_address_size, j, TIMEOUT_SERVER;
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
int i = 1, msgno, expctdframe = 0, ACK, chksum_computed, flag;
size_t len=1, max_len = 100, rcvdata_len; //Receive data length, can be changed
char rcvbuf_withmsgno[100], sendack[1], *temp; //Receive buffer
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
	printf("Enter value of SERVER TIMEOUT \n");
	scanf("%d",&TIMEOUT_SERVER);
	//TIMEOUT_SERVER = 2*(rand()%5);
	printf("Server sleep timer %d sec \n", TIMEOUT_SERVER);

//Exoected frame - but recieved good or corrupt
	if(expctdframe == msgno){
	usleep((TIMEOUT_SERVER)*1000000);
		printf("[Server] - Msg seq no. is fine \n");

//Successful case - Send msgno to sender
		if(chksum_computed == packet.rcvdchksum){
		ACK = !expctdframe;
		printf("[Server] - Data is fine as well...will send ACK %d \n", ACK);
		snprintf(sendack, 2,"%d", ACK);

//Simulate packet loss
	printf("[Server] - Do you want Simulate ACK loss in the medium(remember client timer still ticking in)? ---------> (1 / 0) : ");
	scanf("%d",&flag);
	
			if(flag == 1){
			printf("ACK lost in the medium...expect to receive same packet from client again, discard it next time as a duplicate \n");
			}

//Send msgno to sender
			else{
				if ((sendto(socket_desc_server, sendack, len, 0, (struct sockaddr *)&server, addrlen)) < 0){ 
				perror("[Server] - Reason2 msgno send failed %m \n");
				return 0;
				}
			}

	expctdframe = ACK;//Wait for next frame
	printf("[Server] - Expctd Msg seq no. in next packet %d \n", expctdframe);

//Skipping initial two values, since they belong to Msgseq no. and received checksum value and then writing into output file
	printf("[Server] - trying to writing data - %s \n", rcvbuf_withmsgno+2);

			if(pf_write == NULL){
			perror ("The following error occurred");
			return 0;
			}
//ACK lost or not it will get written in the file, if received again will be discarded as a duplicate
			else
			fwrite(rcvbuf_withmsgno+2, sizeof(char), rcvdata_len-2, pf_write);
			puts("[Server] - Write successful \n");
			}

//Checksum error - Recieved corrupt data
	else if(chksum_computed != packet.rcvdchksum){
	printf("[Server] - Corrupt data received \n");
	ACK = expctdframe;
	printf("[Server] - Sending Old ACK...expctd Msgseq no. for next packet %d \n", expctdframe);
	snprintf(sendack, 2,"%d", ACK);

//Simulate packet loss??
		printf("[Server] - Do you want Simulate ACK loss in the medium(remember client timer still ticking in)? ---------> (1 / 0) : ");
		scanf("%d",&flag);
			if(flag == 1){
			printf("ACK lost in the medium...\n");
			printf("Expecting for correct data next time \n");
			}

//Sending ACK to the client
			else{
				if ((sendto(socket_desc_server, sendack, len, 0, (struct sockaddr *)&server, addrlen)) < 0){ 
				perror("[Server] - Data is fine but msgno send failed %m \n");
				return 0;
			}
		}
	}
}

//Msgno. corrupted - send ACK = expctdframe back to client indicating expected frame; Data may be good or corrupt
	else if(expctdframe != msgno){
		ACK = expctdframe;
		//usleep((TIMEOUT_SERVER)*1000000);
		printf("Server sleep timer %d sec \n", TIMEOUT_SERVER);
		snprintf(sendack, 2,"%d", ACK);
		printf("[Server] - Wrong Msg no. \n");

//Checksum(Data) fine
		if(chksum_computed == packet.rcvdchksum)
		printf("[Server] - Data is fine \n");

//Data corrupt, but MSgno. fine...send data again. send ACK = expctdframe back to client indicating expected frame.
		else if(chksum_computed != packet.rcvdchksum)
		printf("[Server] - Data wrong as well \n");

//Simulate packet loss
		printf("[Server] - Do you want Simulate ACK loss in the medium(remember client timer still ticking in)? ---------> (1 / 0) : ");
		scanf("%d",&flag);
			if(flag == 1){
				printf("ACK lost in the medium...\n");
				printf("Expecting for same Msgseq no. again \n");
			}
			else{
				if ((sendto(socket_desc_server, sendack, len, 0, (struct sockaddr *)&server, addrlen)) < 0){ 
				perror("[Server] - Data is fine but msgno send failed %m \n");
				return 0;
		}
	}
printf("[Server] - Msgno. wrong, sending old ACK...%d \n client to resend packet \n", ACK);
}

//Any abnormal condition
	else{
	printf("[Server] - Multi bits flipped, cannot handle this");
	return 0;
	}

printf("****************\n");
printf("\n [Server] - I am sending ACK %d", ACK);
printf("\n [Server] - I will Expect next Msg seq no. to be %d \n", expctdframe);
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