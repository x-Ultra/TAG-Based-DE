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

//tag_get error codes
#define KEY_USED 0
#define PRIVATE_OPEN 1
#define INVALID_CMD 2

#ifdef DEBUG
#define AUDIT if(1)
#else
#define AUDIT if(0)
#endif
