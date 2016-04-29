#include<stdio.h>
#include<string.h>
#include <sys/types.h>  
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include <unistd.h>
 
int main(int argc , char *argv[])
{
    int socket_desc_server, socket_accepted, client_address_size, LENGTH = 5000, j;
    struct sockaddr_in server ;
    char fname[512];

    //Create socket
    socket_desc_server = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc_server == -1)
    {
        printf("Could not create socket");
    }
     
    //Prepare the sockaddr_in server structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 30000 );
     
    //Bind
    if( bind(socket_desc_server,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        puts("bind failed");
	return 1;
    }
    puts("bind done");
     
    //Listen
    if(listen(socket_desc_server , 5) < 0)
    {
	puts("Listening failed");
    }
    puts("Socket listening..Waiting for incoming connections...");

    //Accept and incoming connection
    client_address_size = sizeof(server);
    socket_accepted = accept(socket_desc_server, (struct sockaddr *)&server, (socklen_t *)&client_address_size);
    if (socket_accepted<0)
    {
        perror("accept failed");
	return 1;
    }
    puts("Connection accepted");

//Receive file name and later write into it
	ssize_t filename_size = recv(socket_accepted, fname, 10, 0);
	if (filename_size<0)
	{
	printf("File name not recieved \n");
	return 1;	
	}
	else
	fname[filename_size] = '\0';
	printf("End of the actual file name may have been stripped off, Received file name is %s \n", fname);

FILE *pf_write;
int i = 0, count = 0, a;
size_t recv_len = 5; //Receive data length, can be changed
char rcvbuf[LENGTH]; // Receive buffer
char* f_name = fname;
bzero(rcvbuf, LENGTH);
int f_block_sz;
	pf_write = fopen(f_name, "ab+");

	printf("Data Received on socket, now saving it...\n");
	do
	{
	a = recv(socket_accepted, rcvbuf + recv_len*i, recv_len, 0);
	int write_sz = fwrite(rcvbuf + recv_len*i, sizeof(char), a, pf_write);
	i=i+1;
	count = count + a;
	}while(a>0);

	if(count<0)
	{
	printf("Received file error.\n");
	return 0;
	}
fclose(pf_write);
puts("Data Received Successfully\n");
close(socket_accepted);
return 0;
}
