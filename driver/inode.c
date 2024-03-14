#include <asm-generic/errno.h>
#include <linux/array_size.h>
#include <linux/buffer_head.h>
#include <linux/byteorder/generic.h>
#include <linux/fs.h>
#include <linux/mnt_idmapping.h>
#include <linux/time64.h>
#include "../include/bitmap.h"
#include "../include/dir.h"
#include "../include/inode.h"
#include "../include/yaf.h"

/*
 * yaf_new_inode() is responsible for creating the on-disk inode
 * and initializing an relative in-memory inode.
 */
static struct inode *yaf_new_inode(struct inode *dir, mode_t mode)
{
    struct super_block *sb = dir->i_sb;
    struct inode *inode;
    uint32_t ino;
    struct timespec64 cur;
    Yaf_Inode_Info *yii;

    /* allocate the on-disk inode */
    ino = yaf_get_free_inode(sb);
    if (ino == RESERVED_INO) {
        log(LOG_ERR, "there is not free inode on the disk");
        return ERR_PTR(-ENOSPC);
    }

    inode = yaf_iget(sb, ino);
    if (IS_ERR(inode)) {
        log(LOG_ERR, "yaf_iget() failed with error code %ld",
            PTR_ERR(inode));
        yaf_put_inode(sb, ino);
        return inode;
    }
    yii = YAF_INODE(inode);

    /* overwrite *struct inode* */
    inode_init_owner(&nop_mnt_idmap, inode, dir, mode);
    set_nlink(inode, 1);
    inode->i_size = 0;
    cur = current_time(inode);
    inode_set_atime_to_ts(inode, cur);
    inode_set_mtime_to_ts(inode, cur);
    inode_set_ctime_to_ts(inode, cur);
    for (int i = 0; i < ARRAY_SIZE(yii->i_block); ++i) {
        yii->i_block[i] = 0;
    }
    if (S_ISDIR(inode->i_mode)) {
        inode->i_fop = &yaf_dir_ops;
    }

    /* mark inode is dirty */
    mark_inode_dirty(inode);

    return inode;
}

/* find available on-disk dentry and return its offset */
static int64_t yaf_get_free_dentry(struct inode *dir)
{
    Yaf_Inode_Info *dyii = YAF_INODE(dir);
    struct super_block *sb = dir->i_sb;
    uint64_t doff = 0;
    struct buffer_head *bh;
    struct timespec64 cur;

    /* iterate diretory to find unsed dentry */
    while(doff < dir->i_size) {
        Yaf_Dentry *yd;
        bh = sb_bread(sb,
                DNO2BID(sb, dyii->i_block[doff / YAF_BLOCK_SIZE]));
        if (!bh) {
            log(LOG_ERR, "sb_bread() failed");
            return -EIO;
        }

        yd = (Yaf_Dentry *)bh->b_data;
        for(int i = 0; i < DENTRYS_PER_BLOCK && doff < dir->i_size;
            ++i, ++yd, doff += YAF_DENTRY_SIZE) {
            if (le32_to_cpu(yd->d_ino) == RESERVED_INO) {
                brelse(bh);
                return doff;
            }
        }

        brelse(bh);
    }

    /* check whether directory is full */
    if (dir->i_size / YAF_DENTRY_SIZE == MAX_DENTRYS) {
        return -ENOSPC;
    }

    if ((dir->i_size % YAF_BLOCK_SIZE) == 0) {
        uint32_t dno = yaf_get_free_dblock(sb);
        if (dno == RESERVED_DNO) {
            log(LOG_ERR, "there is not free data block on the disk");
            return -ENOSPC;
        }
        dyii->i_block[doff / YAF_BLOCK_SIZE] = dno;
    }

    /* mark the found dentry as unuse */
    bh = sb_bread(sb,
            DNO2BID(sb, dyii->i_block[doff / YAF_BLOCK_SIZE]));
    assert(bh);
    ((Yaf_Dentry *)(bh->b_data + doff % YAF_BLOCK_SIZE))->d_ino = RESERVED_INO;
    mark_buffer_dirty(bh);
    brelse(bh);

    /* mark dir inode is dirty */
    dir->i_size = doff + YAF_DENTRY_SIZE;
    cur = current_time(dir);
    inode_set_atime_to_ts(dir, cur);
    inode_set_mtime_to_ts(dir, cur);
    inode_set_ctime_to_ts(dir, cur);
    mark_inode_dirty(dir);
    return doff;
}

