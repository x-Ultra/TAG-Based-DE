#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#include "../macro_file_path.h"

//tag_service constants
#include "../../include/constants.h"

void main(int argc, char **argv)
{

	int ret, permission, f;
	int desc, rdesc;
    char *buffer = (char *)malloc(RW_BUFFER_SIZE-1);

	desc = tag_get(TAG_IPC_PRIVATE, CMD_CREATE, 0);
    if(desc >= 0){
        printf("Created tag on descriptor %d\n", desc);
    }else{
        printf("Tag Creation failed !\n");
    }
    printf("This is the SECRET descriptor available only to the owner and his childs: %d\n", desc);
    int pwd = desc >> (sizeof(unsigned int)*8 - PRIV_PWD_BITS);
    rdesc = (pwd << (sizeof(unsigned int)*8 - PRIV_PWD_BITS)) | desc;
    printf("This is the PUBLIC descriptor (try to use it to brute force pwd...): %d\n", rdesc);

    int level = 3;

    f = fork();

    if(!f){
        //the receiver
        while(1){

            printf("\nR->Receiving on descriptor %d, level %d\n", desc, level);
            ret = tag_receive(desc, level, buffer, RW_BUFFER_SIZE-1);

            if(ret >= 0){
                printf("\nR->Received %s\n", buffer);
            }else{
                printf("\nR->Tag Receiving Failed (ret: %d)\n", ret);
            }

    		sleep(1);
        }


    }

    f = fork();
    if(!f){
        //the sender
        while(1){

            printf("\nS->Type what do you want to send: ");
            fgets(buffer, RW_BUFFER_SIZE-1, stdin);
            printf("\nS->Sending %s on descriptor %d, level %d\n", buffer, desc, level);
            sleep(1);
            ret = tag_send(desc, level, buffer, strlen(buffer)+1);

            if(ret >= 0){
                printf("\nS->Sent !\n");
            }else{
                printf("\nS->Sending operation failed !\n");
            }
        }


    }

    for(int i = 0; i < 2; i++)
        wait(NULL);
}
