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
	int desc1;
	command = CMD_CREATE;
	permission = PERMISSION_ANY;
	int f;

	if(argc != 2){
		printf("./tag_receive <num_processes>");
		return -1;
	}

	int NUM_PROC = atoi(argv[1]);


	for(int k = 0; k < 2; k++){
		if(k){
			printf("Testing on normal keys\n");
			key = 30;
			desc1 = tag_get(key, command, permission);
			printf("tag_ctl key1 returned %d\n", desc1);
		}else{
			//private keys
			printf("Testing on private keys\n");
			desc1 = tag_get(TAG_IPC_PRIVATE, command, permission);
			printf("tag_ctl Private 1 returned %d\n", desc1);
		}
		sleep(2);
		printf("Starting testing...\n");
		//Spawning process
		for(int j = 0; j < 2; j++){
			if(j){
				printf("waiting all on the same level test\n");
			}else{
				printf("waiting all on a different level test\n");
			}
			sleep(2);
			printf("Starting spawning...\n");
			for(int i = 0; i < NUM_PROC; i++){
				f = fork();
				if(f == 0){
					//the child will receive somthing
					//NB: tag_get(cmd=OPEN) not needed, we altrady hacve the descriptor
					//waiting all on the same level test
					if(j){
						printf("Child %d waiting on level 1\n", i);
						ret = tag_receive(desc1, 1, NULL, 2);
						printf("receiving on desc1 %d\n", ret);
						return 0;
					}else{
						//waiting on different level test, OK
						printf("Child %d waiting on level %d\n", i, i);
						ret = tag_receive(desc1, i, NULL, 2);
						printf("receiving on desc1 %d\n", ret);
						return 0;
					}
					sleep(2);
				}else if(f == -1){
					return -1;
				}else{
					//the father
					continue;
				}
			}
			printf("\n\n");
			//the fathes has generated NUM_PROC
			for(int i = 0; i < NUM_PROC; i++)
				wait(NULL);
		}

		//removing tag_service
		ret = tag_ctl(desc1, REMOVE);
	    printf("removing desc1 %d\n", ret);
	}

	return 0;
}
