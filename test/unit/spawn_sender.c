#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string.h>
#include <sys/syscall.h>

#include "../macro_file_path.h"

//tag_service constants
#include "../../include/constants.h"

void main(int argc, char **argv)
{

	int key, ret, level, desc;
    char *buffer = (char *)malloc(RW_BUFFER_SIZE-1);

    if(argc != 3){
        printf("UsageT: ./spawn_sender <tag descriptor> <tag level>\n");
        return;
    }

	key = atoi(argv[1]);
    level = atoi(argv[2]);

	//permission field is ignored if cmd=CMD_OPEN
	desc = tag_get(key, CMD_OPEN, 13221);
	if(desc < 0){
		printf("Error in opening The Tag service to the key provided\n");
		sleep(1);
		return;
	}

    while(1){

        printf("Type what do you want to send: ");
        fgets(buffer, RW_BUFFER_SIZE-1, stdin);
        printf("Sending %s on descriptor %d, level %d\n", buffer, desc, level);
        sleep(1);
        ret = tag_send(desc, level, buffer, strlen(buffer)+1);

        if(ret >= 0){
            printf("Sent !\n");
        }else{
            printf("Sending operation failed !\n");
        }
    }

}
