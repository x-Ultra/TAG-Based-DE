#define DRIVER_STAT_TIT         "TAG-key  TAG-creator  TAG-level  Waiting-threads\n"
#define DRIVER_STAT_LINE        "   %d       %d          %d             %d      \n"
#define DRIVER_STAT_LINE_EMPTY  "   %d       %d          --             %d      \n"
#define DEVICE_NAME "TAG Driver"
#define MIN_PAGES 1

//variable used to adapt log size to the tag_service usage
short increment = 0;
short decrement = 0;
//one increment reached the threeshold vaue, a new page will be
//allocated to contain tad service status messages
int threeshold = 10;
int STAT_PAGES = 3;
//initialized at STAT_PAGES
int starting_pages;

//tag service status message (and size)
int info_mess_size;
//initialized at installation
char *tag_service_stat;
char *tag_service_stat_tmp;
int STAT_LINE_LEN;
int TIT_STAT_LINE_LEN;

//mutex used to enanche performance (paradoxically)
DEFINE_MUTEX(driver_mtx);


static int tag_open(struct inode *inode, struct file *file) {
    //nop
    return 0;
}


static int tag_release(struct inode *inode, struct file *file) {
    //nop
    return 0;
}


int compose_statline(int key, int pid, int lvl, int w_thr, int offset)
{
    int ret;

    AUDIT
        printk(KERN_DEBUG "%s: compose stat called", DEVICE_NAME);

    //if the formatted text wuold overflow...
    if(offset + STAT_LINE_LEN + 10 > STAT_PAGES*PAGE_SIZE){
        STAT_PAGES += 1;
        //add one or more page
        tag_service_stat_tmp = tag_service_stat;
        if((tag_service_stat = vmalloc(PAGE_SIZE*STAT_PAGES)) == NULL){
    		printk("%s: vmalloc failed", DEVICE_NAME);
      	  	return -1;
    	}

        //copy to the newbuffer
        memcpy(tag_service_stat, tag_service_stat_tmp, offset);

        //vfree old pages
        vfree(tag_service_stat_tmp);
    }

    if(lvl == -1){
        ret = snprintf(tag_service_stat+offset, STAT_LINE_LEN+1, DRIVER_STAT_LINE_EMPTY, key, pid, w_thr);
    }else{
        ret = snprintf(tag_service_stat+offset, STAT_LINE_LEN, DRIVER_STAT_LINE, key, pid, lvl, w_thr);
    }

    if(ret <= 0){
        return ERR_SPRINTF;
    }

    AUDIT
        printk(KERN_DEBUG "%s: compose  line ret: %d", DEVICE_NAME, ret);

    return ret;
}


int update_tag_service_stat(void)
{
    int i, ret = 0;
    struct tag_levels_list *cur_levels;
    int charcount = TIT_STAT_LINE_LEN;

    //TODO handle spinlocks

    //copying the tile
    memcpy(tag_service_stat, DRIVER_STAT_TIT, TIT_STAT_LINE_LEN);

    //function that scans the WHOLE tag Table
    for(i = 0; i < TBL_ENTRIES_NUM; i++){

        //same logic of 'check_input_data_head'
        if(down_trylock(&semaphores[i])){
            //resetting the value to 0
            down(&semaphores[i]);
            tag_error(BEING_DELETED, TAG_DRIVER);
            return BEING_DELETED;
        }
        //if w're here the remover ops has not started.
        //Resetting value to previous +1
        up(&semaphores[i]);
        up(&semaphores[i]);

        if(tag_table[i] == NULL){
            down(&semaphores[i]);
            continue;
        }

        //scanning levels information
        cur_levels = tag_table[i]->tag_levels;
        spin_lock(&tag_table[i]->lvl_spin);
        while(1){
            if(cur_levels == NULL){
                //a penging opened tag service, but unused...
                if((ret = compose_statline(tag_table[i]->key, tag_table[i]->creator_pid, -1, 0, charcount)) < 0){
                    spin_unlock(&(tag_table[i]->lvl_spin));
                    down(&semaphores[i]);
                    return ret;
                }
                charcount += ret;
                break;
            }

            //per each level store informations
            if((ret = compose_statline(tag_table[i]->key, tag_table[i]->creator_pid, cur_levels->level_num, cur_levels->level.waiting_threads, charcount)) < 0){
                spin_unlock(&(tag_table[i]->lvl_spin));
                down(&semaphores[i]);
                return ret;
            }
            charcount += ret;

            cur_levels = cur_levels->next;
            if(cur_levels == NULL){
                break;
            }
        }
        spin_unlock(&(tag_table[i]->lvl_spin));
        down(&semaphores[i]);
    }

    //terminating the message
    tag_service_stat[charcount] = '\0';
    info_mess_size = charcount;

    //SELF-ADJUSTING
    //have we used more or less than STAT_PAGES ? -> lets adapt
    if(info_mess_size > starting_pages*PAGE_SIZE){
        increment += 1;
        decrement -= 1;
    }else{
        increment -= 1;
        decrement += 1;
    }

    //dynamic adaptation the STAT_PAGES
    if(increment > threeshold){
        STAT_PAGES += 1;
        starting_pages = STAT_PAGES;
        increment = 0;
        decrement = 0;
        printk(KERN_NOTICE "%s: Self-Adjusting (increasing) stat pages at %d", DEVICE_NAME, STAT_PAGES);
    }else if(decrement > threeshold && STAT_PAGES > MIN_PAGES){
        STAT_PAGES -= 1;
        starting_pages = STAT_PAGES;
        increment = 0;
        decrement = 0;
        printk(KERN_NOTICE "%s: Self-Adjusting (decreasing) stat pages at %d", DEVICE_NAME, STAT_PAGES);
    }

    return 0;
}

static ssize_t get_tagservice_stat(struct file *filp, char *buff, size_t len, loff_t *off)
{

    //if the mutex is locked, there is no need to create a new message  log
    //because probably nothing is changed from the lock to the release.
    //This shouldincrease performances and decrase memory accesses
    if(!mutex_trylock(&driver_mtx)){
        mutex_lock(&driver_mtx);
    }else{
        if(update_tag_service_stat() < 0){
            printk(KERN_ERR "%s: Error while updating status message", DEVICE_NAME);
            mutex_unlock(&driver_mtx);
            return -1;
        }
        mutex_unlock(&driver_mtx);
    }

    AUDIT
        printk(KERN_DEBUG "%s: called read: %d", DEVICE_NAME, info_mess_size);

    /* If position is behind the end of a file we have nothing to read */
    if(*off >= info_mess_size){
        return 0;
    }
    /* If a user tries to read more than we have, read only as many bytes as we have */
    if(*off + len > info_mess_size){
        len = info_mess_size - *off;
    }

    if(copy_to_user(buff, tag_service_stat + *off, len) != 0 ){
        return -EFAULT;
    }
    /* Move reading position */
    *off += len;

    return len;
}


//Driver ops
static struct file_operations fops = {
  .owner = THIS_MODULE,//do not forget this
  .read = get_tagservice_stat,
  .open =  tag_open,
  .release = tag_release
};
