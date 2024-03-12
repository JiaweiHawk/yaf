#ifndef __DIR_H_

    #define __DIR_H_

    #ifdef __KERNEL__
        #include "linux/fs.h"
    #endif

    extern const struct file_operations yaf_dir_ops;

#endif // __DIR_H_
