#include "constants/constants.h"


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
    unsigned long ipc_private_check;
    struct receiving_threads threads;
};


struct tag_service{

    int key;
    pid_t creator_pid;
    uid_t creator_uid;
    int permission;
    struct tag_level *tag_levels[TAG_LEVELS_NUM];
};


//TODO check allignment and false cache sharing
struct tag_service *tag_table[TBL_ENTRIES_NUM];
