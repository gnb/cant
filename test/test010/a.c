#include <stdio.h>
#include <glib.h>

int
main(int argc, char **argv)
{
    char *str;
    
    str = g_strjoinv(" ", argv);
    printf("argv[] = %s\n", str);
    g_free(str);
    
    return 0;
}
