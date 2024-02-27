#include "super.h"

/*
 * yaf_fill_super() is responsible for parsing the provided
 * block device containing the yaf filesystem image, creating
 * and initializing an in-memory superblock, represented by
 * *struct super_block*, based on the on-disk superblock.
 */
int yaf_fill_super(struct super_block *sb, void *data, int silent)
{
    return -ENOSYS;
}
