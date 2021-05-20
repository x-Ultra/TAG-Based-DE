//function that audit tag_get error repending on (negative) return code
void tag_error(int errorcode, char* modname)
{
    switch(errorcode){
        case INVALID_CMD:
            printk(KERN_ERR "%s: Command value incorrect", modname);
            break;
        case SERVICE_IN_USE:
            printk(KERN_ERR "%s: There are still some threads waiting for data on this service", TAG_CTL);
            break;
        case INVALID_DESCR:
            printk(KERN_ERR "%s: Invalid descriptor", TAG_CTL);
            break;
        case WRONG_PWD:
            printk(KERN_ERR "%s: Wrong password, unable to delete tag service", TAG_CTL);
            prevent_bruteforce(TAG_CTL);
            break;
        case PUT_META_ERR:
            printk(KERN_ERR "%s: Error in put_receive_metadata", TAG_RECEIVE);
            break;
        case PID_NF:
            printk(KERN_ALERT "%s: Pid not found during cleanup procedure", TAG_RECEIVE);
            break;
        case THREAD_WOKE_UP:
            printk(KERN_ALERT "%s: Receive interrupted by wake up operation", TAG_RECEIVE);
            break;
        case SIGNAL_ARRIVED:
            printk(KERN_ALERT "%s: Receive interrupted signal", TAG_RECEIVE);
            break;
        case KEY_USED:
            printk(KERN_ERR "%s: Key was already used", TAG_GET);
            break;
        case PRIVATE_OPEN:
            printk(KERN_ERR "%s: Tag Open on private key not permitted", TAG_GET);
            break;
        case KEY_NOT_FOUND:
            printk(KERN_ERR "%s: Key was not found", modname);
            prevent_bruteforce(modname);
            break;
        case BEING_DELETED:
            printk(KERN_ERR "%s: The tag service is being deleted", modname);
            break;
        case INVALID_EUID:
            printk(KERN_ERR "%s: Invalid EUID", modname);
            break;
        case MOD_INUSE:
            printk(KERN_ERR "%s: Module in use", modname);
            break;
        case KEY_RESERVED:
            printk(KERN_ERR "%s: Key -1 is reserved", TAG_GET);
            break;
        case ERR_KMALLOC:
            printk(KERN_ERR "%s: Unable to kmalloc", modname);
            break;
        case SERVICE_SETUP_FAIED:
            printk(KERN_ERR "%s: Unable to setup new tag service", TAG_GET);
            break;
        case TAG_TBL_FULL:
            printk(KERN_ERR "%s: Tag Table is full, try again later", TAG_GET);
            break;
        case UNEXPECTED:
            printk(KERN_ALERT "%s: Unexpected error, check previous message", modname);
            break;
        default:
            printk(KERN_ALERT "%s: Unknown error", modname);
    }
}


//function used to scan the tag table
void level_x_ray(void){

    int i, j;
    char *submod = "X-RAY";
    struct tag_levels_list *cur_levels;
    struct receiving_threads *cur_tr;

    //function that scans the WHOLE tag Table
    for(i = 0; i < TBL_ENTRIES_NUM; i++){

        if(tag_table[i] == NULL){
            continue;
        }
        printk(KERN_DEBUG "%s: Table entry %d", submod, i);

        //scanning levels information
        cur_levels = tag_table[i]->tag_levels;
        while(1){
            if(cur_levels == NULL){
                printk(KERN_DEBUG "%s: Table Entry %d IS EMPTY", submod, i);
                break;
            }

            //per each level print informations
            printk(KERN_DEBUG "%s: Level %d, Waiting Threads: %d, Waiting thread PIDs:", submod, cur_levels->level_num, cur_levels->level.waiting_threads);
            //waiting pid:

            cur_tr =  cur_levels->level.threads;
            for(j = 0; j < cur_levels->level.waiting_threads; j++){
                if(cur_tr == NULL){
                    printk(KERN_ALERT "%s: Warining, threads data structure is NULL", submod);
                    continue;
                }
                printk(KERN_DEBUG "%s: Thread PID: %u", submod, cur_tr->data.pid);
                cur_tr = cur_tr->next;
            }

            cur_levels = cur_levels->next;
            if(cur_levels == NULL)
                break;
        }
    }

    return;
}