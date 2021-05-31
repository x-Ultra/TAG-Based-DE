
//if CMD = REMOVE this function will be triggered
int remove_tag_service(int descriptor)
{
    struct tag_service *tag_service;
    int ret, i, the_key, orig_descriptor;
    orig_descriptor = descriptor;

    //preempt_disable()/enable() wounld be reduntant
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

    //if semaphore has been acquired, some thread is waiting
    //NB: we are unpreemtable (spinlock), so this thread will
    //never go to sleep because of down_->try<-lock.
    //This is more like a 'reverse semaphore'...
    AUDIT
        printk(KERN_DEBUG "%s: Semaphore count %u", TAG_CTL, semaphores[descriptor].count);
    if(!down_trylock(&semaphores[descriptor])){
        //if i am able to acquire 2 time the semaphore,
        //some thread has the tag service.
        if(!down_trylock(&semaphores[descriptor])){
            up(&semaphores[descriptor]);
            up(&semaphores[descriptor]);
            spin_unlock(&tag_tbl_spin);
            return SERVICE_IN_USE;
        }
    }else{
        //if i am not able to acquire the semaphore for the first time,
        //another removing ops for this service has been called
        spin_unlock(&tag_tbl_spin);
        return BEING_DELETED;
    }
    AUDIT
        printk(KERN_DEBUG "%s: waiting threads checked", TAG_CTL);

    //checking password if TAG_IPC_PRIVATE
    if((ret = check_password(tag_service, orig_descriptor)) != 0){
        up(&semaphores[descriptor]);
        spin_unlock(&tag_tbl_spin);
        return ret;
    }

    //checking permission
    if((ret = check_permission(tag_service)) != 0){
        up(&semaphores[descriptor]);
        spin_unlock(&tag_tbl_spin);
        return ret;
    }

    AUDIT
        printk(KERN_DEBUG "%s: Permission ok", TAG_CTL);

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
    up(&semaphores[descriptor]);
    spin_unlock(&tag_tbl_spin);
    return 0;
}


int awake_all(int tag)
{
    int descriptor;
    struct tag_service *tag_service;

    //checking descrptor validity
    if((descriptor = check_input_data_head(tag)) < 0){
        //descriptor contains the error code
        return descriptor;
    }
    tag_service = tag_table[descriptor];

    AUDIT
        printk(KERN_DEBUG "%s: Awaking on %d, old_awake: %d", TAG_CTL, descriptor, tag_service->awake_all);

    //No need to acquire the spinlock, semaphore acquired with check_input_data_tail
    //will prevent the remotion of this level
    if(tag_service->awake_all > (unsigned)(1 << 15) ){
        tag_service->awake_all = 0;
    }
    tag_service->awake_all += 1;
    wake_up(&receiving_queue);

    AUDIT
        printk(KERN_DEBUG "%s: Awaking done, new awake_all: %d", TAG_CTL, tag_service->awake_all);

    check_input_data_tail(descriptor);
    return 0;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(2, _tag_ctl, int, tag, int, command){
#else
asmlinkage int sys_tag_ctl(int tag, int command)
{
#endif
    int retval;

    if(try_module_get(THIS_MODULE) == 0){
		return MOD_INUSE;
	}

    switch(command){
        case AWAKE_ALL:
            if((retval = awake_all(tag)) < 0){
                tag_error(retval, TAG_CTL);
            }
            break;
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

    module_put(THIS_MODULE);
    return retval;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
static unsigned long sys_tag_ctl = (unsigned long) __x64_sys_tag_ctl;
#else
#endif
