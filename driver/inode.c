#include <linux/array_size.h>
#include <linux/buffer_head.h>
#include <linux/byteorder/generic.h>
#include "../include/yaf.h"
#include "../include/inode.h"
#include "../include/dir.h"

/*
 * The name to look for is found in the dentry.
 *
 * This method must call *d_add()* to insert the found inode
 * into the dentry.
 *
 * If the named inode does not exist a NULL inode should be inserted
 * into the dentry (this is called a negative dentry). Returning an
 * error code from this routine must only be done on a real error,
 * otherwise creating inodes with system calls will fail.
 */
static struct dentry* yaf_lookup(struct inode *dir,
                        struct dentry *dentry, unsigned int flags)
{
    Yaf_Inode_Info *yii = YAF_INODE(dir);
    struct super_block *sb = dir->i_sb;
    struct inode *inode = NULL;
    int doff = 0;

    /* check the dentry name length */
    if (dentry->d_name.len > YAF_DENTRY_NAME_LEN) {
        log(LOG_ERR, "dentry->d_name.len = %d is too long for [1, %ld]",
            dentry->d_name.len, YAF_DENTRY_NAME_LEN);
        return ERR_PTR(-ENAMETOOLONG);
    }

    /* search for the dentry in directory */
    while(doff < dir->i_size) {
        Yaf_Dentry *yd;
        struct buffer_head *bh = sb_bread(sb,
                    DNO2BID(sb, yii->i_block[doff / DENTRYS_PER_BLOCK]));
        if (!bh) {
            log(LOG_ERR, "sb_bread() failed");
            return ERR_PTR(-EIO);
        }

        yd = (Yaf_Dentry *)bh->b_data;
        for(int i = 0; i < DENTRYS_PER_BLOCK && doff < dir->i_size;
            ++i, doff += YAF_DENTRY_SIZE) {
            if (yd->d_ino != RESERVED_INO &&
                !strncmp(yd->d_name, dentry->d_name.name,
                        YAF_DENTRY_NAME_LEN)) {
                inode = yaf_iget(sb, yd->d_ino);
                brelse(bh);
                if (IS_ERR(inode)) {
                    log(LOG_ERR, "yaf_iget() failed with error code %ld",
                        PTR_ERR(inode));
                    return ERR_CAST(inode);
                }
                goto out;
            }
        }

        brelse(bh);
    }

out:

    /* update the directory access time */
    dir->__i_atime = current_time(dir);
    mark_inode_dirty(dir);

    /* fill the dentry with the inode */
    d_add(dentry, inode);

    return NULL;
}

/*
 * describes how the VFS can manipulate an inode according to
 * https://docs.kernel.org/next/filesystems/vfs.html#struct-inode-operations
 */
static const struct inode_operations yaf_inode_ops = {
    .lookup = yaf_lookup,   /* called when the VFS needs to
                    look up an inode in a parent directory */
};

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
    inode->i_op = &yaf_inode_ops;
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
    if (S_ISDIR(inode->i_mode)) {
        inode->i_fop = &yaf_dir_ops;
    }

    /* unlock the inode to make it available */
    unlock_new_inode(inode);

    brelse(bh);

out:
    return inode;
}
