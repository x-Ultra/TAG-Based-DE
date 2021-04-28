//number of tag services (tag table entries)
#define TBL_ENTRIES_NUM 256

//prsmission types
#define PERMISSION_USER 0
#define PERMISSION_ANY 1

//Commands types
#define CMD_OPEN 0
#define CMD_CREATE 1

//ipc private constants
#define TAG_IPC_PRIVATE 0
//12 bit -> 2^12=4096 IPC private tag services maximum
//their descriptor has to go from 0 to (2^12)-1
#define PRIV_TAG_BIT 12
//this has to be 2^PRIV_TAG_BIT !
#define MAX_PRIVATE_TAGS 4096

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
