#include "message_slot.h"    
#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int file_desc;
    int ret_val;
    char* file_path;
    unsigned int channel_id;
    char *buffer;

    if(argc != 3){
        printf("Number of args not fit %s\n", strerror(errno));
        exit(1);
    }

    file_path = argv[1];
    channel_id = atoi(argv[2]);

    file_desc = open( file_path, O_RDWR );
    if( file_desc < 0 ) {
        printf("Can't open device file %s\n", strerror(errno));
        exit(1);
    }

    buffer = (char*)malloc(BUF_LEN*sizeof(char));
    if(buffer == NULL){
        printf("Allocation problem %s\n", strerror(errno));
        exit(1);
    }

    ret_val = ioctl( file_desc, MSG_SLOT_CHANNEL, channel_id);
    if(ret_val == -1){
        printf("There was a IOCTL problem %s\n", strerror(errno));
        exit(1);
    }

    ret_val = read( file_desc, buffer, BUF_LEN); 
    if(ret_val == -1){
        printf("There was a writing problem %s\n", strerror(errno));
        exit(1);
    }

    close(file_desc); 

    ret_val = write(1, buffer, strlen(buffer));

    if(ret_val == -1){
        printf("There was a writing problem %s\n", strerror(errno));
        exit(1);
    }

    free(buffer);

    exit(0);
}
