#include "../include/yaf.h"
#include "arguments.h"

int main(int argc, char *argv[])
{
    Arguments arguments = {};

    log(LOG_INFO, "format the yaf filesystem");

    mkfs_parse_arguments(&arguments, argc, argv);

    return 0;
}
