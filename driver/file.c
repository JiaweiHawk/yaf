#include <asm-generic/errno-base.h>
#include <linux/buffer_head.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/mpage.h>
#include "../include/file.h"
#include "../include/inode.h"
#include "../include/yaf.h"

/*
 * Associate the @bh with the @iblock-th block of the file
 * denoted by @inode.
 */
static int yaf_get_block(struct inode *inode, sector_t iblock,
                         struct buffer_head *bh_result, int create) {
    Yaf_Inode_Info *yii = YAF_INODE(inode);
    struct super_block *sb = inode->i_sb;

    /* check whether the iblock is in bounds */
    if (iblock * YAF_BLOCK_SIZE >= inode->i_size) {
        log(LOG_ERR, "@iblock %llu is out-of-bounds for [0, %d]",
            iblock, YAF_IBLOCKS);
        return -EFBIG;
    }

    /* map the physical block to the given 'buffer_head' */
    map_bh(bh_result, sb, DNO2BID(sb, yii->i_block[iblock]));

    return 0;
}

/* read the page from the disk and map it into memory*/
static void yaf_readahead(struct readahead_control *rac)
{
    mpage_readahead(rac, yaf_get_block);
}

/*
 * describes how the VFS can manipulate mapping of a file
 * to page cache in your filesystem accroding to
 * https://docs.kernel.org/next/filesystems/vfs.html#struct-address-space-operations
 */
const struct address_space_operations yaf_as_ops = {
    .readahead = yaf_readahead,     /* called by the page cache to
                read pages associated with the address_space object */
};

/*
 * describes how the VFS can manipulate an open file accroding to
 * https://docs.kernel.org/next/filesystems/vfs.html#struct-file-operations
 */
const struct file_operations yaf_file_ops = {
    .owner = THIS_MODULE,
    .read_iter = generic_file_read_iter,    /* called when the VFS needs to
                                               read the file content */
};
