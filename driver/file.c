#include <asm-generic/errno-base.h>
#include <linux/buffer_head.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/mpage.h>
#include <linux/time64.h>
#include <linux/writeback.h>
#include "../include/bitmap.h"
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
    uint32_t dbnr = 0;

    /* check whether the iblock is in bounds */
    if (iblock >= YAF_IBLOCKS) {
        log(LOG_ERR, "@iblock %llu is out-of-bounds for [0, %d]",
            iblock, YAF_IBLOCKS);
        return -EFBIG;
    }


    while(dbnr < YAF_IBLOCKS && yii->i_block[dbnr] != RESERVED_DNO) {
        ++dbnr;
    }

    if (iblock >= dbnr) {
        if (!create) {
            return 0;
        }

        /* allocate the need data block */
        while(dbnr <= iblock) {
            uint32_t dno = yaf_get_free_dblock(sb);
            if (dno == RESERVED_DNO) {
                mark_inode_dirty(inode);
                log(LOG_ERR, "yaf_gre_free_dblock() failed");
                return -ENOSPC;
            }

            yii->i_block[dbnr] = dno;
            ++dbnr;
        }
        mark_inode_dirty(inode);
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
 * Called by the VFS when a write() and relative syscall is maed on
 * a file, before writing the data into the page cache.
 *
 * yaf_write_begin() checks if the write operation can complete.
 */
static int yaf_write_begin(struct file *file,
                           struct address_space *mapping, loff_t pos,
                           unsigned int len, struct page **pagep,
                           void **fsdata)
{
    int ret;

    /* check whether the write can be completed */
    if (pos + len > MAX_FILESIZE) {
        log(LOG_ERR, "write %u bytes from offset %lld is out-of-bounds "
            "for [0, %d]", len, pos, MAX_FILESIZE);
        return -ENOSPC;
    }

    /* prepare the write */
    ret = block_write_begin(mapping, pos, len, pagep, yaf_get_block);
    if (ret < 0) {
        log(LOG_ERR, "block_write_begin() failed with error code %d", ret);
        return ret;
    }

    return ret;
}

/*
 * Called by the VFS after writing data from a write() syscall to the
 * page cache.
 *
 * yaf_write_end() updates inode metadata and truncates the file
 * if necessary.
 */
static int yaf_write_end(struct file *file,
                         struct address_space *mapping, loff_t pos,
                         unsigned int len, unsigned int copied,
                         struct page *page, void *fsdata)
{
    struct inode *inode = file->f_inode;
    struct timespec64 cur;
    int ret;

    ret = generic_write_end(file, mapping, pos, len, copied, page, fsdata);
    if (ret < len) {
        log(LOG_ERR, "generic_write_end() wrote %d bytes, "
            "less than requested", ret);
        return ret;
    }

    /* update the inode */
    cur = current_time(inode);
    inode_set_ctime_to_ts(inode, cur);
    inode_set_mtime_to_ts(inode, cur);
    mark_inode_dirty(inode);

    return ret;
}

/* write the dirty page to the disk */
static int yaf_writepage(struct page *page, struct writeback_control *wbc)
{
    return block_write_full_page(page, yaf_get_block, wbc);
}

/*
 * describes how the VFS can manipulate mapping of a file
 * to page cache in your filesystem accroding to
 * https://docs.kernel.org/next/filesystems/vfs.html#struct-address-space-operations
 */
const struct address_space_operations yaf_as_ops = {
    .readahead = yaf_readahead,     /* called by the page cache to
                read pages associated with the address_space object */
    .writepage = yaf_writepage,     /* called by the page cache to
                                write a dirty page to backing store */
    .write_begin = yaf_write_begin, /* called by the generic buffered
             write code to ask the filesystem to prepare to write file */
    .write_end = yaf_write_end,     /* after a successful write_begin,
                            and data copy, write_end must be called */
};

/*
 * describes how the VFS can manipulate an open file accroding to
 * https://docs.kernel.org/next/filesystems/vfs.html#struct-file-operations
 */
const struct file_operations yaf_file_ops = {
    .owner = THIS_MODULE,
    .read_iter = generic_file_read_iter,    /* called when the VFS needs to
                                               read the file content */
    .write_iter = generic_file_write_iter,  /* called when the VFS needs to
                                               write the file */
    .llseek = generic_file_llseek,          /* called when the VFS needs to
                                            move the file position index */
};
