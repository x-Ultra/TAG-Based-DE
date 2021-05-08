
//function that puts metadata needed to receive a message using tag service
int put_receive_metadata(struct tag_service *tag_service, int level, char* buffer, size_t size)
{
    struct tag_levels_list *tag_levels;
    struct tag_levels_list *rcvng_level;
    struct receiving_threads *rcv_thread;
    struct receiving_threads *current_tr;

    //handling the case the thread is the first to wait on this service
    tag_levels = tag_service->tag_levels;
    if(tag_levels == NULL){
        AUDIT
            printk(KERN_DEBUG "%s: Setting up tag level", TAG_RECEIVE);

        if((tag_levels = (struct tag_levels_list *)kmalloc(sizeof(struct tag_level), GFP_KERNEL)) == NULL){
    		printk(KERN_ERR "%s: Unable to alloc tag_level", TAG_RECEIVE);
    		return ERR_KMALLOC;
    	}
        tag_levels->level_num = NO_TAG_LEVELS;
        tag_levels->prev = NULL;
        tag_levels->next = NULL;
        tag_levels->level.waiting_threads = 0;
        tag_service->tag_levels = tag_levels;
    }

    //if it the first to wait on this service
    if(tag_levels->level_num == NO_TAG_LEVELS){
        tag_levels->level_num = level;
        //this should be already set
        tag_levels->level.waiting_threads = 0;
        rcvng_level = tag_levels;
    }else{
        //finding level or creating a new one
        for(rcvng_level = tag_levels; ; rcvng_level = rcvng_level->next){
            if(rcvng_level->level_num == level){
                break;
            }
            if(rcvng_level->next == NULL){
                if((rcvng_level->next = (struct tag_levels_list *)kmalloc(sizeof(struct tag_levels_list), GFP_KERNEL)) == NULL){
            		printk(KERN_ERR "%s: Unable to alloc tag_level", TAG_RECEIVE);
            		return ERR_KMALLOC;
            	}
                rcvng_level = rcvng_level->next;
                rcvng_level->level_num = level;
                rcvng_level->prev = NULL;
                rcvng_level->next = NULL;
                rcvng_level->level.waiting_threads = 0;
                break;
            }
        }
    }

    //setting metadata into rcvng_level, could be empty or not
    //using RCU insted of spinlock/mutex to be faster, hopefully....
    //TODO test rcu vs spinlock
    if((rcv_thread = (struct receiving_threads *)kmalloc(sizeof(struct receiving_threads), GFP_KERNEL)) == NULL){
        printk(KERN_ERR "%s: Unable to alloc receiving_threads", TAG_RECEIVE);
        return ERR_KMALLOC;
    }

    rcu_read_lock();

    if(rcvng_level->level.waiting_threads == 0){
        rcvng_level->level.threads.data.pid = current->pid;
        rcvng_level->level.threads.data.buffer = buffer;
        rcvng_level->level.threads.data.size = size;
        rcvng_level->level.threads.next = NULL;
        //not needed
        kfree(rcv_thread);
    }else{
        for(current_tr = &(rcvng_level->level.threads); ; current_tr = current_tr->next){
            if(current_tr->next == NULL){
                rcv_thread->data.pid = current->pid;
                rcv_thread->data.buffer = buffer;
                rcv_thread->data.size = size;
                rcv_thread->next = NULL;
                current_tr->next = rcv_thread;
                //Metadata inserted
                break;
            }
        }
    }

    rcvng_level->level.waiting_threads += 1;

    rcu_read_unlock();
    return 0;
}


void clean_up_metadata()
{
    return;
}


//function that uses a sleep wait condition queue
void receive(void)
{
    //if awake, clean data put preavusly
    //AND if this thread was the last listening for data on this levels
    //do the appropiate cleanup operations
    return;

}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(4, _tag_receive, int, tag, int, level, char*, buffer, size_t, size){
#else
asmlinkage int sys_tag_receive(int tag, int level, char* buffer, size_t size)
{
#endif

    int orig_descriptor, descriptor, ret;
    struct tag_service *tag_service;
    orig_descriptor = tag;
    /*
    if(try_module_get(THIS_MODULE) == 0){
		return MOD_INUSE;
	}
    */

    spin_lock(&tag_tbl_spin);
    //checking descriptor validity
    if((descriptor = check_descriptor(tag, TAG_CTL)) < 0){
        spin_unlock(&tag_tbl_spin);
        return descriptor;
    }
    tag_service = tag_table[descriptor];
    //TODO postponing cleaner timer to corresponding descriptor
    spin_unlock(&tag_tbl_spin);

    //from here in, the cleaner wont wake up for CLEANER_SLEEP seconds
    //on this tag table entry

    //checking password, if needed
    if((ret = check_password(tag_service, orig_descriptor)) != 0){
        return ret;
    }
    //checking permission
    if((ret = check_permission(tag_service)) != 0){
        return ret;
    }
    spin_unlock(&tag_tbl_spin);

    //putting metadata into the right place
    if((ret = put_receive_metadata(tag_service, level, buffer, size)) < 0){
        tag_error(ret, TAG_RECEIVE);
        return ret;
    }

    //going to sleep (until given coindition is met)
    receive();

    //module_put(THIS_MODULE);
    return 0;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
static unsigned long sys_tag_receive = (unsigned long) __x64_sys_tag_receive;
#else
#endif
