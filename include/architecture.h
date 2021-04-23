#include "constants.h"


//TODO ALLIGN TO CACHE LINES
//tag_table entries
//tag_service
//levels ?


struct thread_tagdata{

    //TODO
    int changemelater;
};


struct receiving_threads{

    struct receiving_threads *next;
    struct thread_tagdata data;
};


struct tag_level{

    unsigned int waiting_threads;
    struct receiving_threads threads;
};


struct tag_levels_list{

    struct tag_level_list *next;
    struct tag_level_list *prev;
    struct tag_level level;
    //for a faster lookup
    struct int level_num;
};


struct tag_service{

    int key;
    pid_t creator_pid;
    uid_t creator_euid;
    int permission;
    unsigned long ipc_private_check;
    struct tag_level *tag_levels_list;
    spinlock_t removing;
};


//TODO check allignment and false cache sharing
struct tag_service *tag_table[TBL_ENTRIES_NUM] __attribute__((align(8)));
DEFINE_MUTEX(tag_tbl_mtx);
