#include <linux/buffer_head.h>
#include <linux/byteorder/generic.h>
#include <linux/gfp_types.h>
#include <linux/slab.h>
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