/* create @dentry(a file or directory) in @dir */
static int _yaf_create(struct mnt_idmap *id, struct inode *dir,
                      struct dentry *dentry, umode_t mode, bool excl)
{
    Yaf_Inode_Info *dyii = YAF_INODE(dir);
    struct super_block *sb = dir->i_sb;
    uint64_t doff;
    struct buffer_head *bh;
    Yaf_Dentry *yd;
    struct inode *inode;
    struct timespec64 cur;

    /* check @dentry name length */
    if (dentry->d_name.len > YAF_DENTRY_NAME_LEN) {
        log(LOG_ERR, "dentry->d_name.len = %d is too long for [1, %ld]",
            dentry->d_name.len, YAF_DENTRY_NAME_LEN);
        return -ENAMETOOLONG;
    }

    /* get on-disk free dentry */
    doff = yaf_get_free_dentry(dir);
    if (doff < 0) {
        log(LOG_ERR, "yaf_get_free_dentry() failed "
            "with error code %lld", doff);
        return -EIO;
    }
    bh = sb_bread(sb,
            DNO2BID(sb, dyii->i_block[doff / YAF_BLOCK_SIZE]));
    if (!bh) {
        /*
         * here we do not need to put the free dentry,
         * then can be used next time
         */
        log(LOG_ERR, "sb_bread() failed");
        return -EIO;
    }
    yd = (Yaf_Dentry *)(bh->b_data + doff % YAF_BLOCK_SIZE);

    /* get on-disk free node */
    inode = yaf_new_inode(dir, mode);
    if (IS_ERR(inode)) {
        log(LOG_ERR, "yaf_new_inode() failed with error code %ld",
            PTR_ERR(inode));
        brelse(bh);
        return PTR_ERR(inode);
    }

    yd->d_ino = cpu_to_le32(inode->i_ino);
    yd->d_name_len = cpu_to_le32(dentry->d_name.len);
    strncpy(yd->d_name, dentry->d_name.name, YAF_DENTRY_NAME_LEN);

    mark_buffer_dirty(bh);
    brelse(bh);

    /* update @dir */
    cur = current_time(dir);
    inode_set_atime_to_ts(dir, cur);
    inode_set_mtime_to_ts(dir, cur);
    inode_set_ctime_to_ts(dir, cur);
    inc_nlink(dir);
    mark_inode_dirty(dir);

    d_instantiate(dentry, inode);

    return 0;
}

/* create subdirectory */
static int yaf_mkdir(struct mnt_idmap *id, struct inode *dir,
                     struct dentry *dentry, umode_t mode)
{
    return _yaf_create(id, dir, dentry, mode | S_IFDIR, 0);
}

/* create regular file */
static int yaf_create(struct mnt_idmap *id, struct inode *dir,
                     struct dentry *dentry, umode_t mode, bool excl)
{
    return _yaf_create(id, dir, dentry, mode | S_IFREG, excl);
}

/* return the on-disk dentry offset in the directory */
static int64_t _yaf_lookup(struct inode *dir, struct dentry *dentry)
{
    struct super_block *sb = dir->i_sb;
    Yaf_Inode_Info *yii = YAF_INODE(dir);
    int64_t doff = 0;

    /* check the dentry name length */
    if (dentry->d_name.len > YAF_DENTRY_NAME_LEN) {
        log(LOG_ERR, "dentry->d_name.len = %d is too long for [1, %ld]",
            dentry->d_name.len, YAF_DENTRY_NAME_LEN);
        return -ENAMETOOLONG;
    }

    /* search for the dentry in directory */
    while(doff < dir->i_size) {
        Yaf_Dentry *yd;
        struct buffer_head *bh = sb_bread(sb,
                    DNO2BID(sb, yii->i_block[doff / YAF_BLOCK_SIZE]));
        if (!bh) {
            log(LOG_ERR, "sb_bread() failed");
            return -EIO;
        }

        yd = (Yaf_Dentry *)bh->b_data;
        for(int i = 0; i < DENTRYS_PER_BLOCK && doff < dir->i_size;
            ++i, ++yd, doff += YAF_DENTRY_SIZE) {
            if (le32_to_cpu(yd->d_ino) != RESERVED_INO &&
                !strncmp(yd->d_name, dentry->d_name.name,
                        YAF_DENTRY_NAME_LEN)) {
                brelse(bh);
                return doff;
            }
        }

        brelse(bh);
    }

    return -ENOENT;
}

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
    int64_t doff = 0;
    struct buffer_head *bh;
    Yaf_Dentry *yd;

    /* search for the dentry in directory */
    doff = _yaf_lookup(dir, dentry);
    if (doff < 0) {
        if (doff == -ENOENT) {
            goto out;
        }
        log(LOG_ERR, "_yaf_lookup() failed with error code %lld", doff);
        return ERR_PTR(doff);
    }

    bh = sb_bread(sb, DNO2BID(sb, yii->i_block[doff / YAF_BLOCK_SIZE]));
    if (!bh) {
        log(LOG_ERR, "sb_bread() failed");
        return ERR_PTR(-EIO);
    }
    yd = (Yaf_Dentry *)(bh->b_data + doff % YAF_BLOCK_SIZE);

    inode = yaf_iget(sb, le32_to_cpu(yd->d_ino));
    if (IS_ERR(inode)) {
        log(LOG_ERR, "yaf_iget() failed with error code %ld",
            PTR_ERR(inode));
        brelse(bh);
        return ERR_CAST(inode);
    }

    brelse(bh);

