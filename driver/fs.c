#include <linux/module.h>
#include "../include/yaf.h"
#include "super.h"

/* information for module */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hawkins Jiawei");

/*
 * yaf_mount() is invoked by VFS when a request is made to
 * mount the yaf filesystem onto a directory, as detailed in
 * https://docs.kernel.org/next/filesystems/vfs.html#registering-and-mounting-a-filesystem.
 *
 * In essence, yaf_mount() is responsible for parsing the provided
 * block device containing the yaf filesystem image, creating
 * and initializing an in-memory superblock, represented by
 * *struct super_block*, based on the on-disk superblock, and
 * returning its root dentry.
 *
 * A new vfsmount, referring to the tree returned by yaf_mount(),
 * will be attached to the mountpoint's dentry as illustrated in
 * the following diagram. Consequently, when the pathname resolution
 * reaches the "mnt" mountpoint, its *dentry->d_flags* is tagged with
 * *DCACHE_MOUNTED*, forcing kernel jump to the root of that vfsmount
 * baesd on the *mount_hashtable*, according to
 * https://zhuanlan.zhihu.com/p/606596107.

                                       struct file_system_type
                                          ┌─────────┬─────┐
                                          │name     │"yaf"│
                                          ├─────────┼─────┤
                                          │fs_supers│     │◄───┐     struct super_block◄─┐
                                          └─────────┴─────┘    │      ┌───────────┬──┐   │
                                                               └──────┤s_instances│  │   │
                                                                      ├───────────┼──┤   │
                                                                      │s_mounts   │  │   │
                                                                      ├───────────┼──┤   │
                                                                      │s_root     │  │   │
     struct dentry                                                ┌──►└───────────┴─┬┘   │
    ┌─────────┬───┐                    ┌────────┬──┐              │                 │    │
    │d_name   │"/"│                    │mnt_sb  │  ├──────────────┘                 │    │
    ├─────────┼───┤                    ├────────┼──┤                                │    │
┌──►│d_subdirs│   │                    │mnt_root│  ├────────────────►struct dentry◄─┘    │
│   └─────────┴───┘                    └────────┴──┘                 ┌──────┬───┐        │
│                                     struct vfsmount◄─────┐         │d_name│"/"│        │
│                                                          │         ├──────┼───┤        │
│                                                          │         │d_sb  │   ├────────┘
│                                                          │         └──────┴───┘
│  ┌─────────┬──────────────┐                              │
│  │d_name   │"mnt"         │                              │
│  ├─────────┼──────────────┤                              │
└─►│d_subdirs│              │             struct mount     │          struct mount
   ├─────────┼──────────────┤          ┌──────────────┬──┐ │       ┌──────────────┬──┐
   │d_flags  │DCACHE_MOUNTED│          │mnt           │  ├─┘       │mnt           │  │
   └─────────┴──────────────┘          ├──────────────┼──┤         ├──────────────┼──┤
          struct dentry◄───────────────┤mnt_mountpoint│  │         │mnt_mountpoint│  │
               ▲                       ├──────────────┼──┤         ├──────────────┼──┤
               │                     ┌►│mnt_hash      │  │◄───────►│mnt_hash      │  │
               │                     │ └──────────────┴──┘         └──────────────┴──┘
               │                     │
               │   mount_hashtable   │
               │    ┌───┬─────┐      │
               └────┤key│value│◄─────┘
                    └───┴─────┘
 */
static struct dentry* yaf_mount(struct file_system_type *fs_type,
                                int flags, const char *dev_name,
                                void *data)
{
    struct dentry *ret;

    ret = mount_bdev(fs_type, flags, dev_name, data, yaf_fill_super);
    if (IS_ERR(ret)) {
        log(LOG_ERR "yaf: "
            "mount block device failed with error code %ld",
            PTR_ERR(ret));
        goto out;
    }

    log(LOG_INFO "yaf: mount block device");

out:
    return ret;
}

/*
 * This structure describes the filesystem according to
 * https://docs.kernel.org/next/filesystems/vfs.html#struct-file-system-type
 */
static struct file_system_type yaf_file_system_type = {
    .name = "yaf", // the name of the filesystem type

    .mount = yaf_mount, // the method to call when a new instance of
                        // the filesystem shoud be mounted

    .owner = THIS_MODULE, // is used to manage the modules' reference
                          // count, preventing unloading of the module
                          // while the filesystem code is in use according
                          // to https://lwn.net/Articles/13325/

    .next = NULL, // for internal VFS use
};

static int yaf_init(void)
{
    int ret = 0;

    ret = register_filesystem(&yaf_file_system_type);
    if (ret) {
        log(LOG_ERR "yaf: "
            "registering filesystem failed with error code %d", ret);
        goto out;
    }

    log(LOG_INFO "yaf: initialize filesystem");

out:
    return ret;
}

static void yaf_exit(void)
{
    int ret = 0;

    ret = unregister_filesystem(&yaf_file_system_type);
    if (ret) {
        log(LOG_ERR "yaf: "
            "unregistering filesystem failed with error code %d", ret);
    }

    log(LOG_INFO "yaf: cleanup filesystem");
}

module_init(yaf_init);
module_exit(yaf_exit);
