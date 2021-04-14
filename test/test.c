#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>

#include "/home/ezio/custom_syscall_macros.h"

int main(int argc, char **argv)
{

	int ret;
	int pippo_user = 77;
	unsigned long addr;

	printf("->%lu\n", (unsigned long)&pippo_user);

	scanf("%lu", &addr);
	ret = pippo((void *)addr);
	printf("Syscall returned %d\n", ret);

	return 0;
}