#include <stdio.h>
int checksum_ascii_server(char chunk[], size_t rcvdata_len, int chksumgeneratorsize)
{
int i =0, sum = 0, chksum = 0;

while(rcvdata_len>0){
    sum = sum + chunk[i];
    i++;
    rcvdata_len--;
}
chksum = sum % chksumgeneratorsize;
printf("[Server] - Computed chksum = %d \n", chksum);
return chksum;
}
