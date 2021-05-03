/*
    Header containing fnction used to prevent brute force attacks to the tag service
*/

//TODO ->

unsigned long sleep_secs = 3;

void prevent_bruteforce(char *char_module)
{
    printk(KERN_ALERT "%s: Bruteforce detected", char_module);
    //TODO sent thread to sleep for sleep_secs
}
