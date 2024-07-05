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
    int channel_id;
    char *buffer;

    if(argc != 3){
        printf("Number of argument not fit\n"); 
        exit(-1);
    }

    file_path = argv[1];
    channel_id = atoi(argv[2]);

    file_desc = open( "/dev/"DEVICE_FILE_NAME, O_RDWR );
    if( file_desc < 0 ) {
        printf ("Can't open device file: %s\n", DEVICE_FILE_NAME);
        exit(-1);
    }
    buffer = (char*)malloc(sizeof(char));
    if(buffer == NULL){
        exit(-1);
    }
    ret_val = ioctl( file_desc, MSG_SLOT_CHANNEL, channel_id);
    ret_val = read( file_desc, buffer, BUF_LEN); 
    printf("reading from channel %d\n", channel_id);
    if(ret_val == -1){
        printf("There was a reading problem %s", strerror);
        exit(1);
    }
    printf("%s\n", buffer);
    close(file_desc); 
    free(buffer);
    exit(0);
}
