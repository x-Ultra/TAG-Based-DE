#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>

#include "../macro_file_path.h"

//tag_service constants
#include "../../include/constants.h"

void main(int argc, char **argv)
{

	int key, desc, ret, level;
    char *buffer = (char *)malloc(RW_BUFFER_SIZE-1);

    if(argc != 3){
        printf("UsageT: ./spawn_receiver <tag descriptor> <tag level>\n");
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

        printf("Receiving on descriptor %d, level %d\n", desc, level);
        ret = tag_receive(desc, level, buffer, RW_BUFFER_SIZE-1);

        if(ret >= 0){
            printf("Received %s\n", buffer);
        }else{
            printf("Tag Receiving Failed (ret: %d)\n", ret);
        }

		sleep(1);
    }

}
