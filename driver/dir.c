#include <asm-generic/errno-base.h>
#include <linux/buffer_head.h>
#include <linux/byteorder/generic.h>
#include <linux/fs_types.h>
#include <linux/stat.h>
#include "../include/inode.h"
#include "../include/yaf.h"

/*
 * called when the VFS needs to read the directory contents.
 *
 * Iterate over the files contained in dir and commit them to @ctx
 */
static int yaf_iterate_shared(struct file *dir, struct dir_context *ctx) {
    struct inode *dinode = file_inode(dir);
    struct super_block *sb = dinode->i_sb;
    Yaf_Inode_Info *dyii = YAF_INODE(dinode);
    uint64_t doff;

    /* ensure that dir is a directory */
    if (!S_ISDIR(dinode->i_mode)) {
        log(LOG_ERR, "@dir is not a directory");
        return -ENOTDIR;
    }

    /* commit possible *.* and *..* to @ctx */
    if (!dir_emit_dots(dir, ctx)) {
        log(LOG_ERR, "dir_emit_dots() failed");
        return 0;
    }

    if (ctx->pos == 2) {
        doff = 0;
    } else {
        // ctx->pos > 2
        doff = ctx->pos - 2;

        /* validate the @ctx->pos */
        if (doff % YAF_DENTRY_SIZE) {
            log(LOG_ERR, "@ctx->pos = %lld is not valid", ctx->pos);
            return -ENOENT;
        }
    }

    /* iterate files in the directory from doff */
    while(doff < dinode->i_size) {
        Yaf_Dentry *yd;
        uint64_t iboff = doff % YAF_BLOCK_SIZE;
        struct buffer_head *bh = sb_bread(sb,
                    DNO2BID(sb, dyii->i_block[doff / YAF_BLOCK_SIZE]));
        if (!bh) {
            log(LOG_ERR, "sb_bread() failed");
            return -EIO;
        }

        yd = (Yaf_Dentry *)(bh->b_data);
        for(int i = iboff / YAF_DENTRY_SIZE;
            i < DENTRYS_PER_BLOCK && doff < dinode->i_size;
            ++i, ++yd, doff += YAF_DENTRY_SIZE) {
            if (le32_to_cpu(yd->d_ino) != RESERVED_INO) {
                if (!dir_emit(ctx, yd->d_name, le32_to_cpu(yd->d_name_len),
                              le32_to_cpu(yd->d_ino), DT_UNKNOWN)) {
                    log(LOG_ERR, "dir_emit() for %s failed", yd->d_name);
                }
            }
        }

        brelse(bh);
    }

    /* update the @ctx->pos */
    ctx->pos = doff + 2;

    return 0;
}

/*
 * describes how the VFS can manipulate an open directory accroding to
 * https://docs.kernel.org/next/filesystems/vfs.html#struct-file-operations
 */
const struct file_operations yaf_dir_ops = {
    .iterate_shared = yaf_iterate_shared, /* called when the VFS needs to
                                read the directory contents */
};
