#ifndef __FILE_H_

    #define __FILE_H_

    #ifdef __KERNEL__

        #include <linux/fs.h>
        extern const struct address_space_operations yaf_as_ops;
        extern const struct file_operations yaf_file_ops;

    #endif // __KERNEL__

#endif // __FILE_H_
