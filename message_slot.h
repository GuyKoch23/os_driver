#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>

#define MAJOR_NUM 235
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)
#define DEVICE_RANGE_NAME "message_reader"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "simple_message_reader"
#define SUCCESS 0

#endif
