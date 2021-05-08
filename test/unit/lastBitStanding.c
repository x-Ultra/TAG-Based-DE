#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


void printBits(unsigned int num){
    unsigned int size = sizeof(unsigned int);
    unsigned int maxPow = 1<<(size*8-1);
    int i=0,j;
    for(;i<size;++i){
        for(;i<size*8;++i){
            // print last bit and shift left.
            printf("%u ",num&maxPow ? 1 : 0);
            num = num<<1;
        }
    }
    printf("\n");
}

int main(int argc, char **argv)
{

	int rnd = 1981461296;
    int PRIV_PWD_BITS = 9;
    int descriptor = 3;
    int and = 1;
    int merged;
    for(int i = 0; i < sizeof(int)*8-2; i++){
        and |= and << 1;
    }
    printBits(rnd);
    printBits(and);

    rnd = rnd & and;
    rnd = rnd >> (sizeof(unsigned int)*8 - PRIV_PWD_BITS);
    printBits(rnd);
    printf("Adjusted rnd: %d\n", rnd); //3870041

    merged = (rnd << (sizeof(unsigned int)*8 - PRIV_PWD_BITS)) | descriptor;
    printBits(merged);

    printf("Merged: %d\n", merged); //not 2986344451

	return 0;
}
