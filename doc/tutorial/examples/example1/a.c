#include <stdio.h>

extern void a(void);
extern void b(void);

void
a(void)
{
    printf("a() called\n");
}

int
main(int argc, char **argv)
{
    a();
    b();
    return 0;
}
