#include <stdio.h>

extern void b(void);

int
main(int argc, char **argv)
{
    printf("Hello world\n");
    b();
    return 0;
}
