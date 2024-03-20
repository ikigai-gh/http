#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void die(const char *msg)
{
    fprintf(stderr, "%s :%s\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}
