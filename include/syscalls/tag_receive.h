
//function that puts metadata needed to receive a message using tag service
struct tag_levels_list* put_receive_metadata(struct tag_service *tag_service, int level, char* buffer, size_t size)
{
    struct tag_levels_list *tag_levels;
    struct tag_levels_list *rcvng_level;
    struct receiving_threads *rcv_thread;
    struct receiving_threads *current_tr;
    AUDIT
        printk(KERN_DEBUG "%s: putting thread metadata", TAG_RECEIVE);

    //handling the case the thread is the first to wait on this service
    spin_lock(&tag_service->lvl_spin);
    tag_levels = tag_service->tag_levels;
    if(tag_levels == NULL){
        AUDIT
            printk(KERN_DEBUG "%s: Setting up tag level", TAG_RECEIVE);

        if((tag_levels = (struct tag_levels_list *)kmalloc(sizeof(struct tag_levels_list), GFP_KERNEL)) == NULL){
    		printk(KERN_ERR "%s: Unable to alloc tag_level", TAG_RECEIVE);
            spin_unlock(&tag_service->lvl_spin);
    		return NULL;
    	}
        tag_levels->level_num = NO_TAG_LEVELS;
        tag_levels->prev = NULL;
        tag_levels->next = NULL;
        tag_levels->level.waiting_threads = 0;
        tag_levels->level.threads = NULL;
        tag_levels->level.data_received = 0;
        spin_lock_init(&tag_levels->level.lock);
        tag_service->tag_levels = tag_levels;
    }

    //FINDING LEVEL POSITION
    //if it the first to wait on this service
    if(tag_levels->level_num == NO_TAG_LEVELS){
        tag_levels->level_num = level;
        rcvng_level = tag_levels;
    }else{
        //finding level or ....
        for(rcvng_level = tag_levels; ; rcvng_level = rcvng_level->next){
            if(rcvng_level->level_num == level){
                break;
            }
            //...creating a new one if not found
            if(rcvng_level->next == NULL){
                if((rcvng_level->next = (struct tag_levels_list *)kmalloc(sizeof(struct tag_levels_list), GFP_KERNEL)) == NULL){
            		printk(KERN_ERR "%s: Unable to alloc tag_level", TAG_RECEIVE);
            		return NULL;
            	}
                rcvng_level->next->prev = rcvng_level;
                rcvng_level->next->next = NULL;
                rcvng_level = rcvng_level->next;

                rcvng_level->level_num = level;
                spin_lock_init(&rcvng_level->level.lock);
                rcvng_level->level.waiting_threads = 0;
                rcvng_level->level.data_received = 0;
                rcvng_level->level.threads = NULL;
                break;
            }
        }
    }
    spin_unlock(&tag_service->lvl_spin);

    //FINDING THREADS METADATA POSITION
    //setting metadata into rcvng_level, could be empty or not
    if((rcv_thread = (struct receiving_threads *)kmalloc(sizeof(struct receiving_threads), GFP_KERNEL)) == NULL){
        printk(KERN_ERR "%s: Unable to alloc receiving_threads", TAG_RECEIVE);
        return NULL;
    }
    rcv_thread->data.pid = current->pid;
    rcv_thread->data.buffer = buffer;
    rcv_thread->data.size = size;
    rcv_thread->next = NULL;
    //------end of thread new data to insert
    spin_lock(&rcvng_level->level.lock);
    if(rcvng_level->level.waiting_threads == 0){
        rcvng_level->level.threads = rcv_thread;
    }else{
        //if there are some other threads waiting on this level,
        //we have to insert the entry in the last postition
        for(current_tr = rcvng_level->level.threads; ; current_tr = current_tr->next){
            if(current_tr->next == NULL){
                current_tr->next = rcv_thread;
                //Metadata inserted
                break;
            }
        }
    }
    rcvng_level->level.waiting_threads += 1;

    spin_unlock(&rcvng_level->level.lock);

    AUDIT
        printk(KERN_DEBUG "%s: Level: %d, waiting thread: %d", TAG_RECEIVE, rcvng_level->level_num, rcvng_level->level.waiting_threads);

