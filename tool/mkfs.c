#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../include/inode.h"
#include "../include/super.h"
#include "../include/yaf.h"
#include "../include/bitmap.h"
#include "arguments.h"

static inline uint32_t align_down(uint32_t x, uint32_t a) {
    return x / a * a;
}

static inline uint32_t idiv_ceil(uint32_t a, uint32_t b) {
    return (a / b) + (a % b != 0);
}

/* fill the on-disk superblock with relevant data */
static long write_superblock(int bfd, Yaf_Superblock *ysb, long bnr) {
    long ret;
    uint32_t nr_i, nr_d, nr_ibp, nr_dbp;

    /* initialize the *Yaf_Superblock* */
    bnr = align_down(bnr, INODES_PER_BLOCK);
    nr_ibp = idiv_ceil(bnr, YAF_BLOCK_SIZE * BITS_PER_BYTE);
    ysb->yaf_sb_info.nr_ibp = htole32(nr_ibp);
    log(LOG_INFO, "inode bitmap section has %d block(s)", nr_ibp);

    nr_dbp = idiv_ceil(bnr, YAF_BLOCK_SIZE * BITS_PER_BYTE);
    ysb->yaf_sb_info.nr_dbp = htole32(nr_dbp);
    log(LOG_INFO, "data bitmap section has %d block(s)", nr_dbp);

    nr_i = idiv_ceil(bnr, INODES_PER_BLOCK);
    ysb->yaf_sb_info.nr_i = htole32(nr_i);
    log(LOG_INFO, "inode blocks section has %d block(s)", nr_i);

    nr_d = bnr - 1 - nr_i - nr_ibp - nr_dbp;
    ysb->yaf_sb_info.nr_d = htole32(nr_d);
    log(LOG_INFO, "data blocks section has %d block(s)", nr_d);


    /* fill magic string */
    for (int idx = 0; idx < sizeof(ysb->magic); idx += sizeof(MAGIC)) {
        memcpy(&ysb->magic[idx], MAGIC, sizeof(MAGIC));
    }

    /* write down the data */
    ret = lseek(bfd, BID_SB_MIN(ysb) * YAF_BLOCK_SIZE, SEEK_SET);
    if (ret == -1) {
        ret = errno;
        log(LOG_ERR, "lseek() failed with error %s", strerror(errno));
        goto out;
    }
    ret = write(bfd, ysb, sizeof(Yaf_Superblock));
    if (ret != sizeof(Yaf_Superblock)) {
        ret = -EIO;
        log(LOG_INFO, "write() failed");
        goto out;
    }
    log(LOG_INFO, "Writing %ld byte(s) at disk offset %ld "
        "for the superblock", sizeof(Yaf_Superblock),
        BID_SB_MIN(ysb) * YAF_BLOCK_SIZE);

    /* restore *Yaf_Superblock* to host endian */
    ysb->yaf_sb_info.nr_ibp = le32toh(ysb->yaf_sb_info.nr_ibp);
    ysb->yaf_sb_info.nr_dbp = le32toh(ysb->yaf_sb_info.nr_dbp);
    ysb->yaf_sb_info.nr_i = le32toh(ysb->yaf_sb_info.nr_i);
    ysb->yaf_sb_info.nr_d = le32toh(ysb->yaf_sb_info.nr_d);

    log(LOG_INFO, "superblock is at blocks [%ld, %ld]",
        BID_SB_MIN(ysb), BID_SB_MAX(ysb));
    log(LOG_INFO, "inode bitmap section is at blocks [%ld, %ld]",
        BID_IBP_MIN(ysb), BID_IBP_MAX(ysb));
    log(LOG_INFO, "data bitmap section is at blocks [%ld, %ld]",
        BID_DBP_MIN(ysb), BID_DBP_MAX(ysb));
    log(LOG_INFO, "inode blocks section is at blocks [%ld, %ld]",
        BID_I_MIN(ysb), BID_I_MAX(ysb));
    log(LOG_INFO, "data blocks section is at blocks [%ld, %ld]",
        BID_D_MIN(ysb), BID_D_MAX(ysb));

    ret = 0;

out:
    return ret;
}


/* convert inode bitmap idx to the offset in disk */
#define IDXI2DOFF(sb, idx)  (IDXI2BID(sb, idx) * YAF_BLOCK_SIZE \
                             + IDX2BKOFF(idx))

