/* auxiliary stuff */
static inline void _write_cr3(unsigned long val) {

          asm volatile("mov %0,%%cr3": : "r" (val));

}

static inline unsigned long _read_cr3(void) {

	  unsigned long val;
          asm volatile("mov %%cr3,%0":  "=r" (val) : );
	  return val;

}


//function that does the actual copy of the metadata from the sender to receiver
void send_to_thread(struct thread_rcvdata *thread_metadata, char* kern_buff, size_t len)
{
    int ret, min_size;
    struct task_struct *the_task;
    void** restore_pml4;
    unsigned long restore_cr3;
    struct mm_struct *restore_mm;
    long pid = (long)thread_metadata->pid;

    //suppressing warnings
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wint-conversion"
    restore_pml4 = (unsigned long)current->mm->pgd;
    restore_mm = (unsigned long)current->mm;
    restore_cr3 = _read_cr3();
    #pragma GCC diagnostic pop

    rcu_read_lock();

    //TODO check this
    the_task = pid_task(find_vpid(pid), PIDTYPE_PID);
    AUDIT
        printk(KERN_DEBUG "%s: Sending at pid %ld", TAG_SEND, pid);

    if(!the_task){
      	rcu_read_unlock();
        printk(KERN_ERR "%s: Process with pid %ld not found", TAG_SEND, pid);
    	return;
    }

    //process not found
    if(!(the_task->mm)){
    	rcu_read_unlock();
        return;
    }
    atomic_inc(&(the_task->mm->mm_count));//to be released on failures of below parts

    rcu_read_unlock();

    //no such device or address
    if(!(the_task->mm->pgd)){
    	atomic_dec(&(the_task->mm->mm_count));
        return;
    }

    current->mm = the_task->mm;//this is needed to correctly handle empty zero memory access or other faults
    _write_cr3(__pa(the_task->mm->pgd));

    AUDIT
        printk(KERN_DEBUG "%s: current moved to the address space of process %ld\n", TAG_SEND, pid);

    //in this way receiver could use a buffer size > RD_BUFF_SIZE
    //but, obviuusly, only RD_BUFF_SIZE will be copyied
    if(len >= thread_metadata->size){
        min_size = len;
    }else{
        min_size = thread_metadata->size;
    }
    //copying to user space
    ret = copy_to_user((void*)thread_metadata->buffer, (void *)kern_buff, min_size);
    //resume my own face
    current->mm = restore_mm;

    _write_cr3(restore_cr3);

    AUDIT
        printk("%s: current moved back to its own address space", TAG_SEND);

    atomic_dec(&(the_task->mm->mm_count));
}


int send_data(struct tag_service *tag_service, int level, char* buffer, size_t size)
{
    struct tag_levels_list *target_level;
    struct receiving_threads *target_thread;
    char *kern_buffer;

    if((kern_buffer = kmalloc(RW_BUFFER_SIZE, GFP_KERNEL)) == NULL){
        return ERR_KMALLOC;
    }

    //Checking buffer size
    if(size >= RW_BUFFER_SIZE){
        return BUFF_TOO_LARGE;
    }

    //Copying from user, the sender buffer
    if(copy_from_user(kern_buffer, buffer, size) != 0){
        return CPY_ERR;
    }

    //Fetching tag_level
    for(target_level = tag_service->tag_levels; ; target_level = target_level->next){
        if(target_level == NULL){
            printk(KERN_ALERT "%s: tag_level is NULL in send ops", TAG_SEND);
            return UNEXPECTED;
        }

        if(target_level->level_num == level){
            break;
        }

    }

    //1. Acquire spinlock level.lock
    //   to synchronize write operations
    spin_lock(&target_level->level.lock);

    //2. do the copy of the data in each receiving thread
    for(target_thread = target_level->level.threads; target_thread != NULL ;target_thread = target_thread->next){
        send_to_thread(&target_thread->data, kern_buffer, size);
    }
    AUDIT
        printk(KERN_DEBUG "%s: Copyied to all receiver", TAG_SEND);

    //3. Increment level.data_received (Beware of buffer overflow)
    if(target_level->level.data_received > (1 << 15) ){
        target_level->level.data_received = 0;
    }else{
        target_level->level.data_received += 1;
    }

    //4. Release lock
    spin_unlock(&target_level->level.lock);

    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(4, _tag_send, int, tag, int, level, char*, buffer, size_t, size){
#else
asmlinkage int sys_tag_send(int tag, int level, char* buffer, size_t size)
{
#endif

    int descriptor, ret;
    struct tag_service *tag_service;

    if((descriptor = check_input_data_head(tag)) < 0){
        //descriptor contains the error code
        return descriptor;
    }
    tag_service = tag_table[descriptor];

    //here i am sure that i am autorized to insert the data
    //in the requested tag service & tag level
    if((ret = send_data(tag_service, level, buffer, size)) != 0){
        tag_error(ret, TAG_SEND);
        return ret;
    }

    if(check_input_data_tail(descriptor) != 0){
        printk(KERN_ERR "%s: check_input_data_tail is non zero", TAG_SEND);
        return UNEXPECTED;
    }

    //module_put(THIS_MODULE);
    //return ret_snd;
    return 0;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
static unsigned long sys_tag_send = (unsigned long) __x64_sys_tag_send;
#else
#endif