    return rcvng_level;
}


int remove_thread_metadata(struct tag_levels_list *rcvng_level)
{
    struct receiving_threads *current_tr, *prev_tr;
    pid_t pid;
    pid = current->pid;

    AUDIT
        printk(KERN_DEBUG "%s: Removing thread metadata", TAG_RECEIVE);

    prev_tr = NULL;
    for(current_tr = rcvng_level->level.threads; ; current_tr = current_tr->next){
        if(current_tr->data.pid == pid){
            if(current_tr->next != NULL){
                if(prev_tr != NULL){
                    //we are deleting a general entry here
                    prev_tr->next = current_tr->next;
                }else{
                    //we are deleting the first entry here
                    rcvng_level->level.threads = current_tr->next;
                }
            }else{
                if(prev_tr == NULL){
                    //we are deleting the only one entry <- this hould never hapen
                    rcvng_level->level.threads = current_tr->next;
                    printk(KERN_ALERT "%s: Removing the only one thread, but waiting threads > 1", TAG_RECEIVE);
                }else{
                    //we are deleting last entry here
                    prev_tr->next = NULL;
                }
            }
            kfree(current_tr);
            //Metadata removed
            rcvng_level->level.waiting_threads -= 1;
            return 0;
        }
        prev_tr = current_tr;
    }

    AUDIT
        printk(KERN_DEBUG "%s: Finished removing thread metadata", TAG_RECEIVE);
    return PID_NF;
}


int clean_up_metadata(struct tag_service *tag_service, struct tag_levels_list *rcvng_level)
{
    //when the thread wakes up, he needs to free
    //all the allocated data.
    int ret;

    //level_x_ray();

    //if the thread wasn't the last on the level,
    //he has to free ONLY the receiving_threads entry
    spin_lock(&rcvng_level->level.lock);
    if(rcvng_level->level.waiting_threads > 1){
        ret = remove_thread_metadata(rcvng_level);
        spin_unlock(&rcvng_level->level.lock);
        return ret;
    }

    //otherwise, locking the "situation" on the thread level
    spin_lock(&tag_service->lvl_spin);

    //if the thread was the last standing (on the tag service) he has to
    //delete the entire tag_levels_list
    if(tag_service->tag_levels == NULL){
        printk(KERN_ALERT "%s: tag_service->tag_levels is NULL", TAG_RECEIVE);
        spin_unlock(&tag_service->lvl_spin);
        return UNEXPECTED;
    }
    if(tag_service->tag_levels->next == NULL){
        AUDIT
            printk(KERN_DEBUG "%s: Last thread on this tag service", TAG_RECEIVE);
        kfree(rcvng_level->level.threads);
        kfree(tag_service->tag_levels);
        tag_service->tag_levels = NULL;
        spin_unlock(&tag_service->lvl_spin);
        return 0;
    }

    //If the thread was the last on its level (but not on the tag service) he has to
    //delete only his tag_levels_list entry.
    //Removing the level metadata
    AUDIT
        printk(KERN_DEBUG "%s: Last thread on this level", TAG_RECEIVE);
    if(rcvng_level->prev == NULL){
        tag_service->tag_levels = rcvng_level->next;
        tag_service->tag_levels->prev = NULL;
    }else{
        rcvng_level->prev->next = rcvng_level->next;
        if(rcvng_level->next != NULL){
            rcvng_level->next->prev = rcvng_level->prev;
        }
    }
    kfree(rcvng_level->level.threads);
    kfree(rcvng_level);
    spin_unlock(&tag_service->lvl_spin);

    return 0;
}


