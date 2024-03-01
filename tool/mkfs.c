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
#include "arguments.h"

static inline uint32_t align_down(uint32_t x, uint32_t a) {
    return x / a * a;
}

static inline uint32_t idiv_ceil(uint32_t a, uint32_t b) {
    return (a / b) + (a % b != 0);
}

/* fill the on-disk superblock with relevant data */
static long write_superblock(int bfd, Yaf_Superblock *ysb, long bnr) {
    long ret = 0;
    uint32_t nr_i, nr_d, nr_ibp, nr_dbp;

    /* initialize the *Yaf_Superblock* */
    bnr = align_down(bnr, INODES_PER_BLOCK);
    nr_ibp = idiv_ceil(bnr, YAF_BLOCK_SIZE * 8);
    ysb->yaf_sb_info.nr_ibp = htole32(nr_ibp);
    log(LOG_INFO, "inode bitmap section has %d block(s)", nr_ibp);

    nr_dbp = idiv_ceil(bnr, YAF_BLOCK_SIZE * 8);
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
    ret = write(bfd, ysb, sizeof(Yaf_Superblock));
    if (ret != sizeof(Yaf_Superblock)) {
        ret = -EIO;
        log(LOG_INFO, "write() failed");
        goto out;
    }

    /* restore *Yaf_Superblock* to host endian */
    ysb->yaf_sb_info.nr_ibp = le32toh(ysb->yaf_sb_info.nr_ibp);
    ysb->yaf_sb_info.nr_dbp = le32toh(ysb->yaf_sb_info.nr_dbp);
    ysb->yaf_sb_info.nr_i = le32toh(ysb->yaf_sb_info.nr_i);
    ysb->yaf_sb_info.nr_d = le32toh(ysb->yaf_sb_info.nr_d);

    ret = 0;

out:
    return ret;
}

/* fill the disk inode bitmap section with relevant data */
static long write_inode_bitmap(int bfd, Yaf_Superblock *ysb) {
    // TODO: unimplemented
    return 0;
}

/* fill the disk data bitmap section with relevant data */
static long write_data_bitmap(int bfd, Yaf_Superblock *ysb) {
    // TODO: unimplemented
    return 0;
}

/* fill the disk inode blocks section with relevant data */
static long write_inode_blocks(int bfd, Yaf_Superblock *ysb) {
    // TODO: unimplemented
    return 0;
}

/* fill the disk data blocks section with relevant data */
static long write_data_blocks(int bfd, Yaf_Superblock *ysb) {
    // TODO: unimplemented
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
