#include "id_us.h"

#include <stdio.h>
#include <stdlib.h>

void Quit(const char *error)
{
    if (error && error[0] != '\0') {
        fprintf(stderr, "Quit: %s\n", error);
        exit(1);
    }
    exit(0);
}
