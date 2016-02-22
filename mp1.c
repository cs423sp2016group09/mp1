#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h> 
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/semaphore.h>

#include "mp1_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1
static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_entry;
static struct workqueue_struct *my_wq = 0;

static unsigned long onesec;
#define FILENAME "status"
#define DIRECTORY "mp1"

/**
 * Code to setup the linked list
 */
typedef struct node
{
    struct list_head list;
    int pid;
    long unsigned cputime;
} list_node;

static LIST_HEAD(head);
DEFINE_SEMAPHORE(sem_list);

/**
 * Work queue helper function
 */ 
static void wq_fun(struct work_struct *work)
{
    int ret;
    int count;
    count=0;
    struct node *i;
    list_for_each_entry(i, &head, list) {
        // TODO: LOCK
        down(&sem_list);
        ret = get_cpu_use(i->pid, &(i->cputime));
        if (ret == -1) {
            printk(KERN_ALERT "pnf, pid %d\n", i->pid);    
        } else {
            printk(KERN_ALERT "ud succ, pid %d, cpu: %d\n ", i->pid, i->cputime); 
        }
        up(&sem_list);
        // TODO: UNLOCK
        count++;
    }
    
    // printk(KERN_ALERT "WENT THROUGH %d LIST NODES\n", count);
    return;
}
DECLARE_WORK(wq, wq_fun);

/* setup timer */
static struct timer_list myTimer;

void timerFun (unsigned long arg) {
    myTimer.expires = jiffies + 5*HZ;
    add_timer (&myTimer); /* setup the timer again */
    schedule_work(&wq);
}


static int finished_writing;

static ssize_t mp1_read (struct file *file, char __user *buffer, size_t count, loff_t *data){
    // implementation goes here...
    int copied;
    char * buf;
    char * line;
    int linelength;
    char * bufpointer;

    struct node *i;
    // buf = (char *) kmalloc(count,GFP_KERNEL);
    int char_count;

    if (finished_writing == 1) {
      finished_writing = 0;
      return 0;
    } 

    char_count = 1600;

    copied = 0;
    //... put something into the buf, updated copied 

    // printk(KERN_ALERT "BEFORE THE FOR EACH");
    buf = (char *) kmalloc(char_count,GFP_KERNEL);
    memset(buf, 0, char_count);
    bufpointer = buf;
    list_for_each_entry(i, &head, list) {
        line = kmalloc(80, GFP_KERNEL);
        memset(line, 0, 80);
        down(&sem_list);
        sprintf(line, "PID: %d, time: %lu\n", i->pid, i->cputime);
        up(&sem_list);
        linelength = strlen(line) + 1;

        snprintf(bufpointer, linelength, "%s", line);
        bufpointer += linelength;
        // printk(KERN_ALERT "attempting to copy %d bytes\n", char_count);
        // printk(KERN_ALERT "attempting to copy string: %s\n", buf);
        // printk(KERN_ALERT "INSIDE FOR EACH");
        // printk(KERN_ALERT "pid %d", i->pid);
        kfree(line);
    }
    copy_to_user(buffer, buf, char_count);
    finished_writing = 1;
    // printk(KERN_ALERT "AFTER THE FOR EACH");

    // while (num_failed_to_copy > 0) {
      // printk(KERN_ALERT "copy failed, %d bytes remain\n", num_failed_to_copy);
    //   num_failed_to_copy = copy_to_user(buffer, buf, num_failed_to_copy);
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

    // printk(KERN_ALERT "this is the size of a char pointer in the kernel, %lu", sizeof (buf));

    // printk(KERN_ALERT "attempted to write string %s\n", buffer);
    // printk(KERN_ALERT "attempted to write %u bytes\n", (unsigned int) count);

    // printk(KERN_ALERT "attempted to allocate %u bytes\n", (unsigned int) count);   
    buf = (char *) kmalloc(count+1,GFP_KERNEL); 
    // printk(KERN_ALERT "allocated at address %p\n", buf);


    //... put something into the buf, updated copied 
    copied = copy_from_user(buf, buffer, count);
    buf[count]=0;

    // printk(KERN_ALERT "this is the return value: %u", copied);
    // printk(KERN_ALERT "received this string: %s", buf);

    // get pid from char *
    kstrtoint(buf, 10, &pid);

    // printk(KERN_ALERT "read pid = %d", pid);
    // find the node that corresponds to this list node

    new_node = kmalloc(sizeof(list_node), GFP_KERNEL);
    memset(new_node, 0, sizeof(list_node));
    new_node->pid = pid;
    down(&sem_list);
    list_add(&(new_node->list), &head);
    up(&sem_list);
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
    // printk(KERN_ALERT "MP1 MODULE LOADING\n");
    #endif

    proc_dir = proc_mkdir(DIRECTORY, NULL);
    proc_entry = proc_create(FILENAME, 0666, proc_dir, & mp1_file); 

  
    currentTime = jiffies; 
    expiryTime = currentTime + 5*HZ; 
    /* pre-defined kernel variable jiffies gives current value of ticks */

    init_timer (&myTimer);
    myTimer.function = timerFun;
    myTimer.expires = expiryTime;
    myTimer.data = 0;
    add_timer (&myTimer);
    // printk (KERN_INFO "timer added \n");
    //---------------------

    onesec = msecs_to_jiffies(10000);
    // printk(KERN_INFO "Mykmod loaded %u jiffies \n", (unsigned) onesec);

    // printk(KERN_ALERT "MP1 MODULE LOADED\n");
    return 0;   
}

// mp1_exit - Called when module is unloaded
void __exit mp1_exit(void)
{
    #ifdef DEBUG
    // printk(KERN_ALERT "MP1 MODULE UNLOADING\n");
    #endif
    //-------------------------

    if (!del_timer (&myTimer)) {
        // printk (KERN_INFO "Couldn't remove timer!!\n");
    }
    else {
        // printk (KERN_INFO "timer removed \n");
    }
    if (my_wq)
        destroy_workqueue(my_wq);
    // printk(KERN_INFO "Mykmod exit\n");
    proc_remove(proc_entry);
    proc_remove(proc_dir);

    // printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
