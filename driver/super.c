#include <linux/buffer_head.h>
#include <linux/gfp_types.h>
#include <linux/slab.h>
#include "../include/yaf.h"
#include "../include/super.h"
#include "../include/inode.h"

/*
 * yaf_fill_super() is responsible for parsing the provided
 * block device containing the yaf filesystem image, creating
 * and initializing an in-memory superblock, represented by
 * *struct super_block*, based on the on-disk superblock.
 */
int yaf_fill_super(struct super_block *sb, void *data, int silent)
{
    long ret = 0;
    struct buffer_head *bh = NULL;
    Yaf_Superblock *ysb = NULL;
    Yaf_Sb_Info *ysi = NULL;
    struct inode *root = NULL;

    /* initialize *struct super_block* */
    sb_set_blocksize(sb, YAF_BLOCK_SIZE);

    /* read on-disk superblock from block device */
    bh = sb_bread(sb, BID_SB_MIN(sb));
    if (!bh) {
        ret = -EIO;
        log(LOG_ERR, "sb_bread() failed");
        goto out;
    }

    ysb = (Yaf_Superblock *)bh->b_data;

    /* check on-disk superblock magic string */
    for (int idx = 0; idx < sizeof(ysb->magic); idx += sizeof(MAGIC)) {
        if (memcmp(&ysb->magic[idx], MAGIC, sizeof(MAGIC))) {
            ret = -EINVAL;
            log(LOG_ERR, "magic string check failed");
            goto release_bh;
        }
    }

    /* alloc *Yaf_Sb_Info* */
    ysi = kzalloc(sizeof(Yaf_Sb_Info), GFP_KERNEL);
    if (!ysi) {
        ret = -ENOMEM;
        log(LOG_ERR, "kzalloc() failed");
        goto release_bh;
    }

    /* initialize *Yaf_Sb_Info* */
    ysi->nr_ibp = ysb->yaf_sb_info.nr_ibp;
    ysi->nr_dbp = ysb->yaf_sb_info.nr_dbp;
    ysi->nr_i = ysb->yaf_sb_info.nr_i;
    ysi->nr_d = ysb->yaf_sb_info.nr_d;

    /* get inode fro root dentry from block device */
    root = yaf_iget(sb, 0);
    if (IS_ERR(root)) {
        ret = PTR_ERR(root);
        log(LOG_ERR,
            "yaf_iget() failed with error code %ld", ret);
        goto free_ysi;
    }

    /* create root dentry for this mount instance */
    sb->s_root = d_make_root(root);
    if (!sb->s_root) {
        ret = -ENOMEM;
        log(LOG_ERR, "d_make_root() failed");
        goto iput_root;
    }

    goto release_bh;

iput_root:
    iput(root);
free_ysi:
    kfree(ysi);
release_bh:
    brelse(bh);
out:
    return ret;
}