//function that uses a sleep wait condition queue
int receive(struct tag_levels_list *rcvng_level)
{
    //form now on, the thread will wake up wheter
    //1. he's hit by a signal
    //2. he's wake up by tag_ctl
    //3. some data arrive
    int old_data, old_awake;
    struct tag_service *tag_serv;

    old_data = rcvng_level->level.data_received;
    tag_serv = container_of(&rcvng_level, struct tag_service, tag_levels);
    old_awake = tag_serv->awake_all;

    #ifdef WAIT_EV_TO
    //Used before tag_send was implemented
    wait_event_interruptible_timeout(receiving_queue, (old_data != rcvng_level->level.data_received) || (old_awake != tag_serv->awake_all), msecs_to_jiffies(SEC_EV_TO*1000));
    #else
    wait_event_interruptible(receiving_queue, (old_data != rcvng_level->level.data_received) || (old_awake != tag_serv->awake_all));
    #endif

    //distinguish return codes
    if(old_data != rcvng_level->level.data_received){
        return 0;
    }else if(old_awake != tag_serv->awake_all){
        return THREAD_WOKE_UP;
    }else{
        //or timer if WAIT_EV_TO is defined
        return SIGNAL_ARRIVED;
    }
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(4, _tag_receive, int, tag, int, level, char*, buffer, size_t, size){
#else
asmlinkage int sys_tag_receive(int tag, int level, char* buffer, size_t size)
{
#endif

    int orig_descriptor, descriptor, ret, ret_rcv;
    struct tag_service *tag_service;
    struct tag_levels_list *rcvng_level;
    orig_descriptor = tag;
    /*
    if(try_module_get(THIS_MODULE) == 0){
		return MOD_INUSE;
	}
    */

    //here we want to be as fast as possible to (eventually) arrive
    //sooner than the remover
    preempt_disable();
    if((descriptor = check_descriptor(tag, TAG_CTL)) < 0){
        //spin_unlock(&tag_tbl_spin);
        preempt_enable();
        return descriptor;
    }
    preempt_enable();


    tag_service = tag_table[descriptor];

    //if this one is not satisfied then the lock
    //was already acquired by the remover
    if(down_trylock(&semaphores[descriptor])){
        //resetting the value to 0
        down(&semaphores[descriptor]);
        return BEING_DELETED;
    }
    //this means that the remover ops has not started.
    //Resetting value to previous +1
    up(&semaphores[descriptor]);
    up(&semaphores[descriptor]);
    AUDIT
        printk(KERN_DEBUG "%s: Semaphore count %u", TAG_RECEIVE, semaphores[descriptor].count);

    //TODO postponing cleaner timer to corresponding descriptor

    //from here in, the cleaner wont wake up for CLEANER_SLEEP seconds
    //on this tag table entry

    //checking password, if needed
    if((ret = check_password(tag_service, orig_descriptor)) != 0){
        down(&semaphores[descriptor]);
        tag_error(ret, TAG_RECEIVE);
        return ret;
    }
    //checking permission
    if((ret = check_permission(tag_service)) != 0){
        down(&semaphores[descriptor]);
        tag_error(ret, TAG_RECEIVE);
        return ret;
    }

    //putting metadata into the right place
    if((rcvng_level = put_receive_metadata(tag_service, level, buffer, size)) == NULL){
        down(&semaphores[descriptor]);
        tag_error(PUT_META_ERR, TAG_RECEIVE);
        return PUT_META_ERR;
    }
    AUDIT
        printk(KERN_DEBUG "%s: Metadata inserted", TAG_RECEIVE);

    //going to sleep (until given coindition is met)
    //the call can fail due to AWAKE ALL ops or signal received
    if((ret_rcv = receive(rcvng_level)) != 0){
        tag_error(ret_rcv, TAG_RECEIVE);
    }

    //if awake, clean data put preavusly
    //AND if this thread was the last listening for data on this levels
    //do the appropiate cleanup operations
    if((ret = clean_up_metadata(tag_service, rcvng_level)) != 0){
        down(&semaphores[descriptor]);
        tag_error(ret, TAG_RECEIVE);
        return ret;
    }

    AUDIT
        printk(KERN_DEBUG "%s: Metadata cleaned-up", TAG_RECEIVE);
    //decreasing semaphore count
    down(&semaphores[descriptor]);

    //module_put(THIS_MODULE);
    return ret_rcv;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
static unsigned long sys_tag_receive = (unsigned long) __x64_sys_tag_receive;
#else
#endif
