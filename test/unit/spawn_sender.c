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

	int desc, ret, level;
    char *buffer = (char *)malloc(RW_BUFFER_SIZE);

    if(argc != 3){
        printf("UsageT: ./spawn_sender <tag descriptor> <tag level>\n");
        return;
    }

	desc = atoi(argv[1]);
    level = atoi(argv[2]);

    while(1){

        printf("Type what do you want to send: ");
        gets(buffer);
        printf("Sending %s on descriptor %d, level %d\n", buffer, desc, level);
        sleep(1);
        ret = tag_send(desc, level, buffer, strlen(buffer));

        if(ret >= 0){
            printf("Sent !\n");
        }else{
            printf("Sending operation failed !\n");
        }
    }

}
