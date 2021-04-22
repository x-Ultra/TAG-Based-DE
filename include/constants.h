//nuumber of level of each tag service
#define TAG_LEVELS_NUM 16;

//number of tag services (tag table entries)
#define TBL_ENTRIES_NUM 256;

//prsmission types
#define PERMISSION_USER 0
#define PERMISSION_ANY 1

//Commands types
#define CMD_OPEN 0
#define CMD_CREATE 1

//ipc private key
#define IPC_PRIVATE 0

//Used in debug
#define TAG_GET "tag_get"
#define TAG_CTL "tag_ctl"
#define TAG_SEND "tag_send"
#define TAG_RECEIVE "tag_receive"
#define TAG_DRIVER "tag_driver"

//generalerror codes
#define MOD_INUSE -5
#define ERR_KMALLOC -7
#define MAX_ERR_MESS_LEN 1024

//tag_get error codes
#define KEY_USED -1
#define PRIVATE_OPEN -2
#define INVALID_CMD -3
#define KEY_NOT_FOUND -4
#define INVALID_EUID -5
#define KEY_RESERVED -6

#ifdef DEBUG
#define AUDIT if(1)
#else
#define AUDIT if(0)
#endif


DEFINE_MUTEX(adding_key_mtx);
unsigned int used_keys[TBL_ENTRIES_NUM];
