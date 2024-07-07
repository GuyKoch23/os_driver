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
  char* message;

  if(argc != 4){
    printf("Number of argument not fit\n");
    exit(-1);
  }

  file_path = argv[1];
  channel_id = atoi(argv[2]);
  message = argv[3];

  file_desc = open( file_path, O_RDWR );
  if( file_desc < 0 ) {
    printf("Can't open device file: %s\n", file_path);
    exit(-1);
  }

  ret_val = ioctl(file_desc, MSG_SLOT_CHANNEL, channel_id);
  printf("sending to %d\n", channel_id);
  printf("writing %s\n", message);
  ret_val = write(file_desc, message, strlen(message)); 
  if(ret_val == -1){
    printf("There was a writing problem %s\n", strerror);
    exit(1);
  }
  printf("wrote %d\n", ret_val);
  close(file_desc); 
  exit(0);
}