out:

    /* update the directory access time */
    inode_set_atime_to_ts(dir, current_time(dir));
    mark_inode_dirty(dir);

    /* fill the dentry with the inode */
    d_add(dentry, inode);

    return NULL;
}

/* delete @dentry(a file or directory) in @dir */
static int yaf_delete(struct inode *dir, struct dentry *dentry)
{
    struct super_block *sb = dir->i_sb;
    Yaf_Inode_Info *yii = YAF_INODE(dir);
    struct inode *inode = d_inode(dentry);
    struct buffer_head *bh;
    Yaf_Dentry *yd;
    int64_t doff;
    struct timespec64 cur;

    doff = _yaf_lookup(dir, dentry);
    assert(doff >= 0);

    /* get the *Yaf_Dentry* in @dir  */
    bh = sb_bread(sb, DNO2BID(sb, yii->i_block[doff / YAF_BLOCK_SIZE]));
    if (!bh) {
        log(LOG_ERR, "sb_bread() failed");
        return -EIO;
    }
    yd = (Yaf_Dentry *)(bh->b_data + doff % YAF_BLOCK_SIZE);

    /* remove @dentry from @dir */
    yd->d_ino = cpu_to_le32(RESERVED_INO);

    mark_buffer_dirty(bh);
    brelse(bh);

    /* update the @dir */
    cur = current_time(dir);
    inode_set_atime_to_ts(dir, cur);
    inode_set_mtime_to_ts(dir, cur);
    inode_set_ctime_to_ts(dir, cur);
    drop_nlink(dir);
    mark_inode_dirty(dir);

    drop_nlink(inode);
    if (inode->i_nlink > 1) {
        /* there still other link for this inode */
        mark_inode_dirty(inode);
        return 0;
    }

    /* there is no other link, we can delete this inode */

    /* clear the *i_block* */
    yii = YAF_INODE(inode);
    for (doff = 0; doff < inode->i_size; doff += YAF_BLOCK_SIZE) {
        yaf_put_dblock(sb, yii->i_block[doff / YAF_BLOCK_SIZE]);
    }

    /* put the inode */
    yaf_put_inode(sb, inode->i_ino);
    mark_inode_dirty(inode);

    return 0;
}

/* delete directory @dentry in @dir */
static int yaf_rmdir(struct inode *dir, struct dentry *dentry)
{
    struct inode *inode = d_inode(dentry);

    /* check whether the directory is empty */
    if (inode->i_nlink > 1) {
        return -ENOTEMPTY;
    }

    return yaf_delete(dir, dentry);
}

/*
 * describes how the VFS can manipulate an inode according to
 * https://docs.kernel.org/next/filesystems/vfs.html#struct-inode-operations
 */
static const struct inode_operations yaf_inode_ops = {
    .lookup = yaf_lookup,   /* called when the VFS needs to
                    look up an inode in a parent directory */
    .mkdir = yaf_mkdir,     /* called when the VFS needs to
                               create subdirectories */
    .create = yaf_create,   /* called when the VFS needs to
                               create files */
    .rmdir = yaf_rmdir,     /* called when the VFS needs to
                               delete subdirectories */
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
