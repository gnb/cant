#include <stdio.h>
#include "a.h"

int
main(int argc, char **argv)
{
    printf("MESSAGE=\"%s\"\n", MESSAGE);
    printf("argv[1]=\"%s\"\n", argv[1]);
    return (strcmp(MESSAGE, argv[1]) ? 1 : 0);
}
