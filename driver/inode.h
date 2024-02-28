#ifndef __INODE_H_

    #define __INODE_H_

    #ifdef __KERNEL__
        #include <linux/fs.h>
        #include <linux/types.h>
        struct inode *yaf_iget(struct super_block *sb, sector_t ino);
    #endif // __KERNEL__

#endif // __INODE_H_
