#ifndef __YAF_H_

    #define __YAF_H_

    /* use log for both kernel and user */
    #ifdef __KERNEL__
        #include <linux/printk.h>
        #include <linux/kern_levels.h>
        #define log(args...) { \
            printk(KERN_DEFAULT args); \
            printk(KERN_CONT "\n"); \
        }
    #else // __KERNEL__
        #include <stdio.h>
        #define log(args...) { \
            printf(args); \
            putchar('\n'); \
        }
    #endif // __KERNEL__

#endif // __YAF_H_
