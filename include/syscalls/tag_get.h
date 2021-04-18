#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kern_levels.h>
//kmalloc declaration
#include <linux/slab.h>
//header  for kmalloc flags
#include <linux/gfp.h>
//copying from-to user
#include <linux/uaccess.h>


//function that audit tag_get error repending on (negative) return code
void tag_get_error(int errorcode)
{
    if(errorcode == KEY_USED){
        printk(KERN_ERR "%s: Key was already used", TAG_GET);
    }else if(errorcode == PRIVATE_OPEN){
        printk(KERN_ERR "%s: Tag Open on private key not permitted", TAG_GET);
    }else if(errorcode == INVALID_CMD){
        printk(KERN_ERR "%s: Command value incorrect");
    }
}


int create_tag_service(int key, int permission)
{

    //Check if key was already assigned

    //Create tag table entry

    //TODO-> insert corret metadata to cleaner data structure

    //alloc tag table entry(tag_service)

    return 0;
}


//function that fetched the tag tableentry depending on tag Key
int fetch_tag_desc(int key, int permission)
{
    //perform checks based on permission value

    //TODO scan tag tableentry

    //sanity checks on key ! !
}


asmlinkage int tag_get(int key, int command, int permission)
{
        int tag_descriptor;

        //Check key type
        if(key == IPC_PRIVATE){
            if(command == CMD_OPEN){
                tag_get_error(PRIVATE_OPEN);
                return -1;
            }
            AUDIT
                printk(KERN_DEBUG "%s: Private key tag_get call", TAG_GET);
        }

        //check command type
        if(command == CMD_OPEN){
            AUDIT
                printk(KERN_DEBUG "%s: Private key tag_get call", TAG_GET);
            if((tag_descriptor = fetch_tag_desc(key, permission)) < 0){
                tag_get_error(tag_descriptor);
                return -1;
            }
        }else if(command == CMD_CREATE){
            if((tag_descriptor = create_tag_service(key, permission)) < 0){
                tag_get_error(tag_descriptor);
                return -1;
            }
        }else{
            tag_get_error(INVALID_CMD);
            return -1;
        }

        return 0;
}
