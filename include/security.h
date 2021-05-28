/*
    Header containing fnction used to prevent brute force attacks to the tag service
*/

void prevent_bruteforce(char *char_module)
{

    char *gotcha_message = "\033[1;31mStop doing what you're doing... i see you !\n\033[0m\n";
    struct tty_struct *tty;
    tty = current->signal->tty;

    printk(KERN_ALERT "%s: Bruteforce detected", char_module);
    if(tty != NULL) {
        (tty->driver->ops->write) (tty, gotcha_message, strlen(gotcha_message));
    }

    wait_event_interruptible_timeout(badguys_queue, 0, msecs_to_jiffies(BRUTE_SLEEP*1000));

}


int check_descriptor(int descriptor, char* caller)
{
    int priv_descriptor;

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
            printk(KERN_ERR "%s: tag_table[descriptor] is NULL", caller);
        return INVALID_DESCR;
    }

    return descriptor;
}


int check_password(struct tag_service *tag_service, int descriptor)
{
    int pwd;

    if(tag_service->key == TAG_IPC_PRIVATE){
        pwd = descriptor >> (sizeof(unsigned int)*8 - PRIV_PWD_BITS);
        if(pwd != tag_service->ipc_private_pwd){
            return WRONG_PWD;
        }
    }

    return 0;
}


int check_permission(struct tag_service *tag_service)
{
    kuid_t EUID;

    if(tag_service->permission == PERMISSION_USER){
        EUID = current_euid();
        if(!uid_eq(EUID, tag_service->creator_euid)){
            return INVALID_EUID;
        }
    }

    return 0;
}
