#include <linux/module.h>
#include "../include/yaf.h"

/* information for module */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hawkins Jiawei");

static int yaf_init(void)
{
    log("initializing yaf filesystem");
    return 0;
}

static void yaf_exit(void)
{
    log("clean up yaf filesystem");
}

module_init(yaf_init);
module_exit(yaf_exit);
