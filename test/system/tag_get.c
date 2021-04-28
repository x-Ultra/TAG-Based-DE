#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>

#include "/home/ezio/custom_syscall_macros.h"

//tag_service constants
#include "../../include/constants.h"

int main(int argc, char **argv)
{

	int ret, key, command, permission;
	command = CMD_CREATE;
	permission = PERMISSION_ANY;

	key = 30;
	ret = tag_get(key, command, permission);
	printf("Syscall returned %d\n", ret);

	//private key
	//ret = tag_get(TAG_IPC_PRIVATE, command, permission);
	//printf("Syscall returned %d\n", ret);

	return 0;
}
