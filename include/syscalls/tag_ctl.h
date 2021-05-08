
//function that audit tag_get error repending on (negative) return code
void tag_ctl_error(int errorcode)
{
    switch(errorcode){
        case INVALID_CMD:
            printk(KERN_ERR "%s: Command value incorrect, choose REMOVE or AWAKE_ALL", TAG_CTL);
            break;
        case INVALID_EUID:
            printk(KERN_ERR "%s: Invalid EUID", TAG_CTL);
            break;
        case SERVICE_IN_USE:
            printk(KERN_ERR "%s: There are still some threads waiting for data on this service", TAG_CTL);
            break;
        case INVALID_DESCR:
            printk(KERN_ERR "%s: Invalid descriptor", TAG_CTL);
            break;
        case WRONG_PWD:
            printk(KERN_ERR "%s: Wrong password, unable to delete tag service", TAG_GET);
            prevent_bruteforce(TAG_CTL);
            break;
        default:
            printk(KERN_ALERT "%s: Unknown error", TAG_CTL);
    }
}


int remove_tag_service(int descriptor)
{
    struct tag_service *tag_service;
    int priv_descriptor, i, the_key, pwd, orig_descriptor;
    kuid_t EUID;

    //keeping original descriptor for eventual pwd check
    orig_descriptor = descriptor;
    //is descriptor good ?
    if((unsigned int)descriptor >= TBL_ENTRIES_NUM){
        //maybe it comes from an IPC_PRIVATE service
        priv_descriptor = (descriptor << PRIV_PWD_BITS) >> PRIV_PWD_BITS;
        //let's do the check again
        if(priv_descriptor >= TBL_ENTRIES_NUM || priv_descriptor < 0){
            return INVALID_DESCR;
        }
        descriptor = priv_descriptor;
    }
    if(tag_table[descriptor] == NULL){
        AUDIT
            printk(KERN_ERR "%s: tag_table[descriptor] is NULL", TAG_CTL);
        return INVALID_DESCR;
    }
    AUDIT
        printk(KERN_DEBUG "%s: descriptor checked, removing %d", TAG_CTL, descriptor);

    //fetching tag service using descriptor
    spin_lock(&tag_tbl_spin);
    tag_service = tag_table[descriptor];

    //TODO check password if TAG_IPC_PRIVATE
    if(tag_service->key == TAG_IPC_PRIVATE){
        pwd = orig_descriptor >> (sizeof(unsigned int)*8 - PRIV_PWD_BITS);
        if(pwd != tag_service->ipc_private_pwd){
            spin_unlock(&tag_tbl_spin);
            return WRONG_PWD;
        }
    }

    AUDIT
        printk(KERN_DEBUG "%s: tag service acquired", TAG_CTL);

    //checking permission
    if(tag_service->permission == PERMISSION_USER){
        EUID = current_euid();
        if(!uid_eq(EUID, tag_service->creator_euid)){
            spin_unlock(&tag_tbl_spin);
            return INVALID_EUID;
        }
    }

    AUDIT
        printk(KERN_DEBUG "%s: Permission ok", TAG_CTL);

    //freeing the tag table entry and all data allocated in it
    //only if there is no thread waiting for messages
    //          Note:
    //The last receiving thread on a tag service has to set
    //tag_service->tag_levels->level_num = NO_TAG_LEVELS
    //(and free al allocated struuctures)
    //in order to communicate that there are no more threads waiting
    if(tag_service->tag_levels->level_num != NO_TAG_LEVELS){
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
                tag_ctl_error(retval);
            }
            break;
        default:
            tag_ctl_error(INVALID_CMD);
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
