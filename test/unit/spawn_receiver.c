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

	int desc, ret, level;
    char *buffer = (char *)malloc(RW_BUFFER_SIZE);

    if(argc != 3){
        printf("UsageT: ./spawn_receiver <tag descriptor> <tag level>\n");
        return;
    }

	desc = atoi(argv[1]);
    level = atoi(argv[2]);

    while(1){

        printf("Receiving on descriptor %d, level %d\n", desc, level);
        ret = tag_receive(desc, level, buffer, RW_BUFFER_SIZE);

        if(ret >= 0){
            printf("Received %s\n", buffer);
        }else{
            printf("Tag Receiving Failed (ret: %d)\n", ret);
        }

		sleep(1);
    }

}
