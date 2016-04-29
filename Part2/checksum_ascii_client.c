#include <stdio.h>
int checksum_ascii_client(char chunk[], size_t f_block_sz, int chksumgeneratorsize)
{
int i =0, sum = 0, chksum = 0;

while(f_block_sz > 0){
    sum = sum + chunk[i];
    i++;
    f_block_sz--;
}
chksum = sum % chksumgeneratorsize;
printf("[Client] - Generated chksum = %d \n", chksum);
return chksum;
}
