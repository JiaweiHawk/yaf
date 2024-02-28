#ifndef __YAF_H_

    #define __YAF_H_

    /* use log for both kernel and user */
    #define LOG_INFO "\033[0;32m"
    #define LOG_ERR "\033[0;31m"
    #define LOG_NONE "\033[0m"
    #ifdef __KERNEL__
        #include <linux/printk.h>
        #include <linux/kern_levels.h>
        #define log(level, args...) do { \
            printk(KERN_DEFAULT level); \
            printk(KERN_CONT "[yaf(%s:%d)]: ", __FILE__, __LINE__); \
            printk(KERN_CONT args); \
            printk(KERN_CONT LOG_NONE "\n"); \
        } while(0)
    #else // __KERNEL__
        #include <stdio.h>
        #define log(level, args...) do { \
            printf(level); \
            printf("[mkfs(%s:%d)]: ", __FILE__, __LINE__); \
            printf(args); \
            printf(LOG_NONE "\n"); \
        } while(0)
    #endif // __KERNEL__

    /*
     * partition layout
     *
     *     ┌──────────┬─────────────┬────────────┬────────────┬───────────┐
     *     │superblock│inode bitmap │data bitmap │inode blocks│data blocks│
     *     ├──────────┼─────────────┼────────────┼────────────┼───────────┤
     *     │          │             │            │            │           │
     *     ▼          ▼             ▼            ▼            ▼           ▼
     *  BID_MIN   BID_SB_MAX   BID_IBP_MAX  BID_DBP_MAX   BID_I_MAX   BID_D_MAX
     * BID_SB_MIN BID_IBP_MIN  BID_DBP_MIN   BID_I_MIN    BID_D_MIN    BID_MAX
     */
    #define BID_MIN(sb) 0
    #define BID_SB_MIN(sb) BID_MIN(sb)

#endif // __YAF_H_
