#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>
MODULE_LICENSE("GPL");
#include "message_slot.h"


//================== LinkedList FUNCTIONS ===========================

struct ChannelNode {
  char *the_message;
  unsigned int channel; 
  int current_message_length;
  struct ChannelNode *next;
};

struct SlotNode{
  int minor;
  struct ChannelNode *headChannelNode;
  struct SlotNode *next;
};

struct SlotNode *list_head = NULL;

int insert_channel_node(struct SlotNode *slotNode, unsigned int channel){
  struct ChannelNode *channelNode = kmalloc(sizeof(struct ChannelNode), GFP_KERNEL);
  if(channelNode == NULL){
    printk(KERN_ERR "Error while trying to allocate channel node");
    return 1;
  }
  channelNode->channel = channel;
  channelNode->current_message_length = 0;
  channelNode->the_message = (char*)kmalloc(sizeof(char)*BUF_LEN, GFP_KERNEL);
  if(channelNode->the_message == NULL){
    
    kfree(channelNode);
    return 1;
  }
  channelNode->next = slotNode->headChannelNode;
  slotNode->headChannelNode = channelNode;
  return 0;
};

struct ChannelNode* create_zero_channel_node(void){
  struct ChannelNode *channelNode = kmalloc(sizeof(struct ChannelNode), GFP_KERNEL);
  if(channelNode == NULL){
    printk(KERN_ERR "Error while trying to allocate channel node");
    return NULL;
  }
  channelNode->channel = 0;
  channelNode->next = NULL;
  channelNode->current_message_length = 0;
  channelNode->the_message = NULL;
  return channelNode;
}

void initialize_ds_for_minor(int minor){
  struct SlotNode *slotNode = kmalloc(sizeof(struct SlotNode), GFP_KERNEL);
  if(slotNode == NULL){
    printk(KERN_ERR "Error while trying to allocate slot node");
    return;
  }
  slotNode->minor = minor;
  slotNode->headChannelNode = create_zero_channel_node();
  slotNode->next = list_head;
  list_head = slotNode;
};

struct SlotNode* get_slot_node(int minor){
    struct SlotNode *slotNode = list_head;
    while(slotNode != NULL){
      if(slotNode->minor == minor){
        return slotNode;
      }
      slotNode = slotNode->next;
    }
    initialize_ds_for_minor(minor);
    return list_head;
}

  int is_slot_initialized(int minor){
    struct SlotNode *slotNode = list_head;
    while(slotNode != NULL){
      if(slotNode->minor == minor){
        return 1;
      }
      slotNode = slotNode->next;
    }
    return 0;
};

struct ChannelNode* get_channel_node(int minor, unsigned int channel){
  struct SlotNode* slotNode = get_slot_node(minor);
  struct ChannelNode *channelNode = slotNode->headChannelNode;
  int result;
  while(channelNode->channel != 0){
    if(channelNode->channel == channel){
      return channelNode;
    }
    channelNode = channelNode->next;
  }

  result = insert_channel_node(slotNode, channel);
  if(result == 1){
    return NULL;
  }
  return slotNode->headChannelNode;
};

//================== DEVICE FUNCTIONS ===========================

static int device_open( struct inode* inode,
                        struct file*  file )
{
  int minor;
  printk("Invoking device_open(%p)\n", file);
  //DataStructure already initialized for devide file
  minor = iminor(inode);
  if(is_slot_initialized(minor) == 0){
    initialize_ds_for_minor(minor); // if the ds does not initialized, initializes it
  }
  return SUCCESS;
}

static int device_release( struct inode* inode,
                           struct file*  file)
{
  printk("Invoking device_release(%p,%p)\n", inode, file);
  return SUCCESS;
}

