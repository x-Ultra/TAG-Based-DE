#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>

#include "../macro_file_path.h"

//tag_service constants
#include "../../include/constants.h"

int main(int argc, char **argv)
{

	int ret, key, command, permission;
	int desc1, desc2;
	command = CMD_CREATE;
	permission = PERMISSION_ANY;

	key = 30;
	desc1 = tag_get(key, command, permission);
	printf("tag_ctl key1 returned %d\n", desc1);

	//private keys
	desc2 = tag_get(TAG_IPC_PRIVATE, command, permission);
	printf("tag_ctl Private 1 returned %d\n", desc2);

	ret = tag_receive(desc1, 1, NULL, 2);
	printf("receiving on desc3 %d\n", ret);

	ret = tag_receive(desc2, 1, NULL, 2);
	printf("receiving on desc4 %d\n", ret);


    ret = tag_ctl(desc1, REMOVE);
    printf("removing desc4 %d\n", ret);

    ret = tag_ctl(desc2, REMOVE);
    printf("removing desc4 %d\n", ret);

	return 0;
}
