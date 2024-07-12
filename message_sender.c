#include "message_slot.h"    
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
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
  char* message;

  if(argc != 4){
    printf("Number of args not fit %s\n", strerror);
    exit(1);
  }

  file_path = argv[1];
  channel_id = atoi(argv[2]);
  message = argv[3];

  file_desc = open( file_path, O_RDWR );
  if( file_desc < 0 ) {
    printf("Can't open device file %s\n", strerror);
    exit(1);
  }

  ret_val = ioctl(file_desc, MSG_SLOT_CHANNEL, channel_id);
  if(ret_val == -1){
    printf("There was a IOCTL problem %s\n", strerror);
    exit(1);
  }

  ret_val = write(file_desc, message, strlen(message)); 
  if(ret_val == -1){
    printf("There was a writing problem %s\n", strerror);
    exit(1);
  }

  if(ret_val < strlen(message)){
    printf("There was a writing problem %s\n", strerror);
    exit(1);
  }

  close(file_desc); 

  exit(0);
}
