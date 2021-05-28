#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "../macro_file_path.h"

//tag_service constants
#include "../../include/constants.h"

int main(int argc, char **argv)
{

	int ret, key, command, permission;
	int desc1, desc2, desc3, desc4, f;
	command = CMD_CREATE;
	permission = PERMISSION_ANY;

	int num_receiver = 5;

	key = 30;
	desc1 = tag_get(key, command, permission);
	printf("tag_ctl key1 returned %d\n", desc1);

	char buffer[100];

	for(int i = 0; i < num_receiver; ++i){
		f = fork();
		if(f == 0){
			//child = RECEIVER
			printf("Thread %d, receiging\n", i);
			ret = tag_receive(desc1, 1, buffer, 100);
			printf("Receiver %d has returned: %d, buffer content: %s\n", i, ret, buffer);
			return 0;
		}
	}

    sleep(5);
    //AWAKING ALL
    ret = tag_ctl(desc1, AWAKE_ALL);
	printf("awaing all ret %d\n", ret);

	for(int i = 0; i < num_receiver; ++i)
		wait(NULL);


	ret = tag_ctl(desc1, REMOVE);
	printf("removing desc1 %d\n", ret);

	return 0;
}
