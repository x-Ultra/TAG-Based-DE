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
};


struct tag_levels_list{

    struct tag_levels_list *next;
    struct tag_levels_list *prev;
    struct tag_level level;
    //for a faster lookup, if needed extract fron tam_level with 'containerof'
    int level_num;
};


struct tag_service{

    int key;
    pid_t creator_pid;
    kuid_t creator_euid;
    int permission;
    unsigned int ipc_private_pwd;
    struct tag_levels_list *tag_levels;
};


//TODO check allignment and false cache sharing
struct tag_service *tag_table[TBL_ENTRIES_NUM] __attribute__((aligned(8)));
//spinlock used by soft irq (cleaner) and kernel thread. Used spin_lock_hb() form kernel thread !
DEFINE_SPINLOCK(tag_tbl_spin);
int used_keys[TBL_ENTRIES_NUM];
int num_used_keys = 0;