/* fill the disk inode bitmap section with relevant data */
static long write_inode_bitmap(int bfd, Yaf_Superblock *ysb) {
    long ret = 0;
    uint8_t byte;

    /* zero the inode bitmap section */
    for (int i = 0; i < le32toh(ysb->yaf_sb_info.nr_ibp); ++i) {
        char bytes[YAF_BLOCK_SIZE] = {};

        ret = lseek(bfd, (BID_IBP_MIN(ysb) + i) * YAF_BLOCK_SIZE,
                    SEEK_SET);
        if (ret == -1) {
            ret = errno;
            log(LOG_ERR, "lseek() failed with error %s", strerror(errno));
            goto out;
        }
        ret = write(bfd, &bytes, sizeof(bytes));
        if (ret != sizeof(bytes)) {
            ret = -EIO;
            log(LOG_INFO, "write() failed");
            goto out;
        }
        log(LOG_INFO, "Writing %ld byte(s) at disk offset %ld "
            "for the inode bitmap", sizeof(bytes),
            (BID_IBP_MIN(ysb) + i) * YAF_BLOCK_SIZE);
    }

    /* mark the root inode */
    ret = lseek(bfd, IDXI2DOFF(ysb, ROOT_INO), SEEK_SET);
    if (ret == -1) {
        ret = errno;
        log(LOG_ERR, "lseek() failed with error %s", strerror(errno));
        goto out;
    }
    byte = yaf_set_bit(0, IDX2BKOFF(ROOT_INO));
    ret = write(bfd, &byte, sizeof(byte));
    if (ret != sizeof(byte)) {
        ret = -EIO;
        log(LOG_INFO, "write() failed");
        goto out;
    }
    log(LOG_INFO, "Writing %ld byte(s) at disk offset %ld "
        "for the root inode in inode bitmap", sizeof(byte),
        IDXI2DOFF(ysb, ROOT_INO));

    ret = 0;

out:
    return ret;
}

/* fill the disk data bitmap section with relevant data */
static long write_data_bitmap(int bfd, Yaf_Superblock *ysb) {
    long ret = 0;

    /* zero the data bitmap section */
    for (int i = 0; i < le32toh(ysb->yaf_sb_info.nr_dbp); ++i) {
        char bytes[YAF_BLOCK_SIZE] = {};

        ret = lseek(bfd, (BID_DBP_MIN(ysb) + i) * YAF_BLOCK_SIZE,
                    SEEK_SET);
        if (ret == -1) {
            ret = errno;
            log(LOG_ERR, "lseek() failed with error %s", strerror(errno));
            goto out;
        }
        ret = write(bfd, &bytes, sizeof(bytes));
        if (ret != sizeof(bytes)) {
            ret = -EIO;
            log(LOG_INFO, "write() failed");
            goto out;
        }
        log(LOG_INFO, "Writing %ld byte(s) at disk offset %ld "
            "for the data bitmap", sizeof(bytes),
            (BID_DBP_MIN(ysb) + i) * YAF_BLOCK_SIZE);
    }

    ret = 0;

out:
    return ret;
}

/* print debugging information of the given inode */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
static void debug_inode(uint32_t ino, Yaf_Inode *node) {
    log(LOG_INFO, "==========debug information begin=============");
    log(LOG_INFO, "debugging information for inode %d:", ino);
    log(LOG_INFO, "i_mode = %#o", le32toh(node->i_mode));
    log(LOG_INFO, "i_uid = %d", le32toh(node->i_uid));
    log(LOG_INFO, "i_gid = %d", le32toh(node->i_gid));
    log(LOG_INFO, "i_nlink = %d", le32toh(node->i_nlink));
    log(LOG_INFO, "i_atime = %d", le32toh(node->i_atime));
    log(LOG_INFO, "i_mtime = %d", le32toh(node->i_mtime));
    log(LOG_INFO, "i_ctime = %d", le32toh(node->i_ctime));
    log(LOG_INFO, "i_size = %d", le32toh(node->i_size));
    for (int i = 0; i < ARRAY_SIZE(node->i_block); ++i) {
        log(LOG_INFO, "i_block[%d] = %d", i, le32toh(node->i_block[i]));
    }
    log(LOG_INFO, "==========debug information end=============");
}

/* convert inode number to the offset in disk */
#define INO2DOFF(sb, ino)   (INO2BID(sb, ino) * YAF_BLOCK_SIZE \
                             + INO2BOFF(sb, ino))

