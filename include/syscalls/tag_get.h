//random lib used only in this module
#include <linux/random.h>

//Remove after testing
void retreive_descriptor_pwd(unsigned int merged)
{
    int descriptor;
    unsigned int pwd;

    printk(KERN_DEBUG "%s: merged descriptor: %u", TAG_GET, merged);

    pwd = merged >> (sizeof(unsigned int)*8 - PRIV_PWD_BITS);
    descriptor = (merged << PRIV_PWD_BITS) >> PRIV_PWD_BITS;
    printk(KERN_DEBUG "%s: Reteiving info. Descriptor: %d, Pwd: %u", TAG_GET, descriptor, pwd);
}


int merge_rnd_descriptor(int rnd, int descriptor)
{
    //rnd will be like (if PRIV_PWD_BITS = 20)
    //00000000-00000RRR-RRRRRRRR-RRRRRRRR
    //Descriptor
    //00000000-00000000-0000DDDD-DDDDDDDD


    //the expected result:
    //0RRRRRRR-RRRRRRRR-RRRRDDDD-DDDDDDD    //NB: the first bit is zero...
    //      ^                       ^
    //      |                       |
    //"private pwd"        "real descriptor"
    return (rnd << (sizeof(unsigned int)*8 - PRIV_PWD_BITS)) | descriptor;
}


int set_up_tag_service(struct tag_service *new_service, int key, int permission)
{
    unsigned int rnd = 0;

    AUDIT
        printk(KERN_DEBUG "%s: Setting up tag level", TAG_GET);

    new_service->creator_pid = current->pid;
    new_service->creator_euid = current_euid();
    new_service->key = key;
    new_service->permission = permission;
    new_service->tag_levels = NULL;
    new_service->awake_all = 0;
    spin_lock_init(&new_service->lvl_spin);

    AUDIT
        printk(KERN_DEBUG "%s: new_service setup ok", TAG_GET);

    //upon accesing an ipc_private tag_service, it will be checked
    //that tag_service->ipc_private_pwd
    //is equal to the MOST significat PRIV_PWD_BITS bits (check merge_rnd_descriptor).
    //In this way only the owner of the descriptor (and his childs), will be
    //able to access the tag_service information.
    //This mechanism is introduced due to avoid an arbitraty thread to acces
    //an ipc private service by trying the different values of the descriptor.
    if(key == TAG_IPC_PRIVATE){
        while(rnd == 0)
            get_random_bytes(&rnd, 4);

        //check merge_rnd_descriptor explaination, MSB has to be 0
        rnd = rnd & positron;
        AUDIT
            printk(KERN_DEBUG "%s: Random value: %d", TAG_GET, rnd);
        //adjust random depending on PRIV_PWD_BITS value
        rnd = rnd >> (sizeof(unsigned int)*8 - PRIV_PWD_BITS);
        new_service->ipc_private_pwd = rnd;
        AUDIT
            printk(KERN_DEBUG "%s: Adjusted value: %d, %d", TAG_GET, new_service->ipc_private_pwd, rnd);
        return rnd;
    }

    //if tag is not private
    return 0;
}


