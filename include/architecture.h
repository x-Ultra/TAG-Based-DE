#include "constants.h"

//TODO ALLIGN TO CACHE LINES
//tag_table entries
//tag_service
//levels ?


struct thread_rcvdata{

    pid_t pid;
    char* buffer;
    size_t size;
};


struct receiving_threads{

    struct receiving_threads *next;
    struct thread_rcvdata data;

};


struct tag_level{

    unsigned int waiting_threads;
    struct receiving_threads threads;
    spinlock_t lock;
};


struct tag_levels_list{

    struct tag_levels_list *next;
    struct tag_levels_list *prev;
    struct tag_level level;
    //for a faster lookup, if needed extract from tag_level with 'containerof'
    int level_num;
};


struct tag_service{

    int key;
    pid_t creator_pid;
    kuid_t creator_euid;
    int permission;
    unsigned int ipc_private_pwd;
    struct tag_levels_list *tag_levels;
    //semaphore increased every time a thread needs to access a level
    //used to prevent removing ops
    struct semaphore sem;
};


//TODO check allignment and false cache sharing
struct tag_service *tag_table[TBL_ENTRIES_NUM] __attribute__((aligned(8)));
//spinlock used by soft irq (cleaner) and kernel thread. Used spin_lock_hb() form kernel thread !
//spinlock used during creation/removing procedure of a tag_table entry, because
//these ops require to access more than one entry (Insted of locking each spinlock
//on each entry we prefer locking just one time, to increase performaces).
DEFINE_SPINLOCK(tag_tbl_spin);
int used_keys[TBL_ENTRIES_NUM];
int num_used_keys = 0;
