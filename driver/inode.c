#include <linux/array_size.h>
#include <linux/buffer_head.h>
#include <linux/byteorder/generic.h>
#include "../include/yaf.h"
#include "../include/inode.h"

/*
 * yaf_iget() is responsible for parsing the on-disk inode,
 * creating and initializing an in-memory inode based on
 * the inode number.
 */
struct inode* yaf_iget(struct super_block *sb, unsigned long ino) {
    struct inode *inode = NULL;
    Yaf_Inode *yi = NULL;
    Yaf_Inode_Info *yii = NULL;
    struct buffer_head *bh = NULL;

    /* check whether the ino is out-of-bounds */
    if (ino >= YAF_SB(sb)->nr_i * INODES_PER_BLOCK) {
        inode = ERR_PTR(-EINVAL);
        log(LOG_ERR, "ino %ld is out-of-bounds for [0, %ld]",
            ino, YAF_SB(sb)->nr_i * INODES_PER_BLOCK);
        goto out;
    }

    /* get a locked *struct inode* */
    inode = iget_locked(sb, ino);
    if (!inode) {
        inode = ERR_PTR(-ENOMEM);
        log(LOG_ERR, "iget_locked() failed");
        goto out;
    }
    yii = YAF_INODE(inode);

    /* read on-disk inode from block device */
    bh = sb_bread(sb, INO2BID(sb, ino));
    if (!bh) {
        inode = ERR_PTR(-EIO);
        log(LOG_ERR, "sb_bread() failed");
        goto out;
    }
    yi = (Yaf_Inode *)bh->b_data + (ino % INODES_PER_BLOCK);

    /* initialize *struct inode* */
    inode->i_mode = le32_to_cpu(yi->i_mode);
    i_uid_write(inode, le32_to_cpu(yi->i_uid));
    i_gid_write(inode, le32_to_cpu(yi->i_gid));
    set_nlink(inode, le32_to_cpu(yi->i_nlink));
    inode->i_size = le32_to_cpu(yi->i_size);
    inode_set_atime(inode, le32_to_cpu(yi->i_atime), 0);
    inode_set_mtime(inode, le32_to_cpu(yi->i_mtime), 0);
    inode_set_ctime(inode, le32_to_cpu(yi->i_ctime), 0);
    for (int i = 0; i < ARRAY_SIZE(yii->i_block); ++i) {
        yii->i_block[i] = le32_to_cpu(yi->i_block[i]);
    }

    /* unlock the inode to make it available */
    unlock_new_inode(inode);

    brelse(bh);

out:
    return inode;
}
