

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
        default:
            printk(KERN_ERR "%s: Unknown error", TAG_GET);
    }
}


int create_tag_service(int key, int permission)
{
    //Check if key was already assigned
    int descriptor;
    struct tag_service *new_service;

    if(try_module_get(THIS_MODULE) == 0){
		return MOD_INUSE;
	}

    mutex_lock(&adding_key_spin);
    for(descriptor = 0; descriptor < TBL_ENTRIES_NUM; descriptor++){
        if(used_keys[descriptor] == -1){
            break;
        }else if(used_keys[descriptor] == key){
            module_put(THIS_MODULE);
            return KEY_USED;
        }
    }


    //Creating tag table entry
    if((new_service = kmalloc(sizeof(struct tag_service), GFP_KERNEL)) == NULL){
		printk(KERN_ERR "%s: Unable to alloc tag_service", TAG_GET)
		return ERR_KMALLOC;
	}

    //TODO-> insert corret metadata to cleaner data structure
    //TODO spin_lock_init(), mutex_init()...
    //CONTINUE HERE ! ! ! ! !  !! ! ! !  ! !

    //insert tag table entry(tag_service)

    mutex_unlock(&adding_key_mtx);
    return 0;
}


//function that fetched the tag tableentry depending on tag Key
int fetch_tag_desc(int key, int permission)
{
    int descriptor;
    uid_t EUID;
    struct tag_service *current_entry = tag_table[0]

    //Check key type
    if(key == IPC_PRIVATE)
        return PRIVATE_OPEN;

    if(try_module_get(THIS_MODULE) == 0){
		return MOD_INUSE;
	}

    //scaninng tag table
    for(descriptor = 0; descriptor < TBL_ENTRIES_NUM; descriptor++){

        current_entry = tag_table[descriptor];
        //if the cleaner has acquired the entry it will be
        //deleted soon, so skip this entry
        if(!spin_trylock(current_entry->removing))
            continue;

        if(current_entry == NULL){
            spin_unlock(current_entry->removing);
            module_put(THIS_MODULE);
            return KEY_NOT_FOUND;
        }
        if(current_entry->key == key){
            break;
        }
        if(i == TBL_ENTRIES_NUM-1){
            spin_unlock(current_entry->removing);
            module_put(THIS_MODULE);
            return KEY_NOT_FOUND;
        }
        spin_unlock(current_entry->removing);
    }

    //perform checks based on permission value of current_entry
    if(permission == PERMISSION_USER){
        EUID = current_euid();
        if(EUID != current_entry->creator_euid){
            module_put(THIS_MODULE);
            return INVALID_EUID;
        }
    }

    module_put(THIS_MODULE);
    return descriptor;
}


asmlinkage int tag_get(int key, int command, int permission)
{
    int tag_descriptor;

    //key reserved
    if(key == -1){
        tag_get_error(KEY_RESERVED);
        return -1;
    }

    //check command type
    switch(command){
        case CMD_OPEN:
            if((tag_descriptor = fetch_tag_desc(key, permission)) < 0){
                tag_get_error(tag_descriptor);
                return -1;
            }
            break;
        case CMD_CREATE:
            if((tag_descriptor = create_tag_service(key, permission)) < 0){
                tag_get_error(tag_descriptor);
                return -1;
            }
            break;
        default:
            tag_get_error(INVALID_CMD);
            return -1;
    }

    return tag_descriptor;
}
