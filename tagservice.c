#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kern_levels.h>
//kmalloc declaration
#include <linux/slab.h>
//header  for kmalloc flags
#include <linux/gfp.h>
//copying from-to user
#include <linux/uaccess.h>

#define MODNAME "TAG Service"

MODULE_AUTHOR("Ezio Emanuele Ditella");
MODULE_DESCRIPTION("TAG Service Data Exchange, a detailed description of \
					this module can be found at: https://github.com/x-Ultra/TAG-Based-DE");

static int __init install(void)
{
	int i;

	//init tag table NULL entries
	//TODO 
	for(i = 0; i < TBL_ENTRIES_NUM; i++){
		tag_table[i] = NULL;
	}

	return -1;
}

static void __exit uninstall(void)
{
}



module_init(install)
module_exit(uninstall)
MODULE_LICENSE("GPL");