static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
    ssize_t i;
    int minor;
    struct ChannelNode *channelNode;
    char* tempBuffer;
    long channel;

    if(buffer == NULL){ // supplied buffer is not appropriate
      return -EINVAL;
    }
    channel = (unsigned long)(file->private_data); // extracting the current channel number
    if(channel == 0){ // zero channel is not valid
        return -EINVAL;
    }
    minor = iminor(file->f_inode);

    channelNode = get_channel_node(minor, (int)(uintptr_t)file->private_data); // getting the relevant Channel Node for reading from
    if(channelNode == NULL){ // alocation problem
      return -ENOMEM;
    }
    if(channelNode->current_message_length == 0){ // message length is 0 
      return -EWOULDBLOCK;
    }
    if(length < channelNode->current_message_length){ // no enough space in buffer for message
      return -ENOSPC;
    }
    printk("Invoking device_read");
    
    tempBuffer = (char*)kmalloc(sizeof(char)*channelNode->current_message_length, GFP_KERNEL); // temp buffer for atomicity of read process
    if(tempBuffer == NULL){
      return -ENOMEM;
    }

    for(i = 0; i< channelNode->current_message_length; i++){
      tempBuffer[i] = channelNode->the_message[i];
    }

    for(i = 0; i < channelNode->current_message_length && i < BUF_LEN; ++i) {
        if(put_user(tempBuffer[i], &buffer[i]) != 0){ // checking for read success
          return -EFAULT;
        }
    }
    
    kfree(tempBuffer);
    return i; // returns number of read bytes
}

//---------------------------------------------------------------
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
    ssize_t i;
    int minor;
    struct ChannelNode *channelNode;
    char* tempBuffer;
    long channel;
    if(buffer == NULL){
      return -EINVAL;
    }
    channel = (unsigned long)(file->private_data); // extracting the current channel number
    if(channel == 0){
        return -EINVAL;
    }
    if(length == 0 || length > BUF_LEN){
        return -EMSGSIZE;
    }
    minor = iminor(file->f_inode);

    channelNode = get_channel_node(minor, (int)(uintptr_t)file->private_data);// getting the relevant Channel Node for writing to
      if(channelNode == NULL){
      return -ENOMEM;
    }
    printk("Invoking device_write(%p,%ld)\n", file, length);

    tempBuffer = (char*)kmalloc(sizeof(char)*length, GFP_KERNEL); // temp buffer for atomicity of write process
    if(tempBuffer == NULL){
      return -ENOMEM;
    }

    for(i = 0; i < length; ++i){
      if(get_user(tempBuffer[i], &buffer[i]) != 0){ // checking for read success
        return -EFAULT;
      }
    }

    for(i = 0; i < length; ++i) {
         channelNode->the_message[i] = tempBuffer[i];
    }

    channelNode->current_message_length = i;
    kfree(tempBuffer);
    return i;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
  if( MSG_SLOT_CHANNEL != ioctl_command_id || ioctl_param == 0) {
    return -EINVAL;
  }
  file->private_data = (void*)ioctl_param; // assigning the channel number to device file
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
static int __init message_slot_init(void)
{
  int rc = -1;
  rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);
  if( rc < 0 ) {
    printk(KERN_ERR "registraion failed for  %d\n", MAJOR_NUM);
    return rc;
  }
  printk("Registeration is successful.\n");
  return 0;
}

//---------------------------------------------------------------
static void __exit message_slot_cleanup(void)
{
  struct SlotNode* slotNode;
  struct SlotNode* prevSlotNode;

  struct ChannelNode* channelNode;
  struct ChannelNode* prevChannelNode;

  slotNode = list_head;

  while(slotNode != NULL){ // freeing the Linked List
    prevSlotNode = slotNode;
    channelNode = slotNode->headChannelNode;
    while(channelNode != NULL){
      prevChannelNode = channelNode;
      channelNode = channelNode->next;
      kfree(prevChannelNode);
      prevChannelNode = channelNode;
    }
    slotNode = slotNode->next;
    kfree(prevSlotNode);
    prevSlotNode = slotNode;
  }
  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(message_slot_init);
module_exit(message_slot_cleanup);

//========================= END OF FILE =========================
