//random lib used only in this module
#include <linux/random.h>

//function that audit tag_get error repending on (negative) return code
void tag_get_error(int errorcode)
{

    switch(errorcode){
        case KEY_USED:
            printk(KERN_ERR "%s: Key was already used", TAG_GET);
            break;
        case PRIVATE_OPEN:
            printk(KERN_ERR "%s: Tag Open on private key not permitted", TAG_GET);
            break;
        case INVALID_CMD:
            printk(KERN_ERR "%s: Command value incorrect");
            break;
        case KEY_NOT_FOUND:
            printk(KERN_ERR "%s: Key was not found", TAG_GET);
            break;
        case INVALID_EUID:
            printk(KERN_ERR "%s: Invalid EUID", TAG_GET);
            break;
        case MOD_INUSE:
            printk(KERN_ERR "%s: Module in use", TAG_GET);
            break;
        case KEY_RESERVED:
            printk(KERN_ERR "%s: Key -1 is reserved", TAG_GET);
            break;
        case ERR_KMALLOC:
            printk(KERN_ERR "%s: Unable to kmalloc", TAG_GET);
            break;
        case SERVICE_SETUP_FAIED:
            printk(KERN_ERR "%s: Unable to setup new tag service", TAG_GET);
            break;
        case TAG_TBL_FULL:
            printk(KERN_ERR "%s: Tag Table is full, try again later", TAG_GET);
            break;
        default:
            printk(KERN_ERR "%s: Unknown error", TAG_GET);
    }
}


unsigned long integer_xor()
{

}


int set_up_tag_level(struct tag_service *new_service, int key, int permission)
{
    unigned long rnd;
    struct tag_level *tag_levels_list;

    if((tag_levels_list = (struct tag_level *)kmalloc(sizeof(struct tag_level), GFP_KERNEL)) == NULL){
		printk(KERN_ERR "%s: Unable to alloc tag_level", TAG_GET)
		return ERR_KMALLOC;
	}
    tag_levels_list->level_num = NO_TAG_LEVELS;

    new_service->creator_pid = current->pid;
    new_service->creator_euid = current_euid();
    new_service->key = key;
    new_service->permission = permission;
    spin_lock_init(&(new_service->removing));
    new_service->tag_levels_list = tag_levels_list;


    get_random_bytes(&rnd, 8);
    new_service->ipc_private_check = integer_xor(rnd, new_service->tag_levels_list);

    //upon accesing an ipc_private tag_service, it will be checked
    //that integer_xor(fisrt-X-bits-of(Descriptor)=rnd, tag_service->ipc_private_check)
    //must be equal to the address of tag_service->tag_levels_list.
    //In this way only the owned of the descriptor, and then of the rnd, will be
    //able to access the tag_service information.
    //This mechanism is introduced due to avoid an arbitraty thread to acces
    //an ipc private service by tring the different values of the descriptor.
    if(rnd < 0)
        return -rnd;
    return rnd;

}


int create_tag_service(int key, int permission)
{
    //Check if key was already assigned
    int descriptor, max_descriptors;
    struct tag_service *new_service;

    if(key == TAG_IPC_PRIVATE){
        descriptor = 0;
        max_descriptors = MAX_PRIVATE_TAGS;
    }else{
        descriptor = MAX_PRIVATE_TAGS;
        max_descriptors = TBL_ENTRIES_NUM;
    }
    //using a mutex instead of a spinlock because the following
    //actions may take a while
    mutex_lock(&tag_tbl_mtx);
    //TODO CONTINUE here
    //1. fix key handling
    //2. introduce and implement ipc_private tag service generation
    //3. test
    //TODO there is no more used_keys ! ! !
    //TODO change used_kjeys handling with tag_tbl handling

    //finding a free entry on the table
    for(; descriptor < max_descriptors; descriptor++){
        if(used_keys[descriptor] == -1){
            break;
        }else if(used_keys[descriptor] == key){
            return KEY_USED;
        }
    }
    if(key == TAG_IPC_PRIVATE){
        if(descriptor == MAX_PRIVATE_TAGS)
            return TAG_TBL_FULL;
    }else{
        if(descriptor == TBL_ENTRIES_NUM)
            return TAG_TBL_FULL;
    }


    //Creating tag table entry
    if((new_service = (struct tag_service *)kmalloc(sizeof(struct tag_service), GFP_KERNEL)) == NULL){
		printk(KERN_ERR "%s: Unable to alloc tag_service", TAG_GET)
		return ERR_KMALLOC;
	}

    if(set_up_tag_level(new_service, key, permission) < 0){
        return SERVICE_SETUP_FAIED;
    }

    //inserting the new entry in used_keys
    used_keys[descriptor]
    mutex_lock(&tag_tbl_mtx);

    //inserting the new entry in tag table

    return 0;
}


//function that fetched the tag tableentry depending on tag Key
int fetch_tag_desc(int key, int permission)
{
    int descriptor;
    uid_t EUID;
    struct tag_service *current_entry = tag_table[0]

    //Check key type
    if(key == TAG_IPC_PRIVATE)
        return PRIVATE_OPEN;

    //scaninng tag table
    for(descriptor = 0; descriptor < TBL_ENTRIES_NUM; descriptor++){

        current_entry = tag_table[descriptor];
        //if the cleaner has acquired the entry it will be
        //deleted soon, so skip this entry
        if(!spin_trylock(current_entry->removing))
            continue;

        if(current_entry == NULL){
            spin_unlock(current_entry->removing);
            return KEY_NOT_FOUND;
        }
        if(current_entry->key == key){
            break;
        }
        if(i == TBL_ENTRIES_NUM-1){
            spin_unlock(current_entry->removing);
            return KEY_NOT_FOUND;
        }
        spin_unlock(current_entry->removing);
    }

    //perform checks based on permission value of current_entry
    if(permission == PERMISSION_USER){
        EUID = current_euid();
        if(EUID != current_entry->creator_euid){
            return INVALID_EUID;
        }
    }

    return descriptor;
}


asmlinkage int tag_get(int key, int command, int permission)
{
    int tag_descriptor;

    if(try_module_get(THIS_MODULE) == 0){
		return MOD_INUSE;
	}

    //key reserved
    if(key == -1){
        tag_get_error(KEY_RESERVED);
        module_put(THIS_MODULE);
        return KEY_RESERVED;
    }

    //check command type
    switch(command){
        case CMD_OPEN:
            if((tag_descriptor = fetch_tag_desc(key, permission)) < 0){
                tag_get_error(tag_descriptor);
            }
            break;
        case CMD_CREATE:
            if((tag_descriptor = create_tag_service(key, permission)) < 0){
                tag_get_error(tag_descriptor);
            }
            break;
        default:
            tag_get_error(INVALID_CMD);
            return -1;
    }

    module_put(THIS_MODULE);
    return tag_descriptor;
}
