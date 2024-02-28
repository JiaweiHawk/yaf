#include <fcntl.h>
#include <linux/fs.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../include/yaf.h"
#include "../include/super.h"
#include "arguments.h"

/* fill the on-disk superblock with relevant data */
static long write_superblock(int bfd) {
    long ret = 0;
    Yaf_Superblock ysb = {};

    /* fill magic string */
    for (int idx = 0; idx < sizeof(ysb.magic); idx += sizeof(MAGIC)) {
        memcpy(&ysb.magic[idx], MAGIC, sizeof(MAGIC));
    }

    /* write down the data */
    if (write(bfd, &ysb, sizeof(ysb)) != sizeof(ysb)) {
        ret = -EIO;
        log(LOG_INFO, "write() failed");
    }

    return ret;
}

int main(int argc, char *argv[])
{
    Arguments arguments = {};
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
    ret = write_superblock(bfd);
    if (ret) {
        ret = errno;
        log(LOG_ERR,
            "write_superblock() failed with error %s", strerror(errno));
        goto close_bfd;
    }

    log(LOG_INFO, "yaf filesystem has been successfully "
        "formatted on the device");

close_bfd:
    close(bfd);
out:
    return ret;
}
