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


void malicious_process()
{
    int sniffed_descriptor = 0;
    int ret;

    //Lets try to delete the tag service ! >:)
    ret = tag_ctl(sniffed_descriptor, REMOVE);
	printf("removing desc returned %d\n", ret);

    //Lets try to receive on the tag service ! >:)
    ret = tag_receive(sniffed_descriptor, 0, NULL, 0);
	printf("receiving on desc returned %d\n", ret);

    return;

}

int main(int argc, char **argv)
{

	int ret;
	int desc;

    //the father creates a private ag service
	desc = tag_get(TAG_IPC_PRIVATE, CMD_CREATE, PERMISSION_ANY);
	printf("tag_ctl returned %d\n", desc);

    //lets suppose another process try to ajack the service,
    //afther that he knows that the corresponding tag table is at position 0.

    if(!fork()){
        malicious_process();
    }

    wait(NULL);

	ret = tag_ctl(desc, REMOVE);
	printf("removing desc %d\n", ret);


	return 0;
}
