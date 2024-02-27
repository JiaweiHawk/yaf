#ifndef __SUPER_H_

    #define __SUPER_H_

    #include <linux/fs.h>
    /* fill the in-memory superblock according to on-disk superblock */
    int yaf_fill_super(struct super_block *sb, void *data, int flags);

#endif // __SUPER_H_
