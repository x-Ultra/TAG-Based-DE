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

	desc2 = tag_get(key+1, command, permission);
	printf("tag_ctl key2 returned %d\n", desc2);

	//private keys
	desc3 = tag_get(TAG_IPC_PRIVATE, command, permission);
	printf("tag_ctl Private 1 returned %d\n", desc3);

	desc4 = tag_get(TAG_IPC_PRIVATE, command, permission);
	printf("tag_ctl Private 2 returned %d\n", desc4);

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

	f = fork();

	if(f == 0){
		sleep(1);
		printf("Sender activated\n");
		ret = tag_send(desc1, 1, "ciao", 4);
		printf("Sender returned: %d\n", ret);
		return 0;
	}

	for(int i = 0; i < num_receiver; ++i)
		wait(NULL);


	ret = tag_ctl(desc1, REMOVE);
	printf("removing desc1 %d\n", ret);

	ret = tag_ctl(desc2, REMOVE);
	printf("removing desc2 %d\n", ret);

	ret = tag_ctl(desc3, REMOVE);
	printf("removing desc3 %d\n", ret);

	ret = tag_ctl(desc4, REMOVE);
	printf("removing desc4 %d\n", ret);


	return 0;
}
