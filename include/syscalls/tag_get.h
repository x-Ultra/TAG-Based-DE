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
        case UNEXPECTED:
            printk(KERT_ALERT "%s: Unexpected error, check previous message ", TAG_GET);
            break;
        default:
            printk(KERN_ERR "%s: Unknown error", TAG_GET);
    }
}


int merge_rnd_descriptor(int rnd, int descriptor)
{
    //rnd will be like (if PRIV_TAG_BIT = 12)
    //00000000-00005678-12345678-12345678
    //Descriptor
    //00000000-00000000-00001111-11111111
    if(descriptor >= MAX_PRIVATE_TAGS){
        printk(KERN_ALERT "%s: ipc private descriptor is more that expected ?!", TAG_GET);
        tag_get_error(UNEXPECTED);
        return UNEXPECTED;
    }

    //the expected result:
    //12345678-12345678-56781111-11111111
    //      ^                       ^
    //      |                       |
    //the "private key"  "the real descriptor"
    return (rnd << PRIV_TAG_BIT) & descriptor;
}


//TODO fix here and check if working ! ! ! !
unsigned long integer_xor(int *rnd, void *addr)
{
    int i;
    unsigned long xorred = 0;
    unsigned long adjusted_rnd = rnd;

    //adjust key depending on PRIV_TAG_BIT value
    *rnd = *rnd >> PRIV_TAG_BIT;

    adjusted_rnd = rnd;
    //lets make sure that only the relevant bits are non-zero
    adjusted_rnd = adjusted_rnd << (sizeof(void *)-(sizeof(int)));
    adjusted_rnd = adjusted_rnd >> (sizeof(void *)-(sizeof(int)));
    adjusted_rnd = adjusted_rnd << 11;

    //                     What are we doing here ?
    //void *addr =
    //12345678-12345678-12345678-12345678-12345678-12345678-12345678-12345678
    //XOR
    //adjusted_rnd =                (IF PRIV_TAG_BIT = 12, RND BIT = 20
    //00000000-00000000-00000000-00000000-02345678-12345678-12345000-00000000
    //the fact that the first 11 bits are not xorred is because the most
    //relevant information is contained from the 12-th bit on, and not from the ones
    //used to specify the page offset.

    //do the XOR between rnd and addr (starting from 11-th bit of addr)
    for(i = 11; i < sizeof(void *); i++){
        xorred = addr ^ adjusted_rnd;
    }

    return xorred;
}


int set_up_tag_level(struct tag_service *new_service, int key, int permission)
{
    int rnd = 0;
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
    new_service->tag_levels_list = tag_levels_list;


    //upon accesing an ipc_private tag_service, it will be checked
    //that integer_xor(fisrt-X-bits-of(Descriptor)=rnd, tag_service->ipc_private_check)
    //is equal to the address of tag_service->tag_levels_list.
    //In this way only the owner of the descriptor (and his childs), will be
    //able to access the tag_service information.
    //This mechanism is introduced due to avoid an arbitraty thread to acces
    //an ipc private service by trying the different values of the descriptor.
    if(key == TAG_IPC_PRIVATE){
        while(rnd == 0)
            get_random_bytes(&rnd, 4);

        if(rnd < 0)
            rnd = -rnd;
        //the value of rnd will be adjusted depending on PRIV_TAG_BIT
        new_service->ipc_private_check = integer_xor(&rnd, new_service->tag_levels_list);
        return rnd;
    }

    //if tag is not private
    return 0;
}


int create_tag_service(int key, int permission)
{
    //Check if key was already assigned
    int descriptor, max_descriptors, free_key_entry, rnd;
    struct tag_service *new_service;

    //2. introduce and implement ipc_private tag service generation
    //3. test

    //finding out if key is already used
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
    //here we are sure that nobody has used the same key for a tag service

    //finding a free descriptor in tag_table, depending
    //on whether the key is private or not
    if(key == TAG_IPC_PRIVATE){
        descriptor = 0;
        max_descriptors = MAX_PRIVATE_TAGS;
    }else{
        descriptor = MAX_PRIVATE_TAGS;
        max_descriptors = TBL_ENTRIES_NUM;
    }

    //the tag_table counld be accessed by a softirq (the 'cleaner')
    //so cpin lock bottom halves should be used
    spin_lock_bh(&tag_tbl_spin);
    //scanning the tag_table for a free entry
    for(;descriptor < max_descriptors; descriptor++){
        if(tag_table[descriptor] == NULL)
            break;
    }
    if(descriptor == max_descriptors){
        printk(KERN_ALERT "TAG table is full, but not used_keys. This should not have happened");
        return UNEXPECTED;
    }

    //Creating tag table entry
    if((new_service = (struct tag_service *)kmalloc(sizeof(struct tag_service), GFP_KERNEL)) == NULL){
		printk(KERN_ERR "%s: Unable to alloc tag_service", TAG_GET)
		return ERR_KMALLOC;
	}

    if((rnd = set_up_tag_level(new_service, key, permission)) < 0){
        return SERVICE_SETUP_FAIED;
    }

    //inserting the new entry in used_keys & tag_table
    used_keys[free_key_entry] = key;
    tag_table[descriptor] = new_service;
    spin_unlock(&tag_tbl_spin);

    if(key != TAG_IPC_PRIVATE)
        return descriptor;

    return merge_rnd_descriptor(rnd, descriptor);
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
