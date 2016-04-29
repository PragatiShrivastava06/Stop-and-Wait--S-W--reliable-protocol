/*
Command to execute - "./sftp_client 127.0.0.1 30000 output.txt input.txt"
*/
#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/sendfile.h>
#include<inttypes.h>
#include<netinet/in.h>
#define LENGTH 5000 // Buffer length
#define BLOCK_SIZE 10 //Send Data in the order of 10 bytes

int main(int argc , char *argv[])
{
	if(argc < 0)
	{
	printf("Check number of arguments command should be like - './sftp_client 127.0.0.1 30000 output.txt input.txt' ");
	}

int socket_desc_client, new_socket;
struct sockaddr_in server;

//Create TCP Socket of type IPv4
    socket_desc_client = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc_client == -1)
    {
        printf("Could not create socket");
    }
    else
	printf("Socket created successfully \n");

//Convert IP address to long format
	server.sin_addr.s_addr = inet_addr(argv[1]);
	server.sin_family = AF_INET;
	long server_port = strtol(argv[2], (char **)NULL, 10);
	server.sin_port = htons(server_port);

//Connect socket
    new_socket = connect(socket_desc_client , (struct sockaddr *)&server , sizeof(server));
    if (new_socket < 0)
    {
	printf("errno message : %m\n");
        puts("connect error");
        return 1;
    }
printf("The connection was accepted by the server...\n");

// This part will send the filename
		if((send(socket_desc_client, argv[3], sizeof(argv[3]), 0))<0)
		{
		printf("Could not send file %m \n");
		return 1;
		}
		else
		printf("File name send to server i.e. %s \n", argv[3]);

// Read file and send it across the socket
FILE *pf_read;
pf_read = fopen(argv[4], "rb");
	if (pf_read == NULL)
	{
	printf("Read/Send File not found!\n");
	return 1;
	}
	printf("Read file - %s\n", argv[4]);

// Send buffer
	char sdbuf[LENGTH], chunk[LENGTH];
	int f_block_sz, i, j, m, n, a;
	f_block_sz = fread(sdbuf, sizeof(char), LENGTH, pf_read);
	printf("Block size is  %d \n", f_block_sz);

// Send data in chunks of 10 bytes
	if(f_block_sz <=BLOCK_SIZE){
		if((send(socket_desc_client, sdbuf, f_block_sz, 0))<0)
		{
		printf("Send failed %m \n");
		return 1;
		}
	bzero(sdbuf, LENGTH);
	}
	else if(f_block_sz >BLOCK_SIZE){
		    m = f_block_sz % BLOCK_SIZE;
		    n = f_block_sz - m;
		for(j=0;j<n;j=j+BLOCK_SIZE)
		{
		for(i=j;i<BLOCK_SIZE+j;i++)
		    {
			chunk[i]=sdbuf[i];
		    }
			if((send(socket_desc_client, chunk + j, BLOCK_SIZE, 0))<0)
			{
			printf("Send failed %m \n");
			return 1;
			}
		}
		a = m;
		while(m>0){
			chunk[j]=sdbuf[j]; //j =30
			m = m-1;
			j++;
			}
			if((send(socket_desc_client, chunk + n, a, 0))<0)
			{
			printf("Send failed %m \n");
			return 1;
			}
			puts("Data Send Successfully\n");
			bzero(chunk, LENGTH);
	}
close(socket_desc_client);
fclose(pf_read);
return 0;
}
