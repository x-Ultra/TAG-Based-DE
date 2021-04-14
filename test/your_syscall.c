#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kern_levels.h>
#include <linux/uaccess.h>

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("What a cool description !");

#define MODNAME "pippo"

extern int syscall_remover(int syscall_indx);
extern int syscall_adder(void* syscall_addr, char* syscall_name, int syscall_name_length, int syscall_num_params);

//maintain the indexes returned by syscall_adder, in order to 
//remove them, if needed.
int pippo_indx;

asmlinkage void pippo(void *pippo)
{
	int ret;
	int kernel_pippo;
	ret = copy_from_user(&kernel_pippo,pippo,4);
	printk(KERN_DEBUG "Pippo syscall ret: %d", ret);
	printk(KERN_DEBUG "Pippo syscall int: %d", kernel_pippo);
}
// .
// .
// .
/*
	and so on up to 6 parameter
*/


static int __init install(void)
{


	printk(KERN_DEBUG "%s: Adding %s\n", MODNAME, "your_syscall");
	if((pippo_indx = syscall_adder(pippo, "pippo", 5, 1)) == -1){
		printk("%s: Unable to add pippo\n", MODNAME);
	}
	
	printk(KERN_DEBUG "%s: Syscall succesfuly added !\n", MODNAME);

	return 0;
}

static void __exit uninstall(void)
{

	int ret = 0;
	if(syscall_remover(pippo_indx) == -1){
		printk("%s: Unable to remove pippo\n", MODNAME);
		ret = -1;
	}

	if(ret != -1){
		printk(KERN_DEBUG "%s: Systemcalls removed correctly !\n", MODNAME);
	}

}

module_init(install)
module_exit(uninstall)
MODULE_LICENSE("GPL");