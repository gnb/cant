#include <stdio.h>

extern void a(void);
extern void b(void);
extern void c(void);
extern void d(void);
extern void e(void);
extern void f(void);

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
    c();
    d();
    e();
    f();
    return 0;
}
