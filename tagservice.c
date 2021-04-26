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

#include "include/architecture.h"
#include "include/syscalls/tag_get.h"

#define MODNAME "TAG Service"

MODULE_AUTHOR("Ezio Emanuele Ditella");
MODULE_DESCRIPTION("TAG Service Data Exchange, a detailed description of \
					this module can be found at: https://github.com/x-Ultra/TAG-Based-DE");

//exporting syscall used to add/remove syscall
extern int syscall_remover(int syscall_indx);
extern int syscall_adder(void* syscall_addr, char* syscall_name, int syscall_name_length, int syscall_num_params);

//syscall indexes in syscall table
int tag_get_indx;

static int __init install(void)
{
	int i;

	//init static data
	for(i = 0; i < TBL_ENTRIES_NUM; i++){
		tag_table[i] = NULL;
		used_keys[i] = -1;
	}

	//adding Systemcalls
	printk(KERN_DEBUG "%s: Adding %s\n", MODNAME, "tag_get");
	if((tag_get_indx = syscall_adder(tag_get, "tag_get", 7, 3)) == -1){
		printk("%s: Unable to tag_get\n", MODNAME);
	}

	return -1;
}

static void __exit uninstall(void)
{
	int ret = 0;

	//TODO, if tag table not empty, fail
	//

	//removing systemcalls
	if(syscall_remover(tag_get_indx) == -1){
		printk(KERN_DEBUG "%s: Unable to remove tag_get_indx", MODNAME);
		ret = -1;
	}

	if(ret != -1){
		printk(KERN_DEBUG "%s: All systemcalls has been removed correctly", MODNAME);
	}else{
		printk(KERN_ALERT "%s: Some systemcalls has not been removed", MODNAME);
	}


}



module_init(install)
module_exit(uninstall)
MODULE_LICENSE("GPL");
