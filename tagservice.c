#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kern_levels.h>
//kmalloc declaration
#include <linux/slab.h>
//header  for kmalloc flags
#include <linux/gfp.h>
//copying from-to user
#include <linux/uaccess.h>
//spinlocks
#include <linux/spinlock.h>
//mutex
#include <linux/mutex.h>
//uid functions
#include <linux/cred.h>
#include <linux/uidgid.h>
//versions checks
#include <linux/version.h>
//syscall definition
#include <linux/syscalls.h>
//rcu_read_lock
#include <linux/rcupdate.h>
//semaphore used in tag_service entries
#include <linux/semaphore.h>
//wait event queue
#include <linux/wait.h>
//jiffies used during tag_receive Testing
#include <linux/jiffies.h>
//tty used to print warning message
#include <linux/tty.h>
//usedto acquire TCB from pid
#include <linux/pid.h>
//driver registration
#include <linux/fs.h>
//strlen usage
#include <linux/string.h>
//vmalloc used for driver operations
#include <linux/vmalloc.h>

#include "include/architecture.h"
#include "include/security.h"
#include "include/utilities.h"
#include "include/syscalls/tag_get.h"
#include "include/syscalls/tag_ctl.h"
#include "include/syscalls/tag_receive.h"
#include "include/syscalls/tag_send.h"
#include "include/tag_driver.h"

#define MODNAME "TAG Service"

MODULE_AUTHOR("Ezio Emanuele Ditella");
MODULE_DESCRIPTION("TAG Service Data Exchange, a detailed description of \
					this module can be found at: https://github.com/x-Ultra/TAG-Based-DE");

//exporting syscall used to add/remove syscall
extern int syscall_remover(int syscall_indx);
extern int syscall_adder(void* syscall_addr, char* syscall_name, int syscall_name_length, int syscall_num_params);

//syscall indexes in syscall table
int tag_get_indx, tag_ctl_indx, tag_send_indx, tag_receive_indx;
static int Major;
dev_t devNo;

static int __init install(void)
{
	int i, temp;

	//init static data
	for(i = 0; i < TBL_ENTRIES_NUM; i++){
		tag_table[i] = NULL;
		used_keys[i] = -1;
		sema_init(&semaphores[i], 1);
	}

	//setting PRIV_TAG_BITS variable value
	i = 0;
	temp = TBL_ENTRIES_NUM;
	while(temp != 0){
		temp = temp >> 1;
		i += 1;
	}
	//PRIV_TAG_BITS has to have a minimum value to prevent easy bruteforce attack
	PRIV_PWD_BITS = (sizeof(unsigned long)*8 - i);
	if(PRIV_PWD_BITS <= 0){
		printk(KERN_ERR "%s: Invalid value of PRIV_PWD_BITS, please decrease TBL_ENTRIES_NUM value", MODNAME);
		return -1;
	}

	//variable used to make positive passwords (check merge_rnd_descriptor in tag_get)
    for(i = 0; i < sizeof(int)*8-2; i++){
        positron |= positron << 1;
    }

	//adding Systemcalls
	printk(KERN_DEBUG "%s: Adding %s", MODNAME, "tag_get");
	if((tag_get_indx = syscall_adder((void *)sys_tag_get, "tag_get", 7, 3)) == -1){
		printk(KERN_ERR "%s: Unable to add tag_get", MODNAME);
		return -1;
	}

	printk(KERN_DEBUG "%s: Adding %s", MODNAME, "tag_ctl");
	if((tag_ctl_indx = syscall_adder((void *)sys_tag_ctl, "tag_ctl", 7, 2)) == -1){
		printk(KERN_ERR "%s: Unable to add tag_ctl", MODNAME);
		return -1;
	}

	printk(KERN_DEBUG "%s: Adding %s", MODNAME, "tag_send");
	if((tag_send_indx = syscall_adder((void *)sys_tag_send, "tag_send", 8, 4)) == -1){
		printk(KERN_ERR "%s: Unable to add tag_send", MODNAME);
		return -1;
	}

	printk(KERN_DEBUG "%s: Adding %s", MODNAME, "tag_receive");
	if((tag_receive_indx = syscall_adder((void *)sys_tag_receive, "tag_receive", 11, 4)) == -1){
		printk(KERN_ERR "%s: Unable to add tag_receive", MODNAME);
		return -1;
	}

	//number of starting page size kept for tag service stat message
	starting_pages = STAT_PAGES;

	//defining once and for all the template stat line len
	STAT_LINE_LEN = strlen(DRIVER_STAT_LINE)+1;
	if((tag_service_stat = vmalloc(PAGE_SIZE*STAT_PAGES)) == NULL){
		printk("%s: vmalloc failed", MODNAME);
  	  	return -1;
	}
	TIT_STAT_LINE_LEN = strlen(DRIVER_STAT_TIT)+1;

	//char device registration
	Major = register_chrdev(0, DEVICE_NAME, &fops);
	printk(KERN_NOTICE "%s: Major for device: %d", MODNAME, Major);


	printk(KERN_INFO "%s: Module inserted correctly", MODNAME);
	return 0;
}

static void __exit uninstall(void)
{
	int ret = 0, i;

	printk(KERN_DEBUG "%s: 1", MODNAME);

	//if tag table is still used, abort remotion
	for(i = 0; i < TBL_ENTRIES_NUM; i++){
		if(tag_table[i] == NULL)
			continue;

		if(!down_trylock(&semaphores[i])){
	        //if i am able to acquire 2 time the semaphore,
	        //some thread has the tag service.
	        if(!down_trylock(&semaphores[i])){
	            up(&semaphores[i]);
	            up(&semaphores[i]);
	            printk(KERN_ERR "%s: There are still threads on descriptor %d, try later", MODNAME, i);
				ret = -1;
				continue;
	        }
	    }else{
	        //if i am not able to acquire the semaphore for the first time,
	        //another removing ops for this service has been called, wait for it
	        down(&semaphores[i]);
	    }
		if(tag_table[i]->tag_levels != NULL){
			printk(KERN_ERR "%s: There are still threads on descriptor %d, try later", MODNAME, i);
			ret = -1;
		}
	}
	if(ret == -1)
		return;

	//removing systemcalls
	if(syscall_remover(tag_get_indx) == -1){
		printk(KERN_DEBUG "%s: Unable to remove tag_get", MODNAME);
		ret = -1;
	}

	if(syscall_remover(tag_ctl_indx) == -1){
		printk(KERN_DEBUG "%s: Unable to remove tag_ctl", MODNAME);
		ret = -1;
	}

	if(syscall_remover(tag_send_indx) == -1){
		printk(KERN_DEBUG "%s: Unable to remove tag_send", MODNAME);
		ret = -1;
	}

	if(syscall_remover(tag_receive_indx) == -1){
		printk(KERN_DEBUG "%s: Unable to remove tag_receive", MODNAME);
		ret = -1;
	}

	unregister_chrdev(Major, DEVICE_NAME);

	if(tag_service_stat != NULL)
		vfree(tag_service_stat);

	if(ret != -1){
		printk(KERN_DEBUG "%s: All systemcalls has been removed correctly", MODNAME);
	}else{
		printk(KERN_ALERT "%s: Some systemcalls has not been removed", MODNAME);
	}

}



module_init(install)
module_exit(uninstall)
MODULE_LICENSE("GPL");