int create_tag_service(int key, int permission)
{
    //Check if key was already assigned
    int descriptor, free_key_entry = 0;
    unsigned int rnd;
    struct tag_service *new_service;

    printk(KERN_ALERT "%s: Creating tag called", TAG_GET);

    //finding out if key is already used, if not private
    if(key != TAG_IPC_PRIVATE){
        for(free_key_entry = 0; free_key_entry < TBL_ENTRIES_NUM; free_key_entry++){
            if(used_keys[free_key_entry] == -1){
                break;
            }else if(used_keys[free_key_entry] == key){
                return KEY_USED;
            }
        }
        if(free_key_entry == TBL_ENTRIES_NUM){
            return TAG_TBL_FULL;
        }
        AUDIT
            printk(KERN_DEBUG "%s: Free entry: %d", TAG_GET, free_key_entry);
        //here we are sure that nobody has used the same key for a tag service
    }

    //Finding a free descriptor in tag_table.
    //The tag_table counld be accessed by a softirq (the 'cleaner')
    //so spin lock bottom halves should be used.

    //preempt_enable()/disable would be redundant
    spin_lock(&tag_tbl_spin);
    AUDIT
        printk(KERN_DEBUG "%s: Spinlock bh called", TAG_GET);

    //scanning the tag_table for a free entry
    for(descriptor = 0 ;descriptor < TBL_ENTRIES_NUM; descriptor++){
        if(tag_table[descriptor] == NULL)
            break;
    }
    if(descriptor == TBL_ENTRIES_NUM){
        printk(KERN_ALERT "TAG table is full, but not used_keys. This should not have happened");
        spin_unlock(&tag_tbl_spin);
        return UNEXPECTED;
    }
    AUDIT
        printk(KERN_DEBUG "%s: Descriptor found: %d", TAG_GET, descriptor);

    //Creating tag table entry
    if((new_service = (struct tag_service *)kmalloc(sizeof(struct tag_service), GFP_KERNEL)) == NULL){
		printk(KERN_ERR "%s: Unable to alloc tag_service", TAG_GET);
        spin_unlock(&tag_tbl_spin);
		return ERR_KMALLOC;
	}

    if((rnd = set_up_tag_service(new_service, key, permission)) < 0){
        spin_unlock(&tag_tbl_spin);
        return SERVICE_SETUP_FAIED;
    }

    //inserting the new entry in used_keys & tag_table
    if(key != TAG_IPC_PRIVATE){
        used_keys[free_key_entry] = key;
        num_used_keys += 1;
    }

    tag_table[descriptor] = new_service;
    spin_unlock(&tag_tbl_spin);

    if(key != TAG_IPC_PRIVATE)
        return descriptor;

    AUDIT
        retreive_descriptor_pwd(merge_rnd_descriptor(rnd, descriptor));

    return merge_rnd_descriptor(rnd, descriptor);
}


//function that fetched the tag tableentry depending on tag Key
int fetch_tag_desc(int key, int permission)
{
    int descriptor;
    kuid_t EUID;
    struct tag_service *current_entry = tag_table[0];

    printk(KERN_ALERT "%s: Fetching tag called", TAG_GET);
    return 3;

    //Check key type
    if(key == TAG_IPC_PRIVATE)
        return PRIVATE_OPEN;

    //scaninng tag table
    //preempt_enable()/disable would be redundant
    spin_lock(&tag_tbl_spin);
    for(descriptor = 0; descriptor < TBL_ENTRIES_NUM; descriptor++){

        current_entry = tag_table[descriptor];

        if(current_entry == NULL){
            spin_unlock(&tag_tbl_spin);
            return KEY_NOT_FOUND;
        }
        if(current_entry->key == key){
            break;
        }
        if(descriptor == TBL_ENTRIES_NUM-1){
            spin_unlock(&tag_tbl_spin);
            return KEY_NOT_FOUND;
        }

    }
    spin_unlock(&tag_tbl_spin);

    //perform checks based on permission value of current_entry
    if(permission == PERMISSION_USER){
        EUID = current_euid();
        if(!uid_eq(EUID, current_entry->creator_euid)){
            return INVALID_EUID;
        }
    }

    return descriptor;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(3, _tag_get, int, key, int, command, int, permission){
#else
asmlinkage int sys_tag_get(int key, int command, int permission)
{
#endif
    int tag_descriptor;

    /*
    if(try_module_get(THIS_MODULE) == 0){
		return MOD_INUSE;
	}
    */

    //key reserved
    if(key == -1){
        tag_error(KEY_RESERVED, TAG_GET);
        //module_put(THIS_MODULE);
        return KEY_RESERVED;
    }

    //check command type
    switch(command){
        case CMD_OPEN:
            if((tag_descriptor = fetch_tag_desc(key, permission)) < 0){
                tag_error(tag_descriptor, TAG_GET);
            }
            break;
        case CMD_CREATE:
            if((tag_descriptor = create_tag_service(key, permission)) < 0){
                tag_error(tag_descriptor, TAG_GET);
            }
            break;
        default:
            tag_error(INVALID_CMD, TAG_GET);
            tag_descriptor = INVALID_CMD;
            break;
    }

    //module_put(THIS_MODULE);
    return tag_descriptor;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
static unsigned long sys_tag_get = (unsigned long) __x64_sys_tag_get;
#else
#endif
