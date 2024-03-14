#include <linux/buffer_head.h>
#include <linux/byteorder/generic.h>
#include <linux/fs.h>
#include <linux/gfp_types.h>
#include <linux/slab.h>
#include <linux/writeback.h>
#include "../include/yaf.h"
#include "../include/super.h"
#include "../include/inode.h"
#include "../include/fs.h"

/* *Yaf_Inode_Info* strucutre cache  */
static struct kmem_cache *yaf_inode_cachep;

/*
 * yaf_inode_init() initializes only the embedded *struct inode*,
 * other parts will be initialized when parsing the on-disk inode.
 */
static void yaf_inode_init(void *object)
{
    Yaf_Inode_Info *yii = object;
	inode_init_once(&yii->vfs_inode);
}

/* initialize the *Yaf_Sb_Info* cache */
long yaf_init_inode_cache(void)
{
    long ret = 0;

    yaf_inode_cachep = kmem_cache_create("yaf_inode_cache",
                    sizeof(Yaf_Inode_Info), 0,
                    SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD | SLAB_ACCOUNT,
                    yaf_inode_init);

    if (!yaf_inode_cachep) {
        ret = -ENOMEM;
        log(LOG_ERR, "kmem_cache_create() failed");
    }

	return ret;
}

/* finalize the *Yaf_Sb_Info* cache */
void yaf_fini_inode_cache(void)
{
	/*
	 * Make sure all delayed rcu free inodes are flushed before we
	 * destroy cache.
	 */
	rcu_barrier();
	kmem_cache_destroy(yaf_inode_cachep);
}

/*
 * yaf_alloc_inode() allocates *Yaf_Inode_Info* from
 * *yaf_inode_cachep*, which contains an initialized
 * *struct inode* embedded within it.
 */
static struct inode* yaf_alloc_inode(struct super_block *sb)
{
    Yaf_Inode_Info *yii = NULL;
    struct inode *inode = NULL;

    yii = kmem_cache_alloc(yaf_inode_cachep, GFP_KERNEL);
    if (!yii) {
        inode = ERR_PTR(-ENOMEM);
        log(LOG_ERR, "kmem_cache_alloc() failed");
        goto out;
    }

    inode = &yii->vfs_inode;

out:
    return inode;
}

/*
 * yaf_destroy_inode() needs to release the *Yaf_Inode_Info*
 * which *struct inode* embedded within it to the *yaf_inode_cachep*.
 */
static void yaf_destroy_inode(struct inode *inode)
{
    Yaf_Inode_Info *yii = YAF_INODE(inode);
    kmem_cache_free(yaf_inode_cachep, yii);
}

/* write in-memory inode back to on-disk inode */
static int yaf_write_inode(struct inode *inode,
                           struct writeback_control *wbc)
{
    struct super_block *sb = inode->i_sb;
    Yaf_Inode_Info *yii = YAF_INODE(inode);
    Yaf_Inode *dyi;
    struct buffer_head *bh;

    bh = sb_bread(sb, INO2BID(sb, inode->i_ino));
    if (!bh) {
        return -EIO;
        log(LOG_ERR, "sb_bread() failed");
    }
    dyi = (Yaf_Inode *)(bh->b_data + INO2BOFF(sb, inode->i_ino));

    dyi->i_mode = cpu_to_le32(inode->i_mode);
    dyi->i_uid = cpu_to_le32(i_uid_read(inode));
    dyi->i_gid = cpu_to_le32(i_gid_read(inode));
    dyi->i_nlink = cpu_to_le32(inode->i_nlink);
    dyi->i_atime = cpu_to_le32(inode_get_atime_sec(inode));
    dyi->i_mtime = cpu_to_le32(inode_get_mtime_sec(inode));
    dyi->i_ctime = cpu_to_le32(inode_get_ctime_sec(inode));
    dyi->i_size = cpu_to_le32(inode->i_size);
    for (int i = 0; i < ARRAY_SIZE(yii->i_block); ++i) {
        yii->i_block[i] = cpu_to_le32(yii->i_block[i]);
    }

    mark_buffer_dirty(bh);
    brelse(bh);

    return 0;
}

/*
 * This describes how the VFS can manipulate the superblock
 * of the yaf according to
 * https://www.kernel.org/doc/html/latest/filesystems/vfs.html#struct-super-operations
 */
static struct super_operations yaf_super_ops = {
    .alloc_inode = yaf_alloc_inode,     /* this method is called to allocate
                                         * memory for *struct inode* and
                                         * initialize it */
    .destroy_inode = yaf_destroy_inode, /* this method is called to release
                                         * resources allocated for
                                         * *struct inode* */
    .write_inode = yaf_write_inode,     /* this method is called when the VFS
                                         * needs to write an inode to disk */
};

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
    sb->s_op = &yaf_super_ops;

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
    ysi->nr_ibp = le32_to_cpu(ysb->yaf_sb_info.nr_ibp);
    ysi->nr_dbp = le32_to_cpu(ysb->yaf_sb_info.nr_dbp);
    ysi->nr_i = le32_to_cpu(ysb->yaf_sb_info.nr_i);
    ysi->nr_d = le32_to_cpu(ysb->yaf_sb_info.nr_d);

    /* attach yaf private data to *struct super_block* */
    sb->s_fs_info = ysi;

    /* get inode for root dentry from block device */
    root = yaf_iget(sb, ROOT_INO);
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

    log(LOG_INFO, "superblock is at blocks [%ld, %ld]",
        BID_SB_MIN(sb), BID_SB_MAX(sb));
    log(LOG_INFO, "inode bitmap section is at blocks [%ld, %ld]",
        BID_IBP_MIN(sb), BID_IBP_MAX(sb));
    log(LOG_INFO, "data bitmap section is at blocks [%ld, %ld]",
        BID_DBP_MIN(sb), BID_DBP_MAX(sb));
    log(LOG_INFO, "inode blocks section is at blocks [%ld, %ld]",
        BID_I_MIN(sb), BID_I_MAX(sb));
    log(LOG_INFO, "data blocks section is at blocks [%ld, %ld]",
        BID_D_MIN(sb), BID_D_MAX(sb));

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

void yaf_kill_sb(struct super_block *sb)
{
    kill_block_super(sb);
}
