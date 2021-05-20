


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(4, _tag_send, int, tag, int, level, char*, buffer, size_t, size){
#else
asmlinkage int sys_tag_send(int tag, int level, char* buffer, size_t size)
{
#endif

    int descriptor;
    struct tag_service *tag_service;

    if((descriptor = check_input_data_head(tag)) < 0){
        //descriptor contains the error code
        return descriptor;
    }
    tag_service = tag_table[descriptor];

    //insert here the magic
    //TODO

    if(check_input_data_tail(descriptor) != 0){
        printk(KERN_ERR "%s: check_input_data_tail is nor zero", TAG_SEND);
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
