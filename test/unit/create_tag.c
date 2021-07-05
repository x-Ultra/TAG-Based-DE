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

	int key, permission;
	int desc;

    if(argc != 3){
        printf("UsageT: ./create_tag <PERMISSION> <TAG_KEY>\n");
		return;
    }

	permission = atoi(argv[1]);
	key = atoi(argv[2]);

	desc = tag_get(key, CMD_CREATE, permission);
    if(desc >= 0){
        printf("Created tag on descriptor %d\n", desc);
    }else{
        printf("Tag Creation failed !\n");
    }

}
