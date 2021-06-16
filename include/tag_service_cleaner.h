int cleaner_stop = 0;

//The cleaner is a kerner thread that
int tag_service_cleaner(void *unused)
{
    int i, the_key;

    while(1){
        //Go to sleep using contition queue on cleaner_data
        wait_event_interruptible_timeout(cleaner_wq, cleaner_stop == 1, msecs_to_jiffies(CLEANER_SLEEP_SEC*1000));
        if(cleaner_stop == 1)
            return 0;
        AUDIT
            printk(KERN_DEBUG "%s: Cleaner woke up", CLEANER);

        //no new insertion permitted during cleaner checks
        spin_lock(&tag_tbl_spin);
        //checks if some table entry was unused for UNUSED_SECS
        for(i = 0; i < TBL_ENTRIES_NUM; i++){

            //if there is live on this tag service
            if(tag_table[i] != NULL){
                //decrementing 'TTL'
                unused_tag_services[i] -= CLEANER_SLEEP_SEC;
                AUDIT
                    printk(KERN_DEBUG "%s: Unused secs: %d, in %d", CLEANER, unused_tag_services[i], i);

                //this condition means that noone has used the tag service for UNUSED_SECS
                //the cleaner will proceed to remove it
                if(unused_tag_services[i] <= 0){
                    //removing
                    AUDIT
                        printk(KERN_DEBUG "%s: Semaphore count %u, in %d", CLEANER, semaphores[i].count, i);
                    if(!down_trylock(&semaphores[i])){
                        //if i am able to acquire 2 time the semaphore,
                        //some thread has the tag service.
                        if(!down_trylock(&semaphores[i])){
                            up(&semaphores[i]);
                            up(&semaphores[i]);
                            //before continuing, it may happen that a thread
                            //cas killed befor incresing sem count...
                            //So if the unused counter is negative NO threads has
                            //accessed the sem counter for UNUSED_SECS.
                            //Resetting that value to zero
                            semaphores[i].count = 1;
                            continue;
                        }
                    }else{
                        //if i am not able to acquire the semaphore for the first time,
                        //another removing ops for this service has been called
                        continue;
                    }

                    unused_tag_services[i] = UNUSED_SECS;
                    //removing the descriptor on the used_keys in such a way to
                    //have '-1' entries ONLY at the end of the table
                    //checking that key is in used_keys
                    if(tag_table[i]->key != TAG_IPC_PRIVATE){
                        the_key = tag_table[i]->key;
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
                    if(tag_table[i]->tag_levels != NULL)
                        kfree(tag_table[i]->tag_levels);
                    kfree(tag_table[i]);
                    tag_table[i] = NULL;

                    printk(KERN_NOTICE "%s: Cleaner has removed tag_service %d", CLEANER, i);

                }
                //done
                up(&semaphores[i]);
            }
        }

        spin_unlock(&tag_tbl_spin);
    }
}
