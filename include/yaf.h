#ifndef __YAF_H_

    #define __YAF_H_

    /* use log for both kernel and user */
    #define LOG_INFO "\033[0;32m"
    #define LOG_ERR "\033[0;31m"
    #define LOG_NONE "\033[0m"
    #ifdef __KERNEL__
        #include <linux/printk.h>
        #include <linux/kern_levels.h>
        #define log(args...) do { \
            printk(KERN_DEFAULT args); \
            printk(KERN_CONT LOG_NONE "\n"); \
        } while(0)
    #else // __KERNEL__
        #include <stdio.h>
        #define log(args...) do { \
            printf(args); \
            printf(LOG_NONE "\n"); \
        } while(0)
    #endif // __KERNEL__

#endif // __YAF_H_