/* fill the disk inode blocks section with relevant data */
static long write_inode_blocks(int bfd, Yaf_Superblock *ysb) {
    long ret = 0;
    Yaf_Inode root;

    /* initialize the root on-disk inode */
    root.i_mode = htole32(S_IFDIR | ACCESSPERMS);   /*directory, 0777*/
    root.i_uid = htole32(geteuid());
    root.i_gid = htole32(getegid());
    root.i_nlink = htole32(1);
    root.i_atime = root.i_mtime = root.i_ctime = htole32(0);
    root.i_size = 0;
    memset(&root.i_block, 0, sizeof(root.i_block));

    /* write down the data */
    ret = lseek(bfd, INO2DOFF(ysb, ROOT_INO), SEEK_SET);
    if (ret == -1) {
        ret = errno;
        log(LOG_ERR, "lseek() failed with error %s", strerror(errno));
        goto out;
    }

    if (write(bfd, &root, sizeof(root)) != sizeof(root)) {
        ret = -EIO;
        log(LOG_INFO, "write() failed");
        goto out;
    }

    log(LOG_INFO, "Writing %ld byte(s) at disk offset %ld "
        "for the root inode", sizeof(root), INO2DOFF(ysb, ROOT_INO));
    debug_inode(ROOT_INO, &root);
    ret = 0;

out:
    return ret;
}

/* fill the disk data blocks section with relevant data */
static long write_data_blocks(int bfd, Yaf_Superblock *ysb) {
    /*
     * Since the VFS manages "." and ".." dentrys,
     * the root inode currently requires no data.
     */
    return 0;
}

int main(int argc, char *argv[])
{
    Arguments arguments = {};
    Yaf_Superblock ysb = {};
    int bfd = -1;
    long ret = 0, bnr;
    struct stat bstat;

    log(LOG_INFO, "format the yaf filesystem");

    mkfs_parse_arguments(&arguments, argc, argv);

    /* open device */
    bfd = open(arguments.device, O_RDWR);
    if (bfd == -1) {
        ret = errno;
        log(LOG_ERR, "open() failed with error %s", strerror(errno));
        goto out;
    }

    /* get device block number */
    ret = fstat(bfd, &bstat);
    if (ret == -1) {
        ret = errno;
        log(LOG_ERR, "fstat() failed with error %s", strerror(errno));
        goto close_bfd;
    }
    if ((bstat.st_mode & S_IFMT) == S_IFBLK) {
        /* for block device */
        long size;
        ret = ioctl(bfd, BLKGETSIZE64, &size);
        if (ret) {
            ret = errno;
            log(LOG_ERR, "ioctl() failed with error %s", strerror(errno));
            goto close_bfd;
        }
        bnr = size / YAF_BLOCK_SIZE;
    } else {
        /* for other cases */
        bnr = bstat.st_size / YAF_BLOCK_SIZE;
    }
    log(LOG_INFO, "%s has %ld blocks", arguments.device, bnr);


    /* write down the superblock data */
    ret = write_superblock(bfd, &ysb, bnr);
    if (ret) {
        ret = errno;
        log(LOG_ERR,
            "write_superblock() failed with error %s", strerror(errno));
        goto close_bfd;
    }

    /* write down the inode bitmap data */
    ret = write_inode_bitmap(bfd, &ysb);
    if (ret) {
        ret = errno;
        log(LOG_ERR,
            "write_inode_bitmap() failed with error %s", strerror(errno));
        goto close_bfd;
    }

    /* write down the data bitmap data */
    ret = write_data_bitmap(bfd, &ysb);
    if (ret) {
        ret = errno;
        log(LOG_ERR,
            "write_data_bitmap() failed with error %s", strerror(errno));
        goto close_bfd;
    }

    /* write down the inode blocks data */
    ret = write_inode_blocks(bfd, &ysb);
    if (ret) {
        ret = errno;
        log(LOG_ERR,
            "write_inode_blocks() failed with error %s", strerror(errno));
        goto close_bfd;
    }

    /* write down the data blocks data */
    ret = write_data_blocks(bfd, &ysb);
    if (ret) {
        ret = errno;
        log(LOG_ERR,
            "write_data_blocks() failed with error %s", strerror(errno));
        goto close_bfd;
    }

    log(LOG_INFO, "yaf filesystem has been successfully "
        "formatted on the device");

close_bfd:
    close(bfd);
out:
    return ret;
}
