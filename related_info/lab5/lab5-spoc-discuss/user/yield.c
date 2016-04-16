#include <ulib.h>
#include <stdio.h>

int
main(void) {
    int i;
    cprintf("Hello, I am process %d.\n", getpid());
    for (i = 0; i < 5; i ++) {
        int CS = 1;
        asm volatile (
        		"movl %%cs, %0\n"
        		: "=r"(CS)
        		);
        cprintf("CPL = %d \n", CS&3);
        yield();
        cprintf("Back in process %d, iteration %d.\n", getpid(), i);
    }
    cprintf("All done in process %d.\n", getpid());
    cprintf("yield pass.\n");
    return 0;
}

