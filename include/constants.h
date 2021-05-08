//number of tag services (tag table entries)
#define TBL_ENTRIES_NUM 256
//bits reserved to the passwrd. What password ? Check tag_get.h
unsigned long PRIV_PWD_BITS;
unsigned int positron = 1;

//prsmission types
#define PERMISSION_USER 0
#define PERMISSION_ANY 1

//Commands types (tag_get)
#define CMD_OPEN 0
#define CMD_CREATE 1
//Commands types (tag_ctl)
#define AWAKE_ALL 0
#define REMOVE 1

//ipc private constants
#define TAG_IPC_PRIVATE 0

//used in level initialization
#define NO_TAG_LEVELS -1

//Used in debug
#define TAG_GET "tag_get"
#define TAG_CTL "tag_ctl"
#define TAG_SEND "tag_send"
#define TAG_RECEIVE "tag_receive"
#define TAG_DRIVER "tag_driver"

//generalerror codes
#define MOD_INUSE -3001
#define ERR_KMALLOC -3002
#define UNEXPECTED -3003

//common errors on syscalls
#define INVALID_CMD -1
#define INVALID_EUID -2
#define KEY_NOT_FOUND -3

//tag_get error codes
#define KEY_USED -1001
#define PRIVATE_OPEN -1002
#define KEY_RESERVED -1003
#define SERVICE_SETUP_FAIED -1004
#define TAG_TBL_FULL -1005

//tag_ctl error codes
#define INVALID_DESCR -2001
#define SERVICE_IN_USE -2002

#define DEBUG 1

#ifdef DEBUG
#define AUDIT if(1)
#else
#define AUDIT if(0)
#endif
