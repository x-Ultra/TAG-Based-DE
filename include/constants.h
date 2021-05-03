//number of tag services (tag table entries)
#define TBL_ENTRIES_NUM 256
//bits reserved to the passwrd. What password ? Check tag_get.h
unsigned long PRIV_PWD_BITS;

//prsmission types
#define PERMISSION_USER 0
#define PERMISSION_ANY 1

//Commands types
#define CMD_OPEN 0
#define CMD_CREATE 1

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
#define MOD_INUSE -1001
#define ERR_KMALLOC -1002
#define UNEXPECTED -1003

//tag_get error codes
#define KEY_USED -1
#define PRIVATE_OPEN -2
#define INVALID_CMD -3
#define KEY_NOT_FOUND -4
#define INVALID_EUID -5
#define KEY_RESERVED -6
#define SERVICE_SETUP_FAIED -7
#define TAG_TBL_FULL -8
#define DEBUG 1

#ifdef DEBUG
#define AUDIT if(1)
#else
#define AUDIT if(0)
#endif
