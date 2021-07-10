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
	int key, ret, desc;

    if(argc != 2){
        printf("UsageT: ./awake_all <KEY> \n");
		return;
    }

	key = atoi(argv[1]);
    //permission field is ignored if cmd=CMD_OPEN
	desc = tag_get(key, CMD_OPEN, 13221);
	if(desc < 0){
		printf("Error in opening The Tag service to the key provided\n");
		sleep(1);
		return;
	}

	ret = tag_ctl(desc, AWAKE_ALL);
    if(ret >= 0){
        printf("TAG Service thread awaken correctly\n");
    }else{
        printf("Awake all failed (ret = %d)\n", ret);
    }

}
