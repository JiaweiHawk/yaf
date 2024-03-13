#ifndef __INODE_H_

    #define __INODE_H_

    /*
     * inode structure
     *
     *                   struct inode                                    on-disk inode
     *                ┌─────────┬───────────┐             ┌──────────┬────────────────────────────────┐◄──0  bytes
     *                │i_mode   │S_IFREG|RWX│◄───────────►│i_mode    │file mode                       │
     *                ├─────────┼───────────┤             ├──────────┼────────────────────────────────┤◄──4  bytes
     *                │i_uid    │           │◄───────────►│i_uid     │owner id                        │
     *                ├─────────┼───────────┤             ├──────────┼────────────────────────────────┤◄──8  bytes
     *                │i_gid    │           │◄───────────►│i_gid     │group id                        │
     *                ├─────────┼───────────┤             ├──────────┼────────────────────────────────┤◄──12 bytes
     *                │i_nlink  │           │◄───────────►│i_nlink   │number of hard links            │
     *                ├─────────┼───────────┤             ├──────────┼────────────────────────────────┤◄──16 bytes
     *                │__i_atime│           │◄───────────►│i_atime   │inode access time               │
     *                ├─────────┼───────────┤             ├──────────┼────────────────────────────────┤◄──20 bytes
     *                │__i_mtime│           │◄───────────►│i_mtime   │inode modification time         │
     *                ├─────────┼───────────┤             ├──────────┼────────────────────────────────┤◄──24 bytes
     *                │__i_ctime│           │◄───────────►│i_ctime   │inode change time               │
     *                ├─────────┼───────────┤             ├──────────┼────────────────────────────────┤◄──28 bytes
     *                │i_size   │           │◄───────────►│i_size    │inode data size in bytes        │
     * ┌──────────┬──┐└─────────┴───────────┘             ├──────────┼────────────────────────────────┤◄──32 bytes     ┌──────────┐
     * │i_block[0]│  │◄──────────────────────────────────►│i_block[0]│data block id for the data block│◄──────────────►│data block│
     * ├──────────┼──┤                                    ├──────────┼────────────────────────────────┤◄──36 bytes     └──────────┘  ┌──────────┐
     * │i_block[1]│  │◄──────────────────────────────────►│i_block[1]│data block id for the data block│◄────────────────────────────►│data block│
     * ├──────────┼──┤                                    ├──────────┼────────────────────────────────┤◄──40 bytes     ┌──────────┐  └──────────┘
     * │ ........ │  │◄──────────────────────────────────►│ ........ │                                │◄──────────────►│data block│
     * ├──────────┼──┤                                    ├──────────┼────────────────────────────────┤◄──56 bytes     └──────────┘   ┌──────────┐
     * │i_block[6]│  │◄──────────────────────────────────►│i_block[6]│data block id for the data block│◄─────────────────────────────►│data block│
     * ├──────────┼──┤                                    ├──────────┼────────────────────────────────┤◄──60 bytes                    └──────────┘
     * │i_block[7]│  │◄──────────────────────────────────►│i_block[7]│data block id for the data block│
     * └──────────┴──┘                                    └──────────┴────────────────────────────────┘◄──64 bytes
     * struct yaf_inode_info
     *
     *
     *                     struct inode                                     on-disk inode
     *                  ┌─────────┬───────────┐             ┌──────────┬──────────────────────────────────┐◄──0  bytes
     *                  │i_mode   │S_IFDIR|RWX│◄───────────►│i_mode    │file mode                         │
     *                  ├─────────┼───────────┤             ├──────────┼──────────────────────────────────┤◄──4  bytes
     *                  │i_uid    │           │◄───────────►│i_uid     │owner id                          │
     *                  ├─────────┼───────────┤             ├──────────┼──────────────────────────────────┤◄──8  bytes
     *                  │i_gid    │           │◄───────────►│i_gid     │group id                          │
     *                  ├─────────┼───────────┤             ├──────────┼──────────────────────────────────┤◄──12 bytes
     *                  │i_nlink  │           │◄───────────►│i_nlink   │number of hard links              │
     *                  ├─────────┼───────────┤             ├──────────┼──────────────────────────────────┤◄──16 bytes
     *                  │__i_atime│           │◄───────────►│i_atime   │inode access time                 │
     *                  ├─────────┼───────────┤             ├──────────┼──────────────────────────────────┤◄──20 bytes
     *                  │__i_mtime│           │◄───────────►│i_mtime   │inode modification time           │
     *                  ├─────────┼───────────┤             ├──────────┼──────────────────────────────────┤◄──24 bytes
     *                  │__i_ctime│           │◄───────────►│i_ctime   │inode change time                 │
     *                  ├─────────┼───────────┤             ├──────────┼──────────────────────────────────┤◄──28 bytes
     *                  │i_size   │           │◄───────────►│i_size    │inode data size in bytes          │
     * ┌──────────┬──┐  └─────────┴───────────┘             ├──────────┼──────────────────────────────────┤◄──32 bytes
     * │i_block[0]│  │◄────────────────────────────────────►│i_block[0]│data block id for the dentry block│
     * ├──────────┼──┤                                      ├──────────┼──────────────────────────────────┤◄──36 bytes
     * │i_block[1]│  │◄────────────────────────────────────►│i_block[1]│data block id for the dentry block│
     * ├──────────┼──┤                                      ├──────────┼──────────────────────────────────┤◄──40 bytes
     * │ ........ │  │◄────────────────────────────────────►│ .......  │                                  │
     * ├──────────┼──┤                                      ├──────────┼──────────────────────────────────┤◄──56 bytes
     * │i_block[6]│  │◄────────────────────────────────────►│i_block[6]│data block id for the dentry block|
     * ├──────────┼──┤                                      ├──────────┼──────────────────────────────────┤◄──60 bytes
     * │i_block[7]│  │◄────────────────────────────────────►│i_block[7]│data block id for the dentry block|
     * └──────────┴──┘                                      └──────────┴──────────┬───────────────────────┘◄──64 bytes
     * struct yaf_inode_info                                                      │
     *                                                                            │
     *                    dentry ◄──────────────────────────────────┐             ▼
     * ┌──────────┬─────────────────────────┐◄──0    bytes          │       dentry block
     * │d_ino     │inode id for the dentry  │                       │      ┌───────────┬───────────────┐◄──0    bytes
     * ├──────────┼─────────────────────────┤◄──4    bytes          └──────┤dentry[0]  │directory entry│
     * │d_name_len│length of the dentry name│                              ├───────────┼───────────────┤◄──32   bytes
     * ├──────────┼─────────────────────────┤◄──8    bytes                 │ ........  │               │
     * │d_name    │dentry name              │                              ├───────────┼───────────────┤◄──4064 bytes
     * └──────────┴─────────────────────────┘◄──32   bytes                 │dentry[128]│directory entry│
     *                                                                     └───────────┴───────────────┘◄──4096 bytes
     */

    /* the array size of *i_block* */
    #define YAF_IBLOCKS         8
    #ifdef __KERNEL__
        #include <linux/types.h>
        #include <linux/fs.h>

        typedef struct YAF_INODE_INFO {
            uint32_t i_block[8];
            struct inode vfs_inode;
        } Yaf_Inode_Info;
    #else // __KERNEL__
        #include <stdint.h>
    #endif // __KERNEL__

    /* on-disk inode structure */
    typedef struct YAF_INODE {
        uint32_t i_mode;                /* file mode */
        uint32_t i_uid;                 /* owner id */
        uint32_t i_gid;                 /* group id */
        uint32_t i_nlink;               /* number of hard links */
        uint32_t i_size;                /* inode data size in bytes */
        uint32_t i_atime;               /* inode access time */
        uint32_t i_mtime;               /* inode modification time */
        uint32_t i_ctime;               /* inode change time */
        uint32_t i_block[YAF_IBLOCKS];  /* block ids for the data block */
    } Yaf_Inode;

    #define YAF_DENTRY_SIZE     32
    #define YAF_DENTRY_NAME_LEN (YAF_DENTRY_SIZE - \
        sizeof(uint32_t) - sizeof(uint32_t))

    typedef struct YAF_DENTRY {
        uint32_t d_ino;                     /* inode id for the dentry */
        uint32_t d_name_len;                /* length of the dentry name */
        char d_name[YAF_DENTRY_NAME_LEN];   /* dentry name */
    } Yaf_Dentry;

    #ifndef __KERNEL__
        #include <assert.h>
    #endif // __KERNEL__
    #include "super.h"
    static_assert(YAF_BLOCK_SIZE % sizeof(Yaf_Inode) == 0);
    static_assert(sizeof(Yaf_Dentry) == YAF_DENTRY_SIZE);

    /* this is reserved as invalid inode number */
    #define RESERVED_INO    0
    /* inode number for the root inode of yaf */
    #define ROOT_INO        1

    /* number of inodes per block */
    #define INODES_PER_BLOCK    (YAF_BLOCK_SIZE / sizeof(Yaf_Inode))

    #include "fs.h"
    /* convert inode number to the corresponding block id */
    #define INO2BID(sb, ino)    (BID_I_MIN((sb)) + \
                                 ((ino)) / INODES_PER_BLOCK)

    /* convert inode number to the offset within its corresponding block */
    #define INO2BOFF(sb, ino)   ((ino) % INODES_PER_BLOCK \
                                 * sizeof(Yaf_Inode))

    /* convert dblock number to the corresponding block id */
    #define DNO2BID(sb, dno)    (BID_D_MIN((sb)) + (dno))

    /* number of dentrys per block */
    #define DENTRYS_PER_BLOCK   (YAF_BLOCK_SIZE / YAF_DENTRY_SIZE)
    #define MAX_DENTRYS         (YAF_IBLOCKS * DENTRYS_PER_BLOCK)

    #define YAF_INODE(inode) \
        ((Yaf_Inode_Info *)container_of(inode, Yaf_Inode_Info, vfs_inode))

    #ifdef __KERNEL__
        #include <linux/fs.h>
        #include <linux/types.h>
        /* fill the in-memory inode according to on-disk inode */
        struct inode *yaf_iget(struct super_block *sb, unsigned long ino);
    #endif // __KERNEL__

#endif // __INODE_H_
