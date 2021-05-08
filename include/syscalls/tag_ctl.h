
//if CMD = REMOVE this function will be triggered
int remove_tag_service(int descriptor)
{
    struct tag_service *tag_service;
    int ret, i, the_key, orig_descriptor;
    orig_descriptor = descriptor;

    spin_lock(&tag_tbl_spin);
    //cheking descriptor validity
    if((descriptor = check_descriptor(descriptor, TAG_CTL)) < 0){
        spin_unlock(&tag_tbl_spin);
        return descriptor;
    }
    //fetching tag service using descriptor
    tag_service = tag_table[descriptor];
    AUDIT
        printk(KERN_DEBUG "%s: descriptor checked, removing %d", TAG_CTL, descriptor);

    //checking password if TAG_IPC_PRIVATE
    if((ret = check_password(tag_service, orig_descriptor)) != 0){
        spin_unlock(&tag_tbl_spin);
        return ret;
    }

    //checking permission
    if((ret = check_permission(tag_service)) != 0){
        spin_unlock(&tag_tbl_spin);
        return ret;
    }

    AUDIT
        printk(KERN_DEBUG "%s: Permission ok", TAG_CTL);

    //freeing the tag table entry and all data allocated in it
    //only if there is no thread waiting for messages
    //          Note:
    //The last receiving thread on a tag service has to free the tag level
    //(and free all other allocated structures)
    //in order to communicate that there are no more threads waiting
    if(tag_service->tag_levels != NULL){
        spin_unlock(&tag_tbl_spin);
        return SERVICE_IN_USE;
    }

    AUDIT
        printk(KERN_DEBUG "%s: waiting threads checked", TAG_CTL);

    //removing the descriptor on the used_keys in such a way to
    //have '-1' entries ONLY at the end of the table
    //checking that key is in used_keys
    if(tag_service->key != TAG_IPC_PRIVATE){
        the_key = tag_service->key;
        for(i = 0; i < TBL_ENTRIES_NUM; i++){
            if(used_keys[i] == the_key){
                //removing could be performed
                used_keys[i] = used_keys[num_used_keys-1];
                used_keys[num_used_keys-1] = -1;
                num_used_keys -= 1;
                break;
            }
        }
    }

    //freeing data structures allocated in tag get
    kfree(tag_service->tag_levels);
    kfree(tag_service);
    tag_table[descriptor] = NULL;

    AUDIT
        printk(KERN_DEBUG "%s: tag table updated", TAG_CTL);

    //done
    spin_unlock(&tag_tbl_spin);
    return 0;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(2, _tag_ctl, int, tag, int, command){
#else
asmlinkage int sys_tag_ctl(int tag, int command)
{
#endif
    int retval;

    /*
    if(try_module_get(THIS_MODULE) == 0){
		return MOD_INUSE;
	}
    */

    switch(command){
        case AWAKE_ALL:
            printk(KERN_DEBUG "%s: Not implemented yet", TAG_CTL);
            return 0;
        case REMOVE:
            if((retval = remove_tag_service(tag)) < 0){
                tag_error(retval, TAG_CTL);
            }
            break;
        default:
            tag_error(INVALID_CMD, TAG_CTL);
            retval = INVALID_CMD;
            break;
    }
    AUDIT
        printk(KERN_ALERT "%s: removing done", TAG_CTL);

    //module_put(THIS_MODULE);
    return retval;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
static unsigned long sys_tag_ctl = (unsigned long) __x64_sys_tag_ctl;
#else
#endif
