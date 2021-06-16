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

    short data_received;
    unsigned int waiting_threads;
    struct receiving_threads *threads;
    spinlock_t lock;
};


struct tag_levels_list{

    struct tag_levels_list *next;
    struct tag_levels_list *prev;
    struct tag_level level;
    int level_num;
};


struct tag_service{

    int key;
    pid_t creator_pid;
    kuid_t creator_euid;
    int permission;
    unsigned int ipc_private_pwd;
    spinlock_t lvl_spin;
    struct tag_levels_list *tag_levels;
    short awake_all;
};

DECLARE_WAIT_QUEUE_HEAD(receiving_queue);
DECLARE_WAIT_QUEUE_HEAD(badguys_queue);

//TODO check allignment and false cache sharing
struct tag_service *tag_table[TBL_ENTRIES_NUM] __attribute__((aligned(8)));
struct semaphore semaphores[TBL_ENTRIES_NUM] __attribute__((aligned(64)));
//spinlock used by soft irq (cleaner) and kernel thread. Used spin_lock_hb() form kernel thread !
//spinlock used during creation/removing procedure of a tag_table entry
DEFINE_SPINLOCK(tag_tbl_spin);
int used_keys[TBL_ENTRIES_NUM];
int num_used_keys = 0;

//stuff used by cleaner thread
DECLARE_WAIT_QUEUE_HEAD(cleaner_wq);
//no syncronizaiton here, no need to align (to avoid fcs)
short unused_tag_services[TBL_ENTRIES_NUM] = { [0 ... TBL_ENTRIES_NUM-1] = UNUSED_SECS};
