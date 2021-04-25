#include <stdio.h>
#include <stdlib.h>

#define PRIV_TAG_BIT 12
#define MAX_PRIVATE_TAGS 4096

int merge_rnd_descriptor(int rnd, int descriptor)
{
    //rnd will be like (if PRIV_TAG_BIT = 12)
    //00000000-00005678-12345678-12345678
    //Descriptor
    //00000000-00000000-00001111-11111111
    if(descriptor >= MAX_PRIVATE_TAGS){
        printf("Ipc private descriptor is more that expected ?!");
        return -1;
    }

    //the expected result:
    //12345678-12345678-56781111-11111111
    //      ^                       ^
    //      |                       |
    //the "private key"  "the real descriptor"
    return (rnd << PRIV_TAG_BIT) | descriptor;
}


//TODO fix here and check if working ! ! ! !
unsigned long integer_xor(int rnd, unsigned long addr)
{
    int i;
    unsigned long xorred = 0;
    unsigned long adjusted_rnd = 0;

    adjusted_rnd = (unsigned long) (rnd);
    //lets make sure that only the relevant bits are non-zero
    adjusted_rnd = adjusted_rnd << (sizeof(void *)-(sizeof(int)));
    adjusted_rnd = adjusted_rnd >> (sizeof(void *)-(sizeof(int)));
    adjusted_rnd = adjusted_rnd << 11;
    printf("Adjusted_rnd: %lu\n", adjusted_rnd);

    //                     What are we doing here ?
    //void *addr =
    //12345678-12345678-12345678-12345678-12345678-12345678-12345678-12345678
    //XOR
    //adjusted_rnd =                (IF PRIV_TAG_BIT = 12, RND BIT = 20
    //00000000-00000000-00000000-00000000-02345678-12345678-12345000-00000000
    //the fact that the first 11 bits are not xorred is because the most
    //relevant information is contained from the 12-th bit on, and not from the ones
    //used to specify the page offset.

    //do the XOR between rnd and addr (starting from 11-th bit of addr)
    xorred = (unsigned long)addr ^ adjusted_rnd;


    return xorred;
}

int main()
{
    unsigned int rnd = 4294967291;
    //adjust random depending on PRIV_TAG_BIT value
    rnd = rnd >> PRIV_TAG_BIT;

    int descriptor = 23;
    int addr = 3;
    unsigned long address = (unsigned long) &addr;
    unsigned long xorResult;
    unsigned int merged_descriptor;

    printf("Address: %lu\nRnd: %u\nDescriptor: %d\n", address, rnd, descriptor);

    xorResult = integer_xor(rnd, address);

    printf("Xor result: %lu\n", xorResult);

    merged_descriptor = merge_rnd_descriptor(rnd, descriptor);
    printf("Merged Descriptor: %u\n", merged_descriptor);

    //TODO insert this lines on info recover of tag_gtable (in tag_send/tag_receive ! ! !)
    printf("Extracting rnd: %d\n", merged_descriptor>>PRIV_TAG_BIT);
    printf("Extracting descriptor: %d\n", (merged_descriptor<<(32-PRIV_TAG_BIT))>>(32-PRIV_TAG_BIT));


    xorResult = integer_xor(rnd, xorResult);

    printf("This should be the same of Addr: %lu\n", xorResult);

    return 0;
}
