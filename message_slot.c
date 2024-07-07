// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>
// #include <errno.h> // handle this

MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations
#include "message_slot.h"

struct chardev_info {
  spinlock_t lock;
};

// used to prevent concurent access into the same device
static int dev_open_flag = 0;

static struct chardev_info device_info;

// The message the device will give when asked
//static char the_message[BUF_LEN];
//static int current_message_length = 0;
//tatic int channel_id = 0;


//================== LinkedList FUNCTIONS ===========================

struct ChannelNode {
  char *the_message;
  int channel; 
  int current_message_length;
  struct ChannelNode *next;
};

//---------------------------------------------------------------
struct ChannelNode *channel = NULL;
struct ChannelNode *list_head = NULL;
//---------------------------------------------------------------

void InsertChannelNode(int channel){
  struct ChannelNode *channelNode = kmalloc(sizeof(struct ChannelNode), GFP_KERNEL);
  if(channelNode == NULL){
    printk(KERN_ERR "Error while trying to allocate channel node");
    return;
  }
  channelNode->channel = channel;
  channelNode->next = list_head;
  channelNode->current_message_length = 0;
  channelNode->the_message = (char*)kmalloc(sizeof(char)*BUF_LEN, GFP_KERNEL);
  list_head = channelNode;
};

//---------------------------------------------------------------

struct ChannelNode* get_channel_node(int channel){
  struct ChannelNode *channelNode = list_head;
  while(channelNode->channel != 0){
    if(channelNode->channel == channel){
      return channelNode;
    }
    channelNode = channelNode->next;
  }

  InsertChannelNode(channel);
  return list_head;
};

//---------------------------------------------------------------


//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
  unsigned long flags; // for spinlock
  printk("Invoking device_open(%p)\n", file);

  // We don't want to talk to two processes at the same time
  spin_lock_irqsave(&device_info.lock, flags);
  if( 1 == dev_open_flag ) {
    spin_unlock_irqrestore(&device_info.lock, flags);
    return -EBUSY;
  }

  ++dev_open_flag;
  spin_unlock_irqrestore(&device_info.lock, flags);
  return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
  unsigned long flags; // for spinlock
  printk("Invoking device_release(%p,%p)\n", inode, file);

  // ready for our next caller
  spin_lock_irqsave(&device_info.lock, flags);
  --dev_open_flag;
  spin_unlock_irqrestore(&device_info.lock, flags);
  return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
    ssize_t i;
    if(channel->channel == 0){
        //errno = EINVAL;
        return -1;
    }
    // if(buffer[0] = NULL){ //  handle this
    //     //errno = EWOULDBLOCK;
    //     return -1;
    // }
    printk("%ld %d", length, channel->current_message_length);
    if(length < channel->current_message_length){
        //errno = ENOSPC;
        return -1;
    }
    //any other error
    printk("Invocing device_read");
    for(i = 0; i < channel->current_message_length && i < BUF_LEN; ++i) {
        put_user(channel->the_message[i], &buffer[i]);
        printk("%c\n", channel->the_message[i]);
    }
    printk("%s", buffer);
    return i; // return number of read bytes
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
    ssize_t i;
    if(channel->channel == 0){
        //errno = EINVAL;
        return -1;
    }
    if(length == 0 || length > 128){
        //errno = EMSGSIZE;
        return -1;
    }
    // any other error: -1, errno = what suits

    printk("Invoking device_write(%p,%ld)\n", file, length);
    for(i = 0; i < length && i < BUF_LEN; ++i) {
        get_user(channel->the_message[i], &buffer[i]);
    }
    printk("message current is %s\n", channel->the_message);
    channel->current_message_length = i;
    return i;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
  // Switch according to the ioctl called
  if( MSG_SLOT_CHANNEL != ioctl_command_id || ioctl_param == 0) {
    //errno = EINVAL;
    return -1;
  }
  channel = get_channel_node(ioctl_param);
  return SUCCESS;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops = {
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .release        = device_release,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
  int rc = -1;
  // init dev struct
  memset( &device_info, 0, sizeof(struct chardev_info) );
  spin_lock_init( &device_info.lock );

  // Register driver capabilities. Obtain major num
  rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

  // Negative values signify an error
  if( rc < 0 ) {
    printk( KERN_ERR "registraion failed for  %d\n", MAJOR_NUM );
    return rc;
  }

  channel = kmalloc(sizeof(struct ChannelNode), GFP_KERNEL);
  channel->channel = 0;
  list_head = channel;

  printk("channel number is %d", channel->channel);

  printk( "Registeration is successful. ");
  printk( "If you want to talk to the device driver,\n" );
  printk( "you have to create a device file:\n" );
  printk( "mknod /dev/%s c %d 0\n", "device file name", MAJOR_NUM );
  printk( "You can echo/cat to/from the device file.\n" );
  printk( "Dont forget to rm the device file and "
          "rmmod when you're done\n" );

  return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
  // Unregister the device
  // Should always succeed
  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
