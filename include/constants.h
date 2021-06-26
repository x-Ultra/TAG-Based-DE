//number of tag services (tag table entries)
#define TBL_ENTRIES_NUM 256
//bits reserved to the passwrd. What password ? Check tag_get.h
unsigned long PRIV_PWD_BITS;
unsigned int positron = 1;
#define RW_BUFFER_SIZE 4096

//Variable used by cleaner
#define CLEANER_SLEEP_SEC 10
#define UNUSED_SECS 60
#define CLEANER "TAG-Cleaner"

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
#define ERR_SPRINTF -3004

//common errors on syscalls
#define INVALID_CMD -1
#define INVALID_EUID -2
#define KEY_NOT_FOUND -3
#define BEING_DELETED -4
#define WRONG_PWD -5
#define INVALID_DESCR -6

//tag_get error codes
#define KEY_USED -1001
#define PRIVATE_OPEN -1002
#define KEY_RESERVED -1003
#define SERVICE_SETUP_FAIED -1004
#define TAG_TBL_FULL -1005

//tag_ctl error codes
#define SERVICE_IN_USE -2001

//tag_receive error codes
#define PUT_META_ERR -4001
#define PID_NF -4002
#define THREAD_WOKE_UP -4003
#define SIGNAL_ARRIVED -4004

//tag_send error codes
#define BUFF_TOO_LARGE -5001
#define CPY_ERR -5002

//seconds to wait if bruteforce detected
#define BRUTE_SLEEP 3

#define DEBUG
#define WAIT_EV_TO
//secconds to sleep in the wait queue during a tag_receive
//if WAIT_EV_TO is defined (used for testing)
#define SEC_EV_TO 5

#ifdef DEBUG
#define AUDIT if(1)
#else
#define AUDIT if(0)
#endif
