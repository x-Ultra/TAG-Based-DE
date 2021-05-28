//function that does the actual copy of the metadata from the sender to receiver
void send_to_thread(struct thread_rcvdata *thread_metadata, char* buff, size_t len)
{

    //TODO edit here
   char buffer[LINE_SIZE];
   long pid;
   unsigned long addr;
   long val;
   int format = 0;
   int i,j;
   int ret = len;

   char* args[3];

   struct task_struct *the_task;
   void** restore_pml4;
   unsigned long restore_cr3;
   struct mm_struct *restore_mm;

      if(len >= LINE_SIZE) return -1;
      ret = copy_from_user(buffer,buff,len);

      j = 1;
      for(i=0;i<len;i++){
    	if(buffer[i] == ' ') {
    		buffer[i] = '\0';
    		args[j++] = buffer + i + 1;
    		format++;
    	}
      }

      if(format != 2) return -EBADMSG;//bad message

      args[0] = buffer;

      buffer[len] = '\0';
      ret = kstrtol(args[0],10,&pid);
      ret = kstrtol(args[1],10,&addr);
      ret = kstrtol(args[2],10,&val);

      AUDIT
      printk("%s: args are: %ld - %lu - %ld\n",DEVICE_NAME,pid,addr,val);


      restore_pml4 = (unsigned long)current->mm->pgd;
      restore_mm = (unsigned long)current->mm;
      restore_cr3 = _read_cr3();

      rcu_read_lock();

      the_task = pid_task(find_vpid(pid),PIDTYPE_PID);

      if(!the_task){
      	rcu_read_unlock();
    	return -ESRCH;//no such process
      }


      if(!(the_task->mm)){
      	rcu_read_unlock();
            return -ESRCH;//no such process
      }
      atomic_inc(&(the_task->mm->mm_count));//to be released on failures of below parts

      rcu_read_unlock();

      if(!(the_task->mm->pgd)){
      	atomic_dec(&(the_task->mm->mm_count));
            return  -ENXIO;//no such device or address
      }

      AUDIT
      printk("%s: process %ld is there with its memory map\n",DEVICE_NAME,pid);

      current->mm = the_task->mm;//this is needed to correctly handle empty zero memory access or other faults
      _write_cr3(__pa(the_task->mm->pgd));

      AUDIT
      printk("%s: current moved to the address space of process %ld\n",DEVICE_NAME,pid);

      //do the hack
      ret = copy_to_user((void*)addr,(void*)&val,sizeof(val));

      //resume my own face
      current->mm = restore_mm;
      _write_cr3(restore_cr3);

      AUDIT
      printk("%s: current moved back to its own address space\n",DEVICE_NAME);

      atomic_dec(&(the_task->mm->mm_count));

      return len;

}


int send_data(struct tag_service *tag_service, int level, char* buffer, size_t size)
{
    struct tag_levels_list *target_level;
    strunc receiving_threads *target_thread;

    //0. Fetching tag_level
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
        send_to_thread(&target_thread.data, buffer, size);
    }

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
