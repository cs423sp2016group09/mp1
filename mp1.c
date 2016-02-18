#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h> 
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>

#include "mp1_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1

#ifndef LIST_NODE_H
#define LIST_NODE_H 

typedef struct node
{
    struct list_head list;
    int pid;
    int cputime;
} list_node;
#endif


static LIST_HEAD(head);

#define FILENAME "status"
#define DIRECTORY "mp1"


static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_entry;
static struct workqueue_struct *wq = 0;

static unsigned long onesec;


static void mykmod_work_handler (struct work_struct *W){
    printk(KERN_INFO "Mykmod work %u jiffies\n", (unsigned) onesec);
}
/* setup timer */
static struct timer_list myTimer;

static DECLARE_DELAYED_WORK(mykmod_work, mykmod_work_handler);
void timerFun (unsigned long arg) {
    printk (KERN_INFO "TIMER WENT OFF \n"); 
    myTimer.expires = jiffies + 5*HZ;
    add_timer (&myTimer); /* setup the timer again */
}


static int num_failed_to_copy;
static int finished_writing;

static ssize_t mp1_read (struct file *file, char __user *buffer, size_t count, loff_t *data){
   // implementation goes here...
   int copied;
   char * buf;

   struct node *i;
   // buf = (char *) kmalloc(count,GFP_KERNEL);
   int char_count;
   
   if (finished_writing == 1) {
      finished_writing = 0;
      return 0;
   } 

   char_count = 3;
   buf = (char *) kmalloc(char_count,GFP_KERNEL); 
   buf[0] = 'H';
   buf[1] = 'I';
   buf[2] = '\0';

   copied = 0;
   //... put something into the buf, updated copied 
   printk(KERN_ALERT "attempting to copy %d bytes\n", char_count);
   printk(KERN_ALERT "attempting to copy string: %s\n", buf);

   // first time we're copying this
   num_failed_to_copy = copy_to_user(buffer, buf, char_count);

   if (num_failed_to_copy == 0) {
      finished_writing = 1;
   }

   printk(KERN_ALERT "BEFORE THE FOR EACH");
   list_for_each_entry(i, &head, list) {
      printk(KERN_ALERT "INSIDE FOR EACH");
      printk(KERN_ALERT "pid %d", i->pid);
   }

   printk(KERN_ALERT "AFTER THE FOR EACH");

   // while (num_failed_to_copy > 0) {
   //    printk(KERN_ALERT "copy failed, %d bytes remain\n", num_failed_to_copy);
   //    num_failed_to_copy = copy_to_user(buffer, buf, num_failed_to_copy);
   // }
   // printk(KERN_ALERT "copy done, sending EOF\n");
   // copy_to_user(buffer, buf, 0);
   kfree(buf);
   return char_count;
}

static ssize_t mp1_write (struct file *file, const char __user *buffer, size_t count, loff_t *data){ 
   // implementation goes here...
   int copied;
   char *buf;
   list_node *new_node;
   int pid;


   printk(KERN_ALERT "this is the size of a char pointer in the kernel, %lu", sizeof (buf));

   printk(KERN_ALERT "attempted to write string %s\n", buffer);
   printk(KERN_ALERT "attempted to write %u bytes\n", (unsigned int) count);

   printk(KERN_ALERT "attempted to allocate %u bytes\n", (unsigned int) count);   
   buf = (char *) kmalloc(count+1,GFP_KERNEL); 
   printk(KERN_ALERT "allocated at address %p\n", buf);


   //... put something into the buf, updated copied 
   copied = copy_from_user(buf, buffer, count);
   buf[count]=0;

   printk(KERN_ALERT "this is the return value: %u", copied);
   // printk(KERN_ALERT "received this string: %s", buf);

   // get pid from char *
   kstrtoint(buf, 10, &pid);

   printk(KERN_ALERT "read pid = %d", pid);
   // find the node that corresponds to this list node

   new_node = kmalloc(sizeof(list_node), GFP_KERNEL);
   memset(new_node, 0, sizeof(list_node));
   new_node->pid = pid;
   list_add(&(new_node->list), &head);

   // if it doesn't exist, create it


   kfree(buf);
   return count;
}

static const struct file_operations mp1_file = {
    .owner = THIS_MODULE, 
    .read = mp1_read,
    .write = mp1_write,
};

// mp1_init - Called when module is loaded
int __init mp1_init(void)
{
    unsigned long currentTime;
    unsigned long expiryTime;

    #ifdef DEBUG
    printk(KERN_ALERT "MP1 MODULE LOADING\n");
    #endif

    currentTime = jiffies; 
    expiryTime = currentTime + 5*HZ; /* HZ gives number of ticks per second */
    // Insert your code here ...

      
    proc_dir = proc_mkdir(DIRECTORY, NULL);
    proc_entry = proc_create(FILENAME, 0666, proc_dir, & mp1_file); 


    /* pre-defined kernel variable jiffies gives current value of ticks */

    init_timer (&myTimer);
    myTimer.function = timerFun;
    myTimer.expires = expiryTime;
    myTimer.data = 0;

    add_timer (&myTimer);
    printk (KERN_INFO "timer added \n");
    //---------------------

    onesec = msecs_to_jiffies(10000);
    printk(KERN_INFO "Mykmod loaded %u jiffies \n", (unsigned) onesec);

    if (!wq)
            wq = create_workqueue("mykmod");
    if (wq)
        queue_delayed_work(wq,&mykmod_work,onesec);



    printk(KERN_ALERT "MP1 MODULE LOADED\n");
    return 0;   
}

// mp1_exit - Called when module is unloaded
void __exit mp1_exit(void)
{
    #ifdef DEBUG
    printk(KERN_ALERT "MP1 MODULE UNLOADING\n");
    #endif
    //-------------------------

    if (!del_timer (&myTimer)) {
        printk (KERN_INFO "Couldn't remove timer!!\n");
    }
    else {
        printk (KERN_INFO "timer removed \n");
    }
    if (wq)
        destroy_workqueue(wq);
    printk(KERN_INFO "Mykmod exit\n");
    proc_remove(proc_entry);
    proc_remove(proc_dir);

    printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
